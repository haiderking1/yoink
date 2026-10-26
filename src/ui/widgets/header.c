#include "ui/widgets/header.h"

static GMenuModel *
build_menu (void)
{
  g_autoptr (GMenu) downloads = g_menu_new ();
  g_menu_append (downloads, "_Pause All", "app.pause-all");
  g_menu_append (downloads, "_Resume All", "app.resume-all");
  g_menu_append (downloads, "_Clear Finished", "app.clear-finished");

  g_autoptr (GMenu) misc = g_menu_new ();
  g_menu_append (misc, "_Open Downloads Folder", "app.open-folder");
  g_menu_append (misc, "_About DY", "app.about");
  g_menu_append (misc, "_Quit", "app.quit");

  GMenu *menu = g_menu_new ();
  g_menu_append_section (menu, NULL, G_MENU_MODEL (downloads));
  g_menu_append_section (menu, NULL, G_MENU_MODEL (misc));
  return G_MENU_MODEL (menu);
}

GtkWidget *
dy_header_new (AdwWindowTitle **title_out)
{
  GtkWidget *header = adw_header_bar_new ();

  AdwWindowTitle *title = ADW_WINDOW_TITLE (adw_window_title_new ("Downloads", ""));
  adw_header_bar_set_title_widget (ADW_HEADER_BAR (header), GTK_WIDGET (title));

  GtkWidget *add = gtk_button_new_from_icon_name ("list-add-symbolic");
  gtk_widget_set_tooltip_text (add, "Add Download (Ctrl+N)");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (add), "app.add");
  adw_header_bar_pack_start (ADW_HEADER_BAR (header), add);

  g_autoptr (GMenuModel) menu = build_menu ();
  GtkWidget *menu_btn = gtk_menu_button_new ();
  gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (menu_btn), "open-menu-symbolic");
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (menu_btn), menu);
  gtk_menu_button_set_primary (GTK_MENU_BUTTON (menu_btn), TRUE);
  adw_header_bar_pack_end (ADW_HEADER_BAR (header), menu_btn);

  *title_out = title;
  return header;
}
