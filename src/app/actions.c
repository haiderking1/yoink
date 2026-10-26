#include "app/actions.h"

#include "app/settings.h"
#include "downloads/control.h"
#include "downloads/removal.h"
#include "downloads/retry.h"
#include "ui/dialogs/about.h"
#include "ui/dialogs/add_dialog.h"
#include "ui/support/launch.h"

typedef void (*GidControl) (Aria2Client *, const char *, YoinkControlDone, gpointer);
typedef void (*GlobalControl) (Aria2Client *, YoinkControlDone, gpointer);

static void
on_control_done (const char *error, gpointer user_data)
{
  YoinkApplication *app = user_data;
  if (error)
    yoink_application_report (app, error);
  yoink_application_refresh (app);
}

static void
run_gid (YoinkApplication *app, GVariant *param, GidControl fn)
{
  Aria2Client *client = yoink_application_get_client (app);
  if (client)
    fn (client, g_variant_get_string (param, NULL), on_control_done, app);
}

static void
run_global (YoinkApplication *app, GlobalControl fn)
{
  Aria2Client *client = yoink_application_get_client (app);
  if (client)
    fn (client, on_control_done, app);
}

/* ---------- adding ---------- */

static void
act_add (GSimpleAction *a, GVariant *p, gpointer app)
{
  GtkWidget *window = yoink_application_get_window (app);
  if (window)
    yoink_add_dialog_present (window);
}

static void
act_add_links (GSimpleAction *a, GVariant *p, gpointer app)
{
  g_autoptr (YoinkRequest) req = yoink_request_new ();
  g_auto (GStrv) words = g_regex_split_simple ("\\s+", g_variant_get_string (p, NULL), 0, 0);

  for (char **w = words; *w; w++)
    if (**w)
      yoink_request_add_uri (req, *w, NULL);  /* anything that isn't a link is skipped */

  if (req->uris->len == 0) {
    yoink_application_report (app, "No links found");
    return;
  }
  yoink_application_submit (app, g_steal_pointer (&req));
}

static void
act_add_torrents (GSimpleAction *a, GVariant *p, gpointer app)
{
  g_autoptr (YoinkRequest) req = yoink_request_new ();
  g_autofree const char **uris = g_variant_get_strv (p, NULL);

  for (const char **u = uris; *u; u++) {
    g_autoptr (GFile) file = g_file_new_for_uri (*u);
    yoink_request_add_torrent (req, file);
  }
  if (req->torrents->len)
    yoink_application_submit (app, g_steal_pointer (&req));
}

/* ---------- per download ---------- */

static void act_pause   (GSimpleAction *a, GVariant *p, gpointer app) { run_gid (app, p, yoink_control_pause); }
static void act_resume  (GSimpleAction *a, GVariant *p, gpointer app) { run_gid (app, p, yoink_control_resume); }
static void act_retry   (GSimpleAction *a, GVariant *p, gpointer app) { run_gid (app, p, yoink_retry); }
static void act_cancel  (GSimpleAction *a, GVariant *p, gpointer app) { run_gid (app, p, yoink_removal_cancel); }
static void act_dismiss (GSimpleAction *a, GVariant *p, gpointer app) { run_gid (app, p, yoink_removal_dismiss); }

/* ---------- all downloads ---------- */

static void act_pause_all      (GSimpleAction *a, GVariant *p, gpointer app) { run_global (app, yoink_control_pause_all); }
static void act_resume_all     (GSimpleAction *a, GVariant *p, gpointer app) { run_global (app, yoink_control_resume_all); }
static void act_clear_finished (GSimpleAction *a, GVariant *p, gpointer app) { run_global (app, yoink_removal_clear_finished); }

/* ---------- app ---------- */

static void
act_open_folder (GSimpleAction *a, GVariant *p, gpointer app)
{
  g_autofree char *dir = yoink_settings_get_save_dir ();
  yoink_launch_open (yoink_application_get_window (app), dir);
}

static void
act_restart (GSimpleAction *a, GVariant *p, gpointer app)
{
  yoink_application_restart_aria2 (app);
}

static void
act_about (GSimpleAction *a, GVariant *p, gpointer app)
{
  GtkWidget *window = yoink_application_get_window (app);
  if (window)
    yoink_about_present (window);
}

static void
act_quit (GSimpleAction *a, GVariant *p, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
}

void
yoink_actions_install (YoinkApplication *app)
{
  static const GActionEntry entries[] = {
    { .name = "add",            .activate = act_add },
    { .name = "add-links",      .activate = act_add_links,    .parameter_type = "s" },
    { .name = "add-torrents",   .activate = act_add_torrents, .parameter_type = "as" },
    { .name = "pause",          .activate = act_pause,        .parameter_type = "s" },
    { .name = "resume",         .activate = act_resume,       .parameter_type = "s" },
    { .name = "retry",          .activate = act_retry,        .parameter_type = "s" },
    { .name = "cancel",         .activate = act_cancel,       .parameter_type = "s" },
    { .name = "dismiss",        .activate = act_dismiss,      .parameter_type = "s" },
    { .name = "pause-all",      .activate = act_pause_all },
    { .name = "resume-all",     .activate = act_resume_all },
    { .name = "clear-finished", .activate = act_clear_finished },
    { .name = "open-folder",    .activate = act_open_folder },
    { .name = "restart",        .activate = act_restart },
    { .name = "about",          .activate = act_about },
    { .name = "quit",           .activate = act_quit },
  };
  g_action_map_add_action_entries (G_ACTION_MAP (app), entries, G_N_ELEMENTS (entries), app);

  GtkApplication *gtk_app = GTK_APPLICATION (app);
  gtk_application_set_accels_for_action (gtk_app, "app.add", (const char *[]) { "<Ctrl>n", NULL });
  gtk_application_set_accels_for_action (gtk_app, "app.quit", (const char *[]) { "<Ctrl>q", "<Ctrl>w", NULL });
}
