#pragma once

#include <gtk/gtk.h>

/* File chooser for .torrent files; the picks fire app.add-torrents. */
void yoink_torrent_picker_present (GtkWidget *parent);
