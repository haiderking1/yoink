#include "app/application.h"

#include "app/actions.h"
#include "app/cmdline.h"
#include "app/notifier.h"
#include "app/paths.h"
#include "app/version.h"
#include "aria2/daemon.h"
#include "downloads/add.h"
#include "downloads/poller.h"
#include "ui/style/style.h"
#include "ui/window.h"

#define CONNECT_TIMEOUT (10 * G_USEC_PER_SEC)

struct _DyApplication {
  AdwApplication parent_instance;

  Aria2Daemon *daemon;
  DyPoller    *poller;
  DyNotifier  *notifier;
  DyWindow    *window;  /* weak */
  char        *download_dir;

  /* asked for before aria2 was ready */
  GPtrArray   *pending_requests;
  GListStore  *pending_torrents;

  gboolean     connected;
  gint64       started_at;
};

G_DEFINE_FINAL_TYPE (DyApplication, dy_application, ADW_TYPE_APPLICATION)

/* ---------- adding ---------- */

static void
on_added (guint added, const char *error, gpointer user_data)
{
  DyApplication *self = user_data;

  if (error) {
    g_autofree char *msg = g_strdup_printf ("Couldn't add download: %s", error);
    dy_application_report (self, msg);
  } else if (added == 1) {
    dy_application_report (self, "Download added");
  } else if (added > 1) {
    g_autofree char *msg = g_strdup_printf ("%u downloads added", added);
    dy_application_report (self, msg);
  }
  dy_application_refresh (self);
}

static void
flush_pending (DyApplication *self)
{
  Aria2Client *client = dy_application_get_client (self);

  for (guint i = 0; i < self->pending_requests->len; i++)
    dy_add_request (client, g_ptr_array_index (self->pending_requests, i), on_added, self);
  g_ptr_array_set_size (self->pending_requests, 0);

  dy_add_torrents (client, G_LIST_MODEL (self->pending_torrents), on_added, self);
  g_list_store_remove_all (self->pending_torrents);
}

void
dy_application_submit (DyApplication *self, DyRequest *request, GListModel *torrents)
{
  if (request)
    g_ptr_array_add (self->pending_requests, request);

  guint n = torrents ? g_list_model_get_n_items (torrents) : 0;
  for (guint i = 0; i < n; i++) {
    g_autoptr (GObject) file = g_list_model_get_item (torrents, i);
    g_list_store_append (self->pending_torrents, file);
  }
  g_clear_object (&torrents);

  if (self->connected)
    flush_pending (self);
}

/* ---------- aria2 lifecycle ---------- */

static void
fatal (DyApplication *self, const char *title, const char *description)
{
  g_clear_pointer (&self->poller, dy_poller_free);
  if (self->window)
    dy_window_show_failed (self->window, title, description);
}

static void
on_poll (GPtrArray *items, const DyStats *stats, const GError *error, gpointer user_data)
{
  DyApplication *self = user_data;

  if (error) {
    if (!aria2_daemon_is_running (self->daemon))
      fatal (self, "aria2 Stopped", "The aria2 process exited unexpectedly.");
    else if (!self->connected && g_get_monotonic_time () - self->started_at > CONNECT_TIMEOUT)
      fatal (self, "Couldn't Connect to aria2", error->message);
    else if (!self->connected)
      dy_poller_poll_soon (self->poller);  /* aria2 takes a moment to open its RPC port */
    return;
  }

  if (!self->connected) {
    self->connected = TRUE;
    flush_pending (self);
  }
  if (self->window)
    dy_window_update (self->window, items, stats);
  dy_notifier_update (self->notifier, items);
}

static void
stop_aria2 (DyApplication *self)
{
  g_clear_pointer (&self->poller, dy_poller_free);
  g_clear_pointer (&self->daemon, aria2_daemon_stop);
  self->connected = FALSE;
}

void
dy_application_restart_aria2 (DyApplication *self)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *session = dy_paths_session_file ();

  stop_aria2 (self);
  if (self->window) {
    dy_window_clear (self->window);
    dy_window_show_starting (self->window);
  }

  self->daemon = aria2_daemon_spawn (self->download_dir, session, &error);
  if (!self->daemon) {
    fatal (self, "Couldn't Start aria2", error->message);
    return;
  }

  self->started_at = g_get_monotonic_time ();
  self->poller = dy_poller_new (aria2_daemon_get_client (self->daemon), on_poll, self);
}

/* ---------- GApplication ---------- */

static void
on_cmdline_submit (DyRequest *request, GListModel *torrents, gpointer user_data)
{
  dy_application_submit (user_data, request, torrents);
}

static void
dy_application_startup (GApplication *app)
{
  DyApplication *self = DY_APPLICATION (app);
  G_APPLICATION_CLASS (dy_application_parent_class)->startup (app);

  dy_style_load ();
  gtk_window_set_default_icon_name (DY_APP_ID);
  dy_actions_install (self);
  self->download_dir = dy_paths_download_dir ();
  self->notifier = dy_notifier_new (app);
}

static void
dy_application_activate (GApplication *app)
{
  DyApplication *self = DY_APPLICATION (app);

  if (self->window) {
    gtk_window_present (GTK_WINDOW (self->window));
    return;
  }

  self->window = dy_window_new (GTK_APPLICATION (app));
  g_object_add_weak_pointer (G_OBJECT (self->window), (gpointer *) &self->window);
  gtk_window_present (GTK_WINDOW (self->window));

  if (!self->daemon)
    dy_application_restart_aria2 (self);
}

static int
dy_application_command_line (GApplication *app, GApplicationCommandLine *cmd)
{
  g_application_activate (app);
  return dy_cmdline_handle (cmd, on_cmdline_submit, app);
}

static void
dy_application_shutdown (GApplication *app)
{
  DyApplication *self = DY_APPLICATION (app);

  stop_aria2 (self);
  g_clear_pointer (&self->notifier, dy_notifier_free);
  G_APPLICATION_CLASS (dy_application_parent_class)->shutdown (app);
}

static void
dy_application_finalize (GObject *object)
{
  DyApplication *self = DY_APPLICATION (object);

  g_ptr_array_unref (self->pending_requests);
  g_object_unref (self->pending_torrents);
  g_free (self->download_dir);
  G_OBJECT_CLASS (dy_application_parent_class)->finalize (object);
}

static void
dy_application_class_init (DyApplicationClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = dy_application_finalize;
  G_APPLICATION_CLASS (klass)->startup = dy_application_startup;
  G_APPLICATION_CLASS (klass)->activate = dy_application_activate;
  G_APPLICATION_CLASS (klass)->command_line = dy_application_command_line;
  G_APPLICATION_CLASS (klass)->shutdown = dy_application_shutdown;
}

static void
dy_application_init (DyApplication *self)
{
  self->pending_requests = g_ptr_array_new_with_free_func ((GDestroyNotify) dy_request_free);
  self->pending_torrents = g_list_store_new (G_TYPE_FILE);
  dy_cmdline_register (G_APPLICATION (self));
}

DyApplication *
dy_application_new (void)
{
  return g_object_new (DY_TYPE_APPLICATION,
                       "application-id", DY_APP_ID,
                       "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                       NULL);
}

/* ---------- accessors ---------- */

Aria2Client *
dy_application_get_client (DyApplication *self)
{
  return self->daemon ? aria2_daemon_get_client (self->daemon) : NULL;
}

const char *
dy_application_get_download_dir (DyApplication *self)
{
  return self->download_dir;
}

GtkWidget *
dy_application_get_window (DyApplication *self)
{
  return GTK_WIDGET (self->window);
}

void
dy_application_refresh (DyApplication *self)
{
  if (self->poller)
    dy_poller_poll_soon (self->poller);
}

void
dy_application_report (DyApplication *self, const char *message)
{
  if (self->window)
    dy_window_toast (self->window, message);
}
