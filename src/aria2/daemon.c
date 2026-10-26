#include "aria2/daemon.h"

#include <glib/gstdio.h>
#include <signal.h>
#include <unistd.h>

struct _Aria2Daemon {
  GSubprocess  *proc;
  GCancellable *cancel;
  Aria2Client  *client;
  gboolean      exited;
};

typedef struct {
  gboolean done;
  gboolean exited;
} WaitState;

static guint16
pick_free_port (GError **error)
{
  g_autoptr (GSocket) sock = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                                           G_SOCKET_PROTOCOL_TCP, error);
  if (!sock)
    return 0;

  g_autoptr (GInetAddress) lo = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  g_autoptr (GSocketAddress) any = g_inet_socket_address_new (lo, 0);
  if (!g_socket_bind (sock, any, TRUE, error))
    return 0;

  g_autoptr (GSocketAddress) bound = g_socket_get_local_address (sock, error);
  if (!bound)
    return 0;

  return g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (bound));
}

static gboolean
ensure_session_file (const char *session_file, GError **error)
{
  g_autofree char *dir = g_path_get_dirname (session_file);
  /* 0700 every time, not just on creation: the session can contain cookies
   * forwarded from the browser, and aria2 rewrites the file with its own mode */
  if (g_mkdir_with_parents (dir, 0700) != 0 || g_chmod (dir, 0700) != 0) {
    g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno), "Couldn't create %s", dir);
    return FALSE;
  }

  /* aria2 refuses to start if --input-file doesn't exist */
  if (g_file_test (session_file, G_FILE_TEST_EXISTS)) {
    g_chmod (session_file, 0600);  /* best effort, the dir already guards it */
    return TRUE;
  }
  return g_file_set_contents_full (session_file, "", 0, G_FILE_SET_CONTENTS_NONE, 0600, error);
}

static void
on_exited (GObject *src, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  g_subprocess_wait_finish (G_SUBPROCESS (src), res, &error);
  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;  /* aria2_daemon_stop() is handling it; user_data may be gone */

  ((Aria2Daemon *) user_data)->exited = TRUE;
}

Aria2Daemon *
aria2_daemon_spawn (const char *download_dir, const char *session_file, GError **error)
{
  g_autofree char *exe = g_find_program_in_path ("aria2c");
  if (!exe) {
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "aria2c was not found in PATH");
    return NULL;
  }

  guint16 port = pick_free_port (error);
  if (!port || !ensure_session_file (session_file, error))
    return NULL;

  g_autofree char *secret = g_uuid_string_random ();
  g_autofree char *a_port = g_strdup_printf ("--rpc-listen-port=%u", port);
  g_autofree char *a_secret = g_strdup_printf ("--rpc-secret=%s", secret);
  g_autofree char *a_dir = g_strdup_printf ("--dir=%s", download_dir);
  g_autofree char *a_in = g_strdup_printf ("--input-file=%s", session_file);
  g_autofree char *a_save = g_strdup_printf ("--save-session=%s", session_file);
  g_autofree char *a_parent = g_strdup_printf ("--stop-with-process=%d", (int) getpid ());

  const char *argv[] = {
    exe,
    "--enable-rpc", "--rpc-listen-all=false", a_port, a_secret,
    a_dir, a_in, a_save, "--save-session-interval=30",
    /* No --continue: aria2 still resumes its own downloads through their
     * .aria2 control files, but never writes into a file that was already
     * there. That's what makes deleting a cancelled download's files safe. */
    "--auto-file-renaming=true",
    "--content-disposition-default-utf8=true",
    "--max-concurrent-downloads=5",
    "--split=16", "--max-connection-per-server=16", "--min-split-size=1M",
    "--quiet=true", a_parent,
    NULL,
  };

  GSubprocess *proc = g_subprocess_newv (argv, G_SUBPROCESS_FLAGS_STDOUT_SILENCE, error);
  if (!proc)
    return NULL;

  Aria2Daemon *d = g_new0 (Aria2Daemon, 1);
  d->proc = proc;
  d->cancel = g_cancellable_new ();
  d->client = aria2_client_new (port, secret);
  g_subprocess_wait_async (proc, d->cancel, on_exited, d);
  return d;
}

Aria2Client *
aria2_daemon_get_client (Aria2Daemon *d)
{
  return d->client;
}

gboolean
aria2_daemon_is_running (Aria2Daemon *d)
{
  return d && !d->exited;
}

static void
on_wait_done (GObject *src, GAsyncResult *res, gpointer user_data)
{
  WaitState *st = user_data;
  st->exited = g_subprocess_wait_finish (G_SUBPROCESS (src), res, NULL);
  st->done = TRUE;
}

static gboolean
on_wait_timeout (gpointer cancel)
{
  g_cancellable_cancel (cancel);
  return G_SOURCE_REMOVE;
}

/* Blocks for up to ms milliseconds without running the app's main loop. */
static gboolean
wait_with_timeout (GSubprocess *proc, guint ms)
{
  g_autoptr (GMainContext) ctx = g_main_context_new ();
  g_autoptr (GCancellable) cancel = g_cancellable_new ();
  WaitState st = { 0 };

  g_main_context_push_thread_default (ctx);

  GSource *timeout = g_timeout_source_new (ms);
  g_source_set_callback (timeout, on_wait_timeout, cancel, NULL);
  g_source_attach (timeout, ctx);

  /* on_wait_done runs either when the process exits or when the timeout cancels */
  g_subprocess_wait_async (proc, cancel, on_wait_done, &st);
  while (!st.done)
    g_main_context_iteration (ctx, TRUE);

  g_source_destroy (timeout);
  g_source_unref (timeout);
  g_main_context_pop_thread_default (ctx);
  return st.exited;
}

void
aria2_daemon_stop (Aria2Daemon *d)
{
  if (!d)
    return;

  g_cancellable_cancel (d->cancel);

  /* SIGTERM makes aria2 shut down gracefully and write the session file.
   * A second one skips the graceful part (e.g. tracker announces). */
  if (g_subprocess_get_identifier (d->proc)) {
    g_subprocess_send_signal (d->proc, SIGTERM);
    if (!wait_with_timeout (d->proc, 3000)) {
      g_subprocess_send_signal (d->proc, SIGTERM);
      if (!wait_with_timeout (d->proc, 2000))
        g_subprocess_force_exit (d->proc);
    }
  }

  aria2_client_unref (d->client);
  g_object_unref (d->proc);
  g_object_unref (d->cancel);
  g_free (d);
}
