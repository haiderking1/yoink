#include "ui/style/style.h"

#include <gtk/gtk.h>

static const char style_css[] = {
#embed "style.css"
  , 0
};

void
dy_style_load (void)
{
  g_autoptr (GtkCssProvider) css = gtk_css_provider_new ();
  gtk_css_provider_load_from_string (css, style_css);
  gtk_style_context_add_provider_for_display (gdk_display_get_default (), GTK_STYLE_PROVIDER (css),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
