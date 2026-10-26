#pragma once

#include <gtk/gtk.h>

/* Links or .torrent files dropped on target become app.add-links /
 * app.add-torrents actions. */
void yoink_drop_install (GtkWidget *target);
