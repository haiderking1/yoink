#include "ui/support/launch.h"

static GtkWindow *
window_of (GtkWidget *w)
{
  GtkRoot *root = w ? gtk_widget_get_root (w) : NULL;
  return GTK_IS_WINDOW (root) ? GTK_WINDOW (root) : NULL;
}

void
yoink_launch_open (GtkWidget *from, const char *path)
{
  if (!path)
    return;
  g_autoptr (GFile) file = g_file_new_for_path (path);
  g_autoptr (GtkFileLauncher) launcher = gtk_file_launcher_new (file);
  gtk_file_launcher_launch (launcher, window_of (from), NULL, NULL, NULL);
}

void
yoink_launch_show_in_folder (GtkWidget *from, const char *path)
{
  if (!path)
    return;
  g_autoptr (GFile) file = g_file_new_for_path (path);
  g_autoptr (GtkFileLauncher) launcher = gtk_file_launcher_new (file);
  gtk_file_launcher_open_containing_folder (launcher, window_of (from), NULL, NULL, NULL);
}
