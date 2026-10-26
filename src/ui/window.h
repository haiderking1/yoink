#pragma once

#include <adwaita.h>

#include "downloads/poller.h"

/* Main window: header, the download list or a status page, and the speed footer. */
#define DY_TYPE_WINDOW (dy_window_get_type ())
G_DECLARE_FINAL_TYPE (DyWindow, dy_window, DY, WINDOW, AdwApplicationWindow)

DyWindow *dy_window_new           (GtkApplication *app);
void      dy_window_show_starting (DyWindow *self);
void      dy_window_show_failed   (DyWindow *self, const char *title, const char *description);
/* items: DyItem*, in display order */
void      dy_window_update        (DyWindow *self, GPtrArray *items, const DyStats *stats);
void      dy_window_clear         (DyWindow *self);
void      dy_window_toast         (DyWindow *self, const char *message);
