#include "ui/widgets/row.h"

#include "ui/support/format.h"
#include "ui/support/launch.h"

#define GLIDE_MS 900

struct _DyRow {
  GtkListBoxRow parent_instance;

  GtkWidget    *icon;
  GtkWidget    *name;
  GtkWidget    *progress;
  GtkWidget    *status;
  GtkWidget    *toggle_btn;  /* pause / resume / retry */
  GtkWidget    *open_btn;
  GtkWidget    *remove_btn;
  AdwAnimation *glide;

  char         *gid;
  char         *display_name;
  char         *path;
  DyState       state;
  gboolean      has_uri;
  gboolean      has_fraction;
  int           sort_key;
};

G_DEFINE_FINAL_TYPE (DyRow, dy_row, GTK_TYPE_LIST_BOX_ROW)

static void
dy_row_dispose (GObject *object)
{
  DyRow *self = DY_ROW (object);
  g_clear_object (&self->glide);
  G_OBJECT_CLASS (dy_row_parent_class)->dispose (object);
}

static void
dy_row_finalize (GObject *object)
{
  DyRow *self = DY_ROW (object);
  g_free (self->gid);
  g_free (self->display_name);
  g_free (self->path);
  G_OBJECT_CLASS (dy_row_parent_class)->finalize (object);
}

static void
dy_row_class_init (DyRowClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dy_row_dispose;
  G_OBJECT_CLASS (klass)->finalize = dy_row_finalize;
}

static GtkWidget *
icon_button (const char *icon, const char *tooltip)
{
  GtkWidget *b = gtk_button_new_from_icon_name (icon);
  gtk_widget_set_tooltip_text (b, tooltip);
  gtk_widget_set_valign (b, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (b, "flat");
  gtk_widget_add_css_class (b, "circular");
  return b;
}

static GtkWidget *
caption_label (void)
{
  GtkWidget *l = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (l), 0);
  gtk_label_set_ellipsize (GTK_LABEL (l), PANGO_ELLIPSIZE_END);
  gtk_widget_add_css_class (l, "caption");
  gtk_widget_add_css_class (l, "dim-label");
  gtk_widget_add_css_class (l, "numeric");
  return l;
}

static void
on_open_clicked (GtkButton *button, DyRow *self)
{
  dy_launch_show_in_folder (GTK_WIDGET (self), self->path);
}

static void
dy_row_init (DyRow *self)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 14);
  gtk_widget_add_css_class (box, "dl-row");

  self->icon = gtk_image_new ();
  gtk_image_set_pixel_size (GTK_IMAGE (self->icon), 20);
  gtk_widget_set_valign (self->icon, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (self->icon, "dl-icon");
  gtk_box_append (GTK_BOX (box), self->icon);

  GtkWidget *text = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_hexpand (text, TRUE);
  gtk_widget_set_valign (text, GTK_ALIGN_CENTER);

  self->name = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (self->name), 0);
  gtk_label_set_ellipsize (GTK_LABEL (self->name), PANGO_ELLIPSIZE_MIDDLE);
  gtk_widget_add_css_class (self->name, "dl-name");
  gtk_box_append (GTK_BOX (text), self->name);

  self->progress = gtk_progress_bar_new ();
  gtk_widget_add_css_class (self->progress, "dl-progress");
  gtk_box_append (GTK_BOX (text), self->progress);

  self->status = caption_label ();
  gtk_box_append (GTK_BOX (text), self->status);
  gtk_box_append (GTK_BOX (box), text);

  self->toggle_btn = icon_button ("media-playback-pause-symbolic", "Pause");
  self->open_btn = icon_button ("folder-open-symbolic", "Show in Folder");
  self->remove_btn = icon_button ("window-close-symbolic", "Remove");
  gtk_widget_set_visible (self->open_btn, FALSE);
  g_signal_connect (self->open_btn, "clicked", G_CALLBACK (on_open_clicked), self);
  gtk_box_append (GTK_BOX (box), self->toggle_btn);
  gtk_box_append (GTK_BOX (box), self->open_btn);
  gtk_box_append (GTK_BOX (box), self->remove_btn);

  gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (self), box);
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (self), FALSE);

  AdwAnimationTarget *target = adw_property_animation_target_new (G_OBJECT (self->progress), "fraction");
  self->glide = adw_timed_animation_new (self->progress, 0, 0, GLIDE_MS, target);
  adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (self->glide), ADW_EASE_OUT_CUBIC);
}

DyRow *
dy_row_new (const char *gid)
{
  DyRow *self = g_object_new (DY_TYPE_ROW, NULL);
  self->gid = g_strdup (gid);
  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->toggle_btn), "s", gid);
  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->remove_btn), "s", gid);
  return self;
}

static void
set_state_class (GtkWidget *w, DyState state)
{
  for (DyState s = DY_STATE_UNKNOWN; s <= DY_STATE_REMOVED; s++)
    gtk_widget_remove_css_class (w, dy_state_name (s));
  gtk_widget_add_css_class (w, dy_state_name (state));
}

static void
set_fraction (DyRow *self, double frac)
{
  GtkProgressBar *bar = GTK_PROGRESS_BAR (self->progress);
  double cur = gtk_progress_bar_get_fraction (bar);
  double diff = frac > cur ? frac - cur : cur - frac;
  if (self->has_fraction && diff < 0.0005)
    return;

  /* glide forward between polls; jump when going backward or on first show */
  if (!self->has_fraction || frac < cur) {
    adw_animation_pause (self->glide);
    gtk_progress_bar_set_fraction (bar, frac);
    self->has_fraction = TRUE;
    return;
  }
  adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (self->glide), cur);
  adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (self->glide), frac);
  adw_animation_play (self->glide);
}

static void
update_name (DyRow *self, const DyItem *item)
{
  if (g_strcmp0 (item->name, self->display_name) == 0)
    return;

  g_set_str (&self->display_name, item->name);
  gtk_label_set_text (GTK_LABEL (self->name), item->name);
  gtk_widget_set_tooltip_text (self->name, item->name);

  if (item->is_dir) {
    gtk_image_set_from_icon_name (GTK_IMAGE (self->icon), "folder-symbolic");
    return;
  }
  g_autofree char *ctype = g_content_type_guess (item->name, NULL, 0, NULL);
  g_autoptr (GIcon) icon = g_content_type_get_symbolic_icon (ctype);
  gtk_image_set_from_gicon (GTK_IMAGE (self->icon), icon);
}

static void
update_buttons (DyRow *self)
{
  gboolean complete = self->state == DY_STATE_COMPLETE;
  gboolean paused = self->state == DY_STATE_PAUSED;
  gboolean error = self->state == DY_STATE_ERROR;

  gtk_button_set_icon_name (GTK_BUTTON (self->toggle_btn),
                            paused ? "media-playback-start-symbolic" :
                            error  ? "view-refresh-symbolic" :
                                     "media-playback-pause-symbolic");
  gtk_widget_set_tooltip_text (self->toggle_btn, paused ? "Resume" : error ? "Retry" : "Pause");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (self->toggle_btn),
                                  paused ? "app.resume" : error ? "app.retry" : "app.pause");
  gtk_widget_set_visible (self->toggle_btn, !complete && !(error && !self->has_uri));

  gtk_actionable_set_action_name (GTK_ACTIONABLE (self->remove_btn),
                                  dy_state_is_stopped (self->state) ? "app.dismiss" : "app.cancel");

  gtk_widget_set_visible (self->open_btn, complete);
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (self), complete);
}

void
dy_row_update (DyRow *self, const DyItem *item)
{
  update_name (self, item);
  g_set_str (&self->path, item->path);
  self->has_uri = item->uri != NULL;

  if (item->state != self->state) {
    self->state = item->state;
    set_state_class (self->icon, item->state);
    set_state_class (self->progress, item->state);
    update_buttons (self);
  }

  double frac = item->total > 0 ? (double) item->done / item->total : item->state == DY_STATE_COMPLETE;
  set_fraction (self, frac);

  g_autofree char *status = dy_format_status (item);
  gtk_label_set_text (GTK_LABEL (self->status), status);
}

const char *
dy_row_get_gid (DyRow *self)
{
  return self->gid;
}

const char *
dy_row_get_path (DyRow *self)
{
  return self->path;
}

DyState
dy_row_get_state (DyRow *self)
{
  return self->state;
}

int
dy_row_get_sort_key (DyRow *self)
{
  return self->sort_key;
}

gboolean
dy_row_set_sort_key (DyRow *self, int key)
{
  if (self->sort_key == key)
    return FALSE;
  self->sort_key = key;
  return TRUE;
}
