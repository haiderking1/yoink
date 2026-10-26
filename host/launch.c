#include "launch.h"

#include <gio/gio.h>
#include <string.h>
#include <unistd.h>

#define YOINK_BUS_NAME "dev.soka.yoink"

/* yoink-host is installed next to yoink; fall back to PATH. */
static char *
find_yoink (GError **error)
{
  g_autofree char *self = g_file_read_link ("/proc/self/exe", NULL);
  if (self) {
    g_autofree char *dir = g_path_get_dirname (self);
    char *sibling = g_build_filename (dir, "yoink", NULL);
    if (g_file_test (sibling, G_FILE_TEST_IS_EXECUTABLE))
      return sibling;
    g_free (sibling);
  }

  char *found = g_find_program_in_path ("yoink");
  if (!found)
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Couldn't find the yoink program");
  return found;
}

static gboolean
yoink_is_running (void)
{
  g_autoptr (GDBusConnection) bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  if (!bus)
    return FALSE;

  g_autoptr (GVariant) reply = g_dbus_connection_call_sync (bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                            "org.freedesktop.DBus", "NameHasOwner",
                                                            g_variant_new ("(s)", YOINK_BUS_NAME),
                                                            G_VARIANT_TYPE ("(b)"), G_DBUS_CALL_FLAGS_NONE,
                                                            2000, NULL, NULL);
  gboolean owned = FALSE;
  if (reply)
    g_variant_get (reply, "(b)", &owned);
  return owned;
}

/* The browser may kill our process group once we reply; Yoink must outlive us. */
static void
detach (gpointer unused)
{
  setsid ();
}

static gboolean
forward_to_running (const char *yoink, const char *request_json, GError **error)
{
  g_autoptr (GSubprocessLauncher) launcher =
    g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_SILENCE |
                               G_SUBPROCESS_FLAGS_STDERR_PIPE);
  g_autoptr (GSubprocess) proc = g_subprocess_launcher_spawn (launcher, error, yoink, "--request-stdin", NULL);
  if (!proc)
    return FALSE;

  g_autofree char *err_out = NULL;
  if (!g_subprocess_communicate_utf8 (proc, request_json, NULL, NULL, &err_out, error))
    return FALSE;

  if (g_subprocess_get_if_exited (proc) && g_subprocess_get_exit_status (proc) == 0)
    return TRUE;

  /* yoink prints "yoink: <reason>" */
  const char *reason = err_out ? g_strstrip (err_out) : "";
  if (g_str_has_prefix (reason, "yoink: "))
    reason += 4;
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, *reason ? reason : "Yoink rejected the request");
  return FALSE;
}

static gboolean
start_with_request (const char *yoink, const char *request_json, GError **error)
{
  g_autoptr (GSubprocessLauncher) launcher =
    g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_SILENCE |
                               G_SUBPROCESS_FLAGS_STDERR_SILENCE);
  g_subprocess_launcher_set_child_setup (launcher, detach, NULL, NULL);

  g_autoptr (GSubprocess) proc = g_subprocess_launcher_spawn (launcher, error, yoink, "--request-stdin", NULL);
  if (!proc)
    return FALSE;

  GOutputStream *in = g_subprocess_get_stdin_pipe (proc);
  return g_output_stream_write_all (in, request_json, strlen (request_json), NULL, NULL, error) &&
         g_output_stream_close (in, NULL, error);
}

gboolean
yoink_host_launch (const char *request_json, GError **error)
{
  g_autofree char *yoink = find_yoink (error);
  if (!yoink)
    return FALSE;

  return yoink_is_running () ? forward_to_running (yoink, request_json, error)
                             : start_with_request (yoink, request_json, error);
}
