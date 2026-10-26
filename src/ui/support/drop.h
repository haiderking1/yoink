#pragma once

#include <gtk/gtk.h>

/* Links or .torrent files dropped on target become app.add-links /
 * app.add-torrent actions. */
void dy_drop_install (GtkWidget *target);
