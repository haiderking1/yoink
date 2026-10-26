#include "ui/dialogs/torrent_picker.h"

static void
on_picked (GObject *src, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GtkWidget) parent = user_data;
  g_autoptr (GListModel) files = gtk_file_dialog_open_multiple_finish (GTK_FILE_DIALOG (src), res, NULL);
  guint n = files ? g_list_model_get_n_items (files) : 0;

  for (guint i = 0; i < n; i++) {
    g_autoptr (GFile) file = g_list_model_get_item (files, i);
    g_autofree char *uri = g_file_get_uri (file);
    gtk_widget_activate_action (parent, "app.add-torrent", "s", uri);
  }
}

void
yoink_torrent_picker_present (GtkWidget *parent)
{
  g_autoptr (GtkFileDialog) fd = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (fd, "Open Torrent Files");

  g_autoptr (GtkFileFilter) filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "Torrent files");
  gtk_file_filter_add_pattern (filter, "*.torrent");
  gtk_file_filter_add_mime_type (filter, "application/x-bittorrent");

  g_autoptr (GListStore) filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (filters, filter);
  gtk_file_dialog_set_filters (fd, G_LIST_MODEL (filters));

  GtkRoot *root = gtk_widget_get_root (parent);
  gtk_file_dialog_open_multiple (fd, GTK_IS_WINDOW (root) ? GTK_WINDOW (root) : NULL, NULL,
                                 on_picked, g_object_ref (parent));
}
