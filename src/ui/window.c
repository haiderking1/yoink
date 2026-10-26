#include "ui/window.h"

#include "ui/support/drop.h"
#include "ui/support/format.h"
#include "ui/widgets/download_list.h"
#include "ui/widgets/footer.h"
#include "ui/widgets/header.h"
#include "ui/widgets/status_pages.h"

struct _YoinkWindow {
  AdwApplicationWindow parent_instance;

  AdwToastOverlay   *toasts;
  AdwWindowTitle    *title;
  GtkStack          *stack;
  AdwStatusPage     *failed_page;
  YoinkDownloadList *list;
  YoinkFooter       *footer;
};

G_DEFINE_FINAL_TYPE (YoinkWindow, yoink_window, ADW_TYPE_APPLICATION_WINDOW)

static void
yoink_window_class_init (YoinkWindowClass *klass)
{
}

static void
yoink_window_init (YoinkWindow *self)
{
  gtk_window_set_title (GTK_WINDOW (self), "Yoink");
  gtk_window_set_default_size (GTK_WINDOW (self), 720, 560);

  GtkWidget *header = yoink_header_new (&self->title);

  self->stack = GTK_STACK (gtk_stack_new ());
  gtk_stack_set_transition_type (self->stack, GTK_STACK_TRANSITION_TYPE_CROSSFADE);

  self->list = yoink_download_list_new ();
  self->failed_page = ADW_STATUS_PAGE (yoink_status_page_failed_new ());
  gtk_stack_add_named (self->stack, yoink_status_page_starting_new (), "starting");
  gtk_stack_add_named (self->stack, yoink_status_page_empty_new (), "empty");
  gtk_stack_add_named (self->stack, GTK_WIDGET (self->list), "list");
  gtk_stack_add_named (self->stack, GTK_WIDGET (self->failed_page), "failed");

  self->footer = yoink_footer_new ();

  GtkWidget *view = adw_toolbar_view_new ();
  adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (view), header);
  adw_toolbar_view_set_content (ADW_TOOLBAR_VIEW (view), GTK_WIDGET (self->stack));
  adw_toolbar_view_add_bottom_bar (ADW_TOOLBAR_VIEW (view), GTK_WIDGET (self->footer));

  self->toasts = ADW_TOAST_OVERLAY (adw_toast_overlay_new ());
  adw_toast_overlay_set_child (self->toasts, view);
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), GTK_WIDGET (self->toasts));

  yoink_drop_install (GTK_WIDGET (self));
}

YoinkWindow *
yoink_window_new (GtkApplication *app)
{
  return g_object_new (YOINK_TYPE_WINDOW, "application", app, NULL);
}

void
yoink_window_show_starting (YoinkWindow *self)
{
  gtk_stack_set_visible_child_name (self->stack, "starting");
}

void
yoink_window_show_failed (YoinkWindow *self, const char *title, const char *description)
{
  adw_status_page_set_title (self->failed_page, title);
  adw_status_page_set_description (self->failed_page, description);
  gtk_stack_set_visible_child_name (self->stack, "failed");
  adw_window_title_set_subtitle (self->title, "");
}

void
yoink_window_update (YoinkWindow *self, GPtrArray *items, const YoinkStats *stats)
{
  yoink_download_list_update (self->list, items);
  gtk_stack_set_visible_child_name (self->stack, yoink_download_list_get_count (self->list) ? "list" : "empty");

  yoink_footer_set_speeds (self->footer, stats->download_speed, stats->upload_speed);

  g_autofree char *subtitle = yoink_format_subtitle (stats);
  adw_window_title_set_subtitle (self->title, subtitle);
}

void
yoink_window_clear (YoinkWindow *self)
{
  yoink_download_list_clear (self->list);
  yoink_footer_set_speeds (self->footer, 0, 0);
  adw_window_title_set_subtitle (self->title, "");
}

void
yoink_window_toast (YoinkWindow *self, const char *message)
{
  AdwToast *t = adw_toast_new (message);
  adw_toast_set_use_markup (t, FALSE);
  adw_toast_overlay_add_toast (self->toasts, t);
}
