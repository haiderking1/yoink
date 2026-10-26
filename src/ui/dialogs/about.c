#include "ui/dialogs/about.h"

#include <adwaita.h>

#include "app/version.h"

void
dy_about_present (GtkWidget *parent)
{
  AdwDialog *about = adw_about_dialog_new ();
  adw_about_dialog_set_application_name (ADW_ABOUT_DIALOG (about), "DY");
  adw_about_dialog_set_application_icon (ADW_ABOUT_DIALOG (about), DY_APP_ID);
  adw_about_dialog_set_comments (ADW_ABOUT_DIALOG (about), "A small download manager powered by aria2.");
  adw_about_dialog_set_version (ADW_ABOUT_DIALOG (about), DY_VERSION);
  adw_dialog_present (about, parent);
}
