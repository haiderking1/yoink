#include "ui/widgets/download_list.h"

#include "downloads/item.h"
#include "ui/support/launch.h"
#include "ui/widgets/row.h"

struct _DyDownloadList {
  AdwBin      parent_instance;
  GtkListBox *list;
  GHashTable *rows;  /* gid -> DyRow (owned by the list box) */
};

G_DEFINE_FINAL_TYPE (DyDownloadList, dy_download_list, ADW_TYPE_BIN)

static void
dy_download_list_finalize (GObject *object)
{
  g_hash_table_unref (DY_DOWNLOAD_LIST (object)->rows);
  G_OBJECT_CLASS (dy_download_list_parent_class)->finalize (object);
}

static void
dy_download_list_class_init (DyDownloadListClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = dy_download_list_finalize;
}

static int
sort_rows (GtkListBoxRow *a, GtkListBoxRow *b, gpointer user_data)
{
  int ka = dy_row_get_sort_key (DY_ROW (a));
  int kb = dy_row_get_sort_key (DY_ROW (b));
  return (ka > kb) - (ka < kb);
}

static void
on_row_activated (GtkListBox *list, GtkListBoxRow *row, gpointer user_data)
{
  if (dy_row_get_state (DY_ROW (row)) == DY_STATE_COMPLETE)
    dy_launch_open (GTK_WIDGET (row), dy_row_get_path (DY_ROW (row)));
}

static void
dy_download_list_init (DyDownloadList *self)
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

DyDownloadList *
dy_download_list_new (void)
{
  return g_object_new (DY_TYPE_DOWNLOAD_LIST, NULL);
}

void
dy_download_list_update (DyDownloadList *self, GPtrArray *items)
{
  g_autoptr (GHashTable) seen = g_hash_table_new (g_str_hash, g_str_equal);
  gboolean resort = FALSE;

  for (guint i = 0; i < items->len; i++) {
    const DyItem *item = g_ptr_array_index (items, i);
    DyRow *row = g_hash_table_lookup (self->rows, item->gid);

    if (!row) {
      row = dy_row_new (item->gid);
      g_hash_table_insert (self->rows, g_strdup (item->gid), row);
      gtk_list_box_append (self->list, GTK_WIDGET (row));
      resort = TRUE;
    }
    resort |= dy_row_set_sort_key (row, (int) i);
    dy_row_update (row, item);
    g_hash_table_add (seen, item->gid);
  }

  GHashTableIter iter;
  const char *gid;
  DyRow *row;
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
dy_download_list_clear (DyDownloadList *self)
{
  g_hash_table_remove_all (self->rows);
  gtk_list_box_remove_all (self->list);
}

guint
dy_download_list_get_count (DyDownloadList *self)
{
  return g_hash_table_size (self->rows);
}
