#pragma once

#include <gtk/gtk.h>

/* Bottom bar with the total download and upload speed. */
#define YOINK_TYPE_FOOTER (yoink_footer_get_type ())
G_DECLARE_FINAL_TYPE (YoinkFooter, yoink_footer, YOINK, FOOTER, GtkBox)

YoinkFooter *yoink_footer_new        (void);
void      yoink_footer_set_speeds (YoinkFooter *self, gint64 download, gint64 upload);
