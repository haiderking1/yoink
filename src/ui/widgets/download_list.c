#include "ui/widgets/download_list.h"

#include "downloads/item.h"
#include "ui/support/launch.h"
#include "ui/widgets/row.h"

struct _YoinkDownloadList {
  AdwBin      parent_instance;
  GtkListBox *list;
  GHashTable *rows;  /* gid -> YoinkRow (owned by the list box) */
};

G_DEFINE_FINAL_TYPE (YoinkDownloadList, yoink_download_list, ADW_TYPE_BIN)

static void
yoink_download_list_finalize (GObject *object)
{
  g_hash_table_unref (YOINK_DOWNLOAD_LIST (object)->rows);
  G_OBJECT_CLASS (yoink_download_list_parent_class)->finalize (object);
}

static void
yoink_download_list_class_init (YoinkDownloadListClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = yoink_download_list_finalize;
}

static int
sort_rows (GtkListBoxRow *a, GtkListBoxRow *b, gpointer user_data)
{
  int ka = yoink_row_get_sort_key (YOINK_ROW (a));
  int kb = yoink_row_get_sort_key (YOINK_ROW (b));
  return (ka > kb) - (ka < kb);
}

static void
on_row_activated (GtkListBox *list, GtkListBoxRow *row, gpointer user_data)
{
  if (yoink_row_get_state (YOINK_ROW (row)) == YOINK_STATE_COMPLETE)
    yoink_launch_open (GTK_WIDGET (row), yoink_row_get_path (YOINK_ROW (row)));
}

static void
yoink_download_list_init (YoinkDownloadList *self)
{
  self->rows = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  self->list = GTK_LIST_BOX (gtk_list_box_new ());
  gtk_list_box_set_selection_mode (self->list, GTK_SELECTION_NONE);
  gtk_list_box_set_sort_func (self->list, sort_rows, NULL, NULL);
  gtk_widget_add_css_class (GTK_WIDGET (self->list), "boxed-list-separate");
  gtk_widget_set_valign (GTK_WIDGET (self->list), GTK_ALIGN_START);
  gtk_widget_set_margin_top (GTK_WIDGET (self->list), 12);
  gtk_widget_set_margin_bottom (GTK_WIDGET (self->list), 24);
  gtk_widget_set_margin_start (GTK_WIDGET (self->list), 12);
  gtk_widget_set_margin_end (GTK_WIDGET (self->list), 12);
  g_signal_connect (self->list, "row-activated", G_CALLBACK (on_row_activated), NULL);

  GtkWidget *clamp = adw_clamp_new ();
  adw_clamp_set_maximum_size (ADW_CLAMP (clamp), 760);
  adw_clamp_set_child (ADW_CLAMP (clamp), GTK_WIDGET (self->list));

  GtkWidget *scroll = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), clamp);
  adw_bin_set_child (ADW_BIN (self), scroll);
}

YoinkDownloadList *
yoink_download_list_new (void)
{
  return g_object_new (YOINK_TYPE_DOWNLOAD_LIST, NULL);
}

void
yoink_download_list_update (YoinkDownloadList *self, GPtrArray *items)
{
  g_autoptr (GHashTable) seen = g_hash_table_new (g_str_hash, g_str_equal);
  gboolean resort = FALSE;

  for (guint i = 0; i < items->len; i++) {
    const YoinkItem *item = g_ptr_array_index (items, i);
    YoinkRow *row = g_hash_table_lookup (self->rows, item->gid);

    if (!row) {
      row = yoink_row_new (item->gid);
      g_hash_table_insert (self->rows, g_strdup (item->gid), row);
      gtk_list_box_append (self->list, GTK_WIDGET (row));
      resort = TRUE;
    }
    resort |= yoink_row_set_sort_key (row, (int) i);
    yoink_row_update (row, item);
    g_hash_table_add (seen, item->gid);
  }

  GHashTableIter iter;
  const char *gid;
  YoinkRow *row;
  g_hash_table_iter_init (&iter, self->rows);
  while (g_hash_table_iter_next (&iter, (gpointer *) &gid, (gpointer *) &row)) {
    if (g_hash_table_contains (seen, gid))
      continue;
    g_hash_table_iter_remove (&iter);
    gtk_list_box_remove (self->list, GTK_WIDGET (row));
  }

  if (resort)
    gtk_list_box_invalidate_sort (self->list);
}

void
yoink_download_list_clear (YoinkDownloadList *self)
{
  g_hash_table_remove_all (self->rows);
  gtk_list_box_remove_all (self->list);
}

guint
yoink_download_list_get_count (YoinkDownloadList *self)
{
  return g_hash_table_size (self->rows);
}
