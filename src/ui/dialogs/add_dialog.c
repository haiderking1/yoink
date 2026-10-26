#include "ui/dialogs/add_dialog.h"

#include <adwaita.h>

#include "ui/dialogs/torrent_picker.h"
#include "util/links.h"

static void
on_response (AdwAlertDialog *dialog, const char *response, GtkEditable *entry)
{
  GtkWidget *window = GTK_WIDGET (gtk_widget_get_root (GTK_WIDGET (dialog)));

  if (g_str_equal (response, "add"))
    gtk_widget_activate_action (window, "app.add-links", "s", gtk_editable_get_text (entry));
  else if (g_str_equal (response, "torrent"))
    yoink_torrent_picker_present (window);
}

static void
on_entry_changed (GtkEditable *entry, AdwAlertDialog *dialog)
{
  g_autofree char *text = g_strstrip (g_strdup (gtk_editable_get_text (entry)));
  adw_alert_dialog_set_response_enabled (dialog, "add", *text != '\0');
}

static void
on_clipboard_text (GObject *src, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GtkEditable) entry = user_data;
  g_autofree char *text = gdk_clipboard_read_text_finish (GDK_CLIPBOARD (src), res, NULL);

  if (text && yoink_is_link (g_strstrip (text)) && !*gtk_editable_get_text (entry)) {
    gtk_editable_set_text (entry, text);
    gtk_editable_select_region (entry, 0, -1);
  }
}

void
yoink_add_dialog_present (GtkWidget *parent)
{
  AdwDialog *dialog = adw_alert_dialog_new ("Add Download", "Paste one or more links, or a magnet link.");
  AdwAlertDialog *alert = ADW_ALERT_DIALOG (dialog);
  adw_alert_dialog_add_responses (alert,
                                  "cancel", "_Cancel",
                                  "torrent", "_Torrent File…",
                                  "add", "_Add",
                                  NULL);
  adw_alert_dialog_set_response_appearance (alert, "add", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_response_enabled (alert, "add", FALSE);
  adw_alert_dialog_set_default_response (alert, "add");
  adw_alert_dialog_set_close_response (alert, "cancel");

  GtkWidget *list = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (list, "boxed-list");

  GtkWidget *entry = adw_entry_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (entry), "Link");
  adw_entry_row_set_activates_default (ADW_ENTRY_ROW (entry), TRUE);
  gtk_list_box_append (GTK_LIST_BOX (list), entry);
  adw_alert_dialog_set_extra_child (alert, list);

  g_signal_connect (entry, "changed", G_CALLBACK (on_entry_changed), alert);
  g_signal_connect (dialog, "response", G_CALLBACK (on_response), entry);

  gdk_clipboard_read_text_async (gtk_widget_get_clipboard (parent), NULL, on_clipboard_text, g_object_ref (entry));

  adw_dialog_present (dialog, parent);
  gtk_widget_grab_focus (entry);
}
