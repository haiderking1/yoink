#pragma once

#include <gtk/gtk.h>

/* File chooser for .torrent files; each pick fires app.add-torrent. */
void yoink_torrent_picker_present (GtkWidget *parent);
