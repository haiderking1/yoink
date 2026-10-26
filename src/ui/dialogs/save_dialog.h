#pragma once

#include <gtk/gtk.h>

/* What the "where to save" popup shows. */
typedef struct {
  const char *heading;  /* "Save Download", "Save 3 Downloads" */
  const char *body;     /* what's coming, e.g. where it's from or the torrent names */
  const char *name;     /* editable file name, or NULL when there's nothing to rename */
  const char *dir;      /* folder to start from */
} YoinkSaveDialogInfo;

/* dir is NULL when the user cancelled. name is the (maybe edited) file name,
 * NULL if the popup had no name field. */
typedef void (*YoinkSaveDialogDone) (const char *dir, const char *name, gpointer user_data);

/* Asks where to save, with a Browse… button to pick another folder. */
void yoink_save_dialog_present (GtkWidget                 *parent,
                                const YoinkSaveDialogInfo *info,
                                YoinkSaveDialogDone        done,
                                gpointer                   user_data);
