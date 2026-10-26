#pragma once

#include <gtk/gtk.h>

/* Opens a file or folder with its default app. from: any widget in the window. */
void dy_launch_open           (GtkWidget *from, const char *path);
/* Opens the file manager with the file selected. */
void dy_launch_show_in_folder (GtkWidget *from, const char *path);
