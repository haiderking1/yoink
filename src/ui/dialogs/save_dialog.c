#include "ui/dialogs/save_dialog.h"

#include <adwaita.h>

#include "downloads/request.h"
#include "ui/support/format.h"

typedef struct {
  char                *dir;
  GtkEditable         *name;  /* NULL when there's no name field */
  AdwActionRow        *folder;
  YoinkSaveDialogDone  done;
  gpointer             user_data;
} SaveDialog;

static void
save_dialog_free (SaveDialog *sd)
{
  g_free (sd->dir);
  g_free (sd);
}

static void
show_dir (SaveDialog *sd)
{
  g_autofree char *shown = yoink_format_path (sd->dir);
  adw_action_row_set_subtitle (sd->folder, shown);
}

static void
on_response (AdwAlertDialog *dialog, const char *response, SaveDialog *sd)
{
  gboolean start = g_str_equal (response, "download");
  sd->done (start ? sd->dir : NULL,
            start && sd->name ? gtk_editable_get_text (sd->name) : NULL,
            sd->user_data);
}

static void
on_name_changed (GtkEditable *name, AdwAlertDialog *dialog)
{
  g_autofree char *text = g_strstrip (g_strdup (gtk_editable_get_text (name)));
  adw_alert_dialog_set_response_enabled (dialog, "download", yoink_request_out_is_valid (text));
}

/* ---------- Browse… ---------- */

static void
on_folder_picked (GObject *src, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (AdwDialog) dialog = user_data;
  g_autoptr (GFile) folder = gtk_file_dialog_select_folder_finish (GTK_FILE_DIALOG (src), res, NULL);
  SaveDialog *sd = g_object_get_data (G_OBJECT (dialog), "save-dialog");
  g_autofree char *path = folder ? g_file_get_path (folder) : NULL;

  if (!path)  /* cancelled */
    return;
  g_set_str (&sd->dir, path);
  show_dir (sd);
}

static void
on_browse (GtkButton *button, AdwDialog *dialog)
{
  SaveDialog *sd = g_object_get_data (G_OBJECT (dialog), "save-dialog");
  g_autoptr (GtkFileDialog) fd = gtk_file_dialog_new ();
  g_autoptr (GFile) current = g_file_new_for_path (sd->dir);

  gtk_file_dialog_set_title (fd, "Save Downloads To");
  gtk_file_dialog_set_accept_label (fd, "_Select");
  gtk_file_dialog_set_initial_folder (fd, current);

  GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (button));
  gtk_file_dialog_select_folder (fd, GTK_IS_WINDOW (root) ? GTK_WINDOW (root) : NULL, NULL,
                                 on_folder_picked, g_object_ref (dialog));
}

/* ---------- building ---------- */

static GtkWidget *
folder_row (SaveDialog *sd, AdwDialog *dialog)
{
  GtkWidget *row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), "Save to");
  gtk_widget_add_css_class (row, "property");

  GtkWidget *browse = gtk_button_new_with_mnemonic ("_Browse…");
  gtk_widget_set_valign (browse, GTK_ALIGN_CENTER);
  g_signal_connect (browse, "clicked", G_CALLBACK (on_browse), dialog);
  adw_action_row_add_suffix (ADW_ACTION_ROW (row), browse);

  sd->folder = ADW_ACTION_ROW (row);
  show_dir (sd);
  return row;
}

static GtkWidget *
name_row (SaveDialog *sd, const char *name, AdwAlertDialog *alert)
{
  GtkWidget *row = adw_entry_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), "File name");
  adw_entry_row_set_activates_default (ADW_ENTRY_ROW (row), TRUE);
  gtk_editable_set_text (GTK_EDITABLE (row), name);
  g_signal_connect (row, "changed", G_CALLBACK (on_name_changed), alert);

  sd->name = GTK_EDITABLE (row);
  return row;
}

void
yoink_save_dialog_present (GtkWidget                 *parent,
                           const YoinkSaveDialogInfo *info,
                           YoinkSaveDialogDone        done,
                           gpointer                   user_data)
{
  AdwDialog *dialog = adw_alert_dialog_new (info->heading, info->body);
  AdwAlertDialog *alert = ADW_ALERT_DIALOG (dialog);
  adw_alert_dialog_add_responses (alert, "cancel", "_Cancel", "download", "_Download", NULL);
  adw_alert_dialog_set_response_appearance (alert, "download", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (alert, "download");
  adw_alert_dialog_set_close_response (alert, "cancel");

  SaveDialog *sd = g_new0 (SaveDialog, 1);
  sd->dir = g_strdup (info->dir);
  sd->done = done;
  sd->user_data = user_data;
  g_object_set_data_full (G_OBJECT (dialog), "save-dialog", sd, (GDestroyNotify) save_dialog_free);

  GtkWidget *list = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (list, "boxed-list");
  if (info->name)
    gtk_list_box_append (GTK_LIST_BOX (list), name_row (sd, info->name, alert));
  gtk_list_box_append (GTK_LIST_BOX (list), folder_row (sd, dialog));
  adw_alert_dialog_set_extra_child (alert, list);

  g_signal_connect (dialog, "response", G_CALLBACK (on_response), sd);
  adw_dialog_present (dialog, parent);
}
