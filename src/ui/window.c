#include "ui/window.h"

#include "ui/support/drop.h"
#include "ui/support/format.h"
#include "ui/widgets/download_list.h"
#include "ui/widgets/footer.h"
#include "ui/widgets/header.h"
#include "ui/widgets/status_pages.h"

struct _DyWindow {
  AdwApplicationWindow parent_instance;

  AdwToastOverlay *toasts;
  AdwWindowTitle  *title;
  GtkStack        *stack;
  AdwStatusPage   *failed_page;
  DyDownloadList  *list;
  DyFooter        *footer;
};

G_DEFINE_FINAL_TYPE (DyWindow, dy_window, ADW_TYPE_APPLICATION_WINDOW)

static void
dy_window_class_init (DyWindowClass *klass)
{
}

static void
dy_window_init (DyWindow *self)
{
  gtk_window_set_title (GTK_WINDOW (self), "DY");
  gtk_window_set_default_size (GTK_WINDOW (self), 720, 560);

  GtkWidget *header = dy_header_new (&self->title);

  self->stack = GTK_STACK (gtk_stack_new ());
  gtk_stack_set_transition_type (self->stack, GTK_STACK_TRANSITION_TYPE_CROSSFADE);

  self->list = dy_download_list_new ();
  self->failed_page = ADW_STATUS_PAGE (dy_status_page_failed_new ());
  gtk_stack_add_named (self->stack, dy_status_page_starting_new (), "starting");
  gtk_stack_add_named (self->stack, dy_status_page_empty_new (), "empty");
  gtk_stack_add_named (self->stack, GTK_WIDGET (self->list), "list");
  gtk_stack_add_named (self->stack, GTK_WIDGET (self->failed_page), "failed");

  self->footer = dy_footer_new ();

  GtkWidget *view = adw_toolbar_view_new ();
  adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (view), header);
  adw_toolbar_view_set_content (ADW_TOOLBAR_VIEW (view), GTK_WIDGET (self->stack));
  adw_toolbar_view_add_bottom_bar (ADW_TOOLBAR_VIEW (view), GTK_WIDGET (self->footer));

  self->toasts = ADW_TOAST_OVERLAY (adw_toast_overlay_new ());
  adw_toast_overlay_set_child (self->toasts, view);
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), GTK_WIDGET (self->toasts));

  dy_drop_install (GTK_WIDGET (self));
}

DyWindow *
dy_window_new (GtkApplication *app)
{
  return g_object_new (DY_TYPE_WINDOW, "application", app, NULL);
}

void
dy_window_show_starting (DyWindow *self)
{
  gtk_stack_set_visible_child_name (self->stack, "starting");
}

void
dy_window_show_failed (DyWindow *self, const char *title, const char *description)
{
  adw_status_page_set_title (self->failed_page, title);
  adw_status_page_set_description (self->failed_page, description);
  gtk_stack_set_visible_child_name (self->stack, "failed");
  adw_window_title_set_subtitle (self->title, "");
}

void
dy_window_update (DyWindow *self, GPtrArray *items, const DyStats *stats)
{
  dy_download_list_update (self->list, items);
  gtk_stack_set_visible_child_name (self->stack, dy_download_list_get_count (self->list) ? "list" : "empty");

  dy_footer_set_speeds (self->footer, stats->download_speed, stats->upload_speed);

  g_autofree char *subtitle = dy_format_subtitle (stats);
  adw_window_title_set_subtitle (self->title, subtitle);
}

void
dy_window_clear (DyWindow *self)
{
  dy_download_list_clear (self->list);
  dy_footer_set_speeds (self->footer, 0, 0);
  adw_window_title_set_subtitle (self->title, "");
}

void
dy_window_toast (DyWindow *self, const char *message)
{
  AdwToast *t = adw_toast_new (message);
  adw_toast_set_use_markup (t, FALSE);
  adw_toast_overlay_add_toast (self->toasts, t);
}
