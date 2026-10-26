#pragma once

#include <adwaita.h>

#include "aria2/client.h"
#include "downloads/request.h"

/* Owns the aria2 daemon, the poller and the window, and glues them together. */
#define YOINK_TYPE_APPLICATION (yoink_application_get_type ())
G_DECLARE_FINAL_TYPE (YoinkApplication, yoink_application, YOINK, APPLICATION, AdwApplication)

YoinkApplication *yoink_application_new              (void);

/* NULL while aria2 isn't running */
Aria2Client   *yoink_application_get_client       (YoinkApplication *self);
const char    *yoink_application_get_download_dir (YoinkApplication *self);
/* NULL once the window is closed */
GtkWidget     *yoink_application_get_window       (YoinkApplication *self);

/* Adds downloads now, or as soon as aria2 is up. Takes ownership; either may be NULL. */
void           yoink_application_submit           (YoinkApplication *self, YoinkRequest *request, GListModel *torrents);
void           yoink_application_restart_aria2    (YoinkApplication *self);
/* poll aria2 right away instead of waiting for the next tick */
void           yoink_application_refresh          (YoinkApplication *self);
/* short message to the user (a toast) */
void           yoink_application_report           (YoinkApplication *self, const char *message);
