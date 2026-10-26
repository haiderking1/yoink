#include "ui/support/drop.h"

#include "util/links.h"

static void
drop_files (GtkWidget *target, GSList *files)
{
  g_autoptr (GString) links = g_string_new (NULL);

  for (GSList *l = files; l; l = l->next) {
    GFile *file = l->data;
    g_autofree char *uri = g_file_get_uri (file);

    if (g_file_has_uri_scheme (file, "file") && g_str_has_suffix (uri, ".torrent"))
      gtk_widget_activate_action (target, "app.add-torrent", "s", uri);
    else if (yoink_is_link (uri))
      g_string_append_printf (links, "%s\n", uri);
  }

  if (links->len)
    gtk_widget_activate_action (target, "app.add-links", "s", links->str);
}

static gboolean
on_drop (GtkDropTarget *drop, const GValue *value, double x, double y, gpointer user_data)
{
  GtkWidget *target = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (drop));

  if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)) {
    drop_files (target, g_value_get_boxed (value));
    return TRUE;
  }
  if (G_VALUE_HOLDS (value, G_TYPE_STRING)) {
    gtk_widget_activate_action (target, "app.add-links", "s", g_value_get_string (value));
    return TRUE;
  }
  return FALSE;
}

void
yoink_drop_install (GtkWidget *target)
{
  GtkDropTarget *drop = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
  gtk_drop_target_set_gtypes (drop, (GType[]) { GDK_TYPE_FILE_LIST, G_TYPE_STRING }, 2);
  g_signal_connect (drop, "drop", G_CALLBACK (on_drop), NULL);
  gtk_widget_add_controller (target, GTK_EVENT_CONTROLLER (drop));
}
