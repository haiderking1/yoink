#include "app/application.h"

#include "app/actions.h"
#include "app/cmdline.h"
#include "app/notifier.h"
#include "app/paths.h"
#include "app/save_prompt.h"
#include "app/settings.h"
#include "app/version.h"
#include "aria2/daemon.h"
#include "downloads/add.h"
#include "downloads/poller.h"
#include "ui/style/style.h"
#include "ui/window.h"

#define CONNECT_TIMEOUT (10 * G_USEC_PER_SEC)

struct _YoinkApplication {
  AdwApplication parent_instance;

  Aria2Daemon     *daemon;
  YoinkPoller     *poller;
  YoinkNotifier   *notifier;
  YoinkSavePrompt *save_prompt;
  YoinkWindow     *window;  /* weak */

  /* accepted before aria2 was ready */
  GPtrArray   *pending_requests;

  gboolean     connected;
  gint64       started_at;
};

G_DEFINE_FINAL_TYPE (YoinkApplication, yoink_application, ADW_TYPE_APPLICATION)

/* ---------- adding ---------- */

static void
on_added (guint added, const char *error, gpointer user_data)
{
  YoinkApplication *self = user_data;

  if (error) {
    g_autofree char *msg = g_strdup_printf ("Couldn't add download: %s", error);
    yoink_application_report (self, msg);
  } else if (added == 1) {
    yoink_application_report (self, "Download added");
  } else if (added > 1) {
    g_autofree char *msg = g_strdup_printf ("%u downloads added", added);
    yoink_application_report (self, msg);
  }
  yoink_application_refresh (self);
}

static void
flush_pending (YoinkApplication *self)
{
  Aria2Client *client = yoink_application_get_client (self);

  for (guint i = 0; i < self->pending_requests->len; i++)
    yoink_add_request (client, g_ptr_array_index (self->pending_requests, i), on_added, self);
  g_ptr_array_set_size (self->pending_requests, 0);
}

static void
on_save_accepted (YoinkRequest *request, gpointer user_data)
{
  YoinkApplication *self = user_data;

  g_ptr_array_add (self->pending_requests, request);
  if (self->connected)
    flush_pending (self);
}

void
yoink_application_submit (YoinkApplication *self, YoinkRequest *request)
{
  yoink_save_prompt_push (self->save_prompt, request);
}

/* ---------- aria2 lifecycle ---------- */

static void
fatal (YoinkApplication *self, const char *title, const char *description)
{
  g_clear_pointer (&self->poller, yoink_poller_free);
  if (self->window)
    yoink_window_show_failed (self->window, title, description);
}

static void
on_poll (GPtrArray *items, const YoinkStats *stats, const GError *error, gpointer user_data)
{
  YoinkApplication *self = user_data;

  if (error) {
    if (!aria2_daemon_is_running (self->daemon))
      fatal (self, "aria2 Stopped", "The aria2 process exited unexpectedly.");
    else if (!self->connected && g_get_monotonic_time () - self->started_at > CONNECT_TIMEOUT)
      fatal (self, "Couldn't Connect to aria2", error->message);
    else if (!self->connected)
      yoink_poller_poll_soon (self->poller);  /* aria2 takes a moment to open its RPC port */
    return;
  }

  if (!self->connected) {
    self->connected = TRUE;
    flush_pending (self);
  }
  if (self->window)
    yoink_window_update (self->window, items, stats);
  yoink_notifier_update (self->notifier, items);
}

static void
stop_aria2 (YoinkApplication *self)
{
  g_clear_pointer (&self->poller, yoink_poller_free);
  g_clear_pointer (&self->daemon, aria2_daemon_stop);
  self->connected = FALSE;
}

void
yoink_application_restart_aria2 (YoinkApplication *self)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *session = yoink_paths_session_file ();
  g_autofree char *dir = yoink_settings_get_save_dir ();

  stop_aria2 (self);
  if (self->window) {
    yoink_window_clear (self->window);
    yoink_window_show_starting (self->window);
  }

  self->daemon = aria2_daemon_spawn (dir, session, &error);
  if (!self->daemon) {
    fatal (self, "Couldn't Start aria2", error->message);
    return;
  }

  self->started_at = g_get_monotonic_time ();
  self->poller = yoink_poller_new (aria2_daemon_get_client (self->daemon), on_poll, self);
}

/* ---------- GApplication ---------- */

static void
on_cmdline_submit (YoinkRequest *request, gpointer user_data)
{
  yoink_application_submit (user_data, request);
}

static void
yoink_application_startup (GApplication *app)
{
  YoinkApplication *self = YOINK_APPLICATION (app);
  G_APPLICATION_CLASS (yoink_application_parent_class)->startup (app);

  yoink_style_load ();
  gtk_window_set_default_icon_name (YOINK_APP_ID);
  yoink_actions_install (self);
  self->save_prompt = yoink_save_prompt_new (GTK_APPLICATION (app), on_save_accepted, self);
  self->notifier = yoink_notifier_new (app);
}

static void
yoink_application_activate (GApplication *app)
{
  YoinkApplication *self = YOINK_APPLICATION (app);

  if (self->window) {
    gtk_window_present (GTK_WINDOW (self->window));
    return;
  }

  self->window = yoink_window_new (GTK_APPLICATION (app));
  g_object_add_weak_pointer (G_OBJECT (self->window), (gpointer *) &self->window);
  gtk_window_present (GTK_WINDOW (self->window));

  if (!self->daemon)
    yoink_application_restart_aria2 (self);
}

static int
yoink_application_command_line (GApplication *app, GApplicationCommandLine *cmd)
{
  g_application_activate (app);
  return yoink_cmdline_handle (cmd, on_cmdline_submit, app);
}

static void
yoink_application_shutdown (GApplication *app)
{
  YoinkApplication *self = YOINK_APPLICATION (app);

  stop_aria2 (self);
  g_clear_pointer (&self->notifier, yoink_notifier_free);
  G_APPLICATION_CLASS (yoink_application_parent_class)->shutdown (app);
}

static void
yoink_application_finalize (GObject *object)
{
  YoinkApplication *self = YOINK_APPLICATION (object);

  g_ptr_array_unref (self->pending_requests);
  g_clear_pointer (&self->save_prompt, yoink_save_prompt_free);
  G_OBJECT_CLASS (yoink_application_parent_class)->finalize (object);
}

static void
yoink_application_class_init (YoinkApplicationClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = yoink_application_finalize;
  G_APPLICATION_CLASS (klass)->startup = yoink_application_startup;
  G_APPLICATION_CLASS (klass)->activate = yoink_application_activate;
  G_APPLICATION_CLASS (klass)->command_line = yoink_application_command_line;
  G_APPLICATION_CLASS (klass)->shutdown = yoink_application_shutdown;
}

static void
yoink_application_init (YoinkApplication *self)
{
  self->pending_requests = g_ptr_array_new_with_free_func ((GDestroyNotify) yoink_request_free);
  yoink_cmdline_register (G_APPLICATION (self));
}

YoinkApplication *
yoink_application_new (void)
{
  return g_object_new (YOINK_TYPE_APPLICATION,
                       "application-id", YOINK_APP_ID,
                       "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                       NULL);
}

/* ---------- accessors ---------- */

Aria2Client *
yoink_application_get_client (YoinkApplication *self)
{
  return self->daemon ? aria2_daemon_get_client (self->daemon) : NULL;
}

GtkWidget *
yoink_application_get_window (YoinkApplication *self)
{
  return GTK_WIDGET (self->window);
}

void
yoink_application_refresh (YoinkApplication *self)
{
  if (self->poller)
    yoink_poller_poll_soon (self->poller);
}

void
yoink_application_report (YoinkApplication *self, const char *message)
{
  if (self->window)
    yoink_window_toast (self->window, message);
}
