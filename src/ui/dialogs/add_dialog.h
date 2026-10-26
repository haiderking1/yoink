#pragma once

#include <gtk/gtk.h>

/* Asks for links (pre-filled from the clipboard) and fires app.add-links,
 * or hands over to the torrent picker. */
void dy_add_dialog_present (GtkWidget *parent);
