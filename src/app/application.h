#pragma once

#include <adwaita.h>

#include "aria2/client.h"
#include "downloads/request.h"

/* Owns the aria2 daemon, the poller and the window, and glues them together. */
#define DY_TYPE_APPLICATION (dy_application_get_type ())
G_DECLARE_FINAL_TYPE (DyApplication, dy_application, DY, APPLICATION, AdwApplication)

DyApplication *dy_application_new              (void);

/* NULL while aria2 isn't running */
Aria2Client   *dy_application_get_client       (DyApplication *self);
const char    *dy_application_get_download_dir (DyApplication *self);
/* NULL once the window is closed */
GtkWidget     *dy_application_get_window       (DyApplication *self);

/* Adds downloads now, or as soon as aria2 is up. Takes ownership; either may be NULL. */
void           dy_application_submit           (DyApplication *self, DyRequest *request, GListModel *torrents);
void           dy_application_restart_aria2    (DyApplication *self);
/* poll aria2 right away instead of waiting for the next tick */
void           dy_application_refresh          (DyApplication *self);
/* short message to the user (a toast) */
void           dy_application_report           (DyApplication *self, const char *message);
