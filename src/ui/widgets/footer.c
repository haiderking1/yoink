#include "ui/widgets/footer.h"

#include "ui/support/format.h"

struct _DyFooter {
  GtkBox    parent_instance;
  GtkLabel *down;
  GtkLabel *up;
};

G_DEFINE_FINAL_TYPE (DyFooter, dy_footer, GTK_TYPE_BOX)

static void
dy_footer_class_init (DyFooterClass *klass)
{
}

static GtkLabel *
speed_label (void)
{
  GtkWidget *l = gtk_label_new ("0 B/s");
  gtk_widget_add_css_class (l, "numeric");
  return GTK_LABEL (l);
}

static void
dy_footer_init (DyFooter *self)
{
  gtk_box_set_spacing (GTK_BOX (self), 6);
  gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (GTK_WIDGET (self), "dy-footer");

  self->down = speed_label ();
  self->up = speed_label ();

  GtkWidget *gap = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_size_request (gap, 18, -1);

  gtk_box_append (GTK_BOX (self), gtk_image_new_from_icon_name ("go-down-symbolic"));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->down));
  gtk_box_append (GTK_BOX (self), gap);
  gtk_box_append (GTK_BOX (self), gtk_image_new_from_icon_name ("go-up-symbolic"));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->up));
}

DyFooter *
dy_footer_new (void)
{
  return g_object_new (DY_TYPE_FOOTER, "orientation", GTK_ORIENTATION_HORIZONTAL, NULL);
}

void
dy_footer_set_speeds (DyFooter *self, gint64 download, gint64 upload)
{
  g_autofree char *down = dy_format_speed (download);
  g_autofree char *up = dy_format_speed (upload);
  gtk_label_set_text (self->down, down);
  gtk_label_set_text (self->up, up);
}
