#include "ui/widgets/status_pages.h"

static GtkWidget *
page (const char *icon, const char *title, const char *description)
{
  GtkWidget *p = adw_status_page_new ();
  if (icon)
    adw_status_page_set_icon_name (ADW_STATUS_PAGE (p), icon);
  adw_status_page_set_title (ADW_STATUS_PAGE (p), title);
  adw_status_page_set_description (ADW_STATUS_PAGE (p), description);
  return p;
}

static GtkWidget *
pill_button (const char *label, const char *action)
{
  GtkWidget *b = gtk_button_new_with_mnemonic (label);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (b), action);
  gtk_widget_set_halign (b, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (b, "pill");
  gtk_widget_add_css_class (b, "suggested-action");
  return b;
}

GtkWidget *
yoink_status_page_starting_new (void)
{
  GtkWidget *p = page (NULL, "Starting aria2…", NULL);
  GtkWidget *spinner = adw_spinner_new ();
  gtk_widget_set_size_request (spinner, 48, 48);
  adw_status_page_set_child (ADW_STATUS_PAGE (p), spinner);
  return p;
}

GtkWidget *
yoink_status_page_empty_new (void)
{
  GtkWidget *p = page ("folder-download-symbolic", "No Downloads",
                       "Add a link with the + button, or drop one onto this window");
  adw_status_page_set_child (ADW_STATUS_PAGE (p), pill_button ("_Add Download", "app.add"));
  return p;
}

GtkWidget *
yoink_status_page_failed_new (void)
{
  GtkWidget *p = page ("dialog-warning-symbolic", "", NULL);
  adw_status_page_set_child (ADW_STATUS_PAGE (p), pill_button ("_Try Again", "app.restart"));
  return p;
}
