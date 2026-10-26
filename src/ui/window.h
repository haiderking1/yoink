#pragma once

#include <adwaita.h>

#include "downloads/poller.h"

/* Main window: header, the download list or a status page, and the speed footer. */
#define YOINK_TYPE_WINDOW (yoink_window_get_type ())
G_DECLARE_FINAL_TYPE (YoinkWindow, yoink_window, YOINK, WINDOW, AdwApplicationWindow)

YoinkWindow *yoink_window_new           (GtkApplication *app);
void      yoink_window_show_starting (YoinkWindow *self);
void      yoink_window_show_failed   (YoinkWindow *self, const char *title, const char *description);
/* items: YoinkItem*, in display order */
void      yoink_window_update        (YoinkWindow *self, GPtrArray *items, const YoinkStats *stats);
void      yoink_window_clear         (YoinkWindow *self);
void      yoink_window_toast         (YoinkWindow *self, const char *message);
