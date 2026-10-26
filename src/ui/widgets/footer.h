#pragma once

#include <gtk/gtk.h>

/* Bottom bar with the total download and upload speed. */
#define DY_TYPE_FOOTER (dy_footer_get_type ())
G_DECLARE_FINAL_TYPE (DyFooter, dy_footer, DY, FOOTER, GtkBox)

DyFooter *dy_footer_new        (void);
void      dy_footer_set_speeds (DyFooter *self, gint64 download, gint64 upload);
