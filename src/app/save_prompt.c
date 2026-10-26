#include "app/save_prompt.h"

#include <string.h>

#include "app/settings.h"
#include "ui/dialogs/save_dialog.h"
#include "util/links.h"

#define LISTED_NAMES 3

struct _YoinkSavePrompt {
  GtkApplication        *app;
  GQueue                 queue;    /* YoinkRequest*, waiting for their popup */
  YoinkRequest          *current;  /* the one on screen */
  char                  *guessed;  /* name we made up for current, when it had none */
  YoinkSavePromptAccept  accept;
  gpointer               user_data;
};

static void show_next (YoinkSavePrompt *self);

YoinkSavePrompt *
yoink_save_prompt_new (GtkApplication *app, YoinkSavePromptAccept accept, gpointer user_data)
{
  YoinkSavePrompt *self = g_new0 (YoinkSavePrompt, 1);
  self->app = app;
  self->accept = accept;
  self->user_data = user_data;
  g_queue_init (&self->queue);
  return self;
}

void
yoink_save_prompt_free (YoinkSavePrompt *self)
{
  g_queue_clear_full (&self->queue, (GDestroyNotify) yoink_request_free);
  yoink_request_free (self->current);
  g_free (self->guessed);
  g_free (self);
}

void
yoink_save_prompt_push (YoinkSavePrompt *self, YoinkRequest *request)
{
  g_queue_push_tail (&self->queue, request);
  show_next (self);
}

/* ---------- describing a request ---------- */

static gboolean
is_magnet (const char *uri)
{
  return g_ascii_strncasecmp (uri, "magnet:", 7) == 0;
}

static char *
magnet_name (const char *uri)
{
  const char *query = strchr (uri, '?');
  g_autoptr (GHashTable) params = query ? g_uri_parse_params (query + 1, -1, "&", G_URI_PARAMS_NONE, NULL) : NULL;
  const char *dn = params ? g_hash_table_lookup (params, "dn") : NULL;
  return g_strdup (dn && *dn ? dn : "Magnet link");
}

static char *
item_name (const YoinkRequest *r, guint i)
{
  if (i < r->uris->len) {
    const char *uri = g_ptr_array_index (r->uris, i);
    return is_magnet (uri) ? magnet_name (uri) : yoink_uri_basename (uri);
  }
  return g_file_get_basename (g_ptr_array_index (r->torrents, i - r->uris->len));
}

/* a single plain link is the only thing whose file name we can set */
static gboolean
renamable (const YoinkRequest *r)
{
  return r->uris->len == 1 && r->torrents->len == 0 && !is_magnet (g_ptr_array_index (r->uris, 0));
}

static char *
describe_source (const YoinkRequest *r)
{
  g_autoptr (GUri) u = g_uri_parse (g_ptr_array_index (r->uris, 0), G_URI_FLAGS_NONE, NULL);
  const char *host = u ? g_uri_get_host (u) : NULL;
  return host ? g_strdup_printf ("From %s", host) : NULL;
}

static char *
describe_items (const YoinkRequest *r)
{
  guint n = yoink_request_count (r);
  g_autoptr (GString) s = g_string_new (NULL);

  for (guint i = 0; i < n && i < LISTED_NAMES; i++) {
    g_autofree char *name = item_name (r, i);
    g_string_append_printf (s, "%s%s", i ? "\n" : "", name);
  }
  if (n > LISTED_NAMES)
    g_string_append_printf (s, "\nand %u more", n - LISTED_NAMES);
  return g_string_free (g_steal_pointer (&s), FALSE);
}

/* ---------- the popup ---------- */

static void
on_idle_show_next (gpointer self)
{
  show_next (self);
}

static void
on_answered (const char *dir, const char *name, gpointer user_data)
{
  YoinkSavePrompt *self = user_data;
  g_autoptr (YoinkRequest) request = g_steal_pointer (&self->current);
  g_autofree char *guessed = g_steal_pointer (&self->guessed);

  if (dir) {
    g_autofree char *edited = name ? g_strstrip (g_strdup (name)) : NULL;
    /* a guessed name is only a hint: unless it was changed, let the server's
     * Content-Disposition name the file */
    if (edited && (request->out || g_strcmp0 (edited, guessed) != 0))
      yoink_request_set_out (request, edited, NULL);

    g_set_str (&request->dir, dir);
    yoink_settings_set_save_dir (dir);
    self->accept (g_steal_pointer (&request), self->user_data);
  }

  /* let this popup close before the next one opens */
  g_idle_add_once (on_idle_show_next, self);
}

static GtkWindow *
window_for_popup (YoinkSavePrompt *self)
{
  GtkWindow *window = gtk_application_get_active_window (self->app);
  if (!window) {
    g_application_activate (G_APPLICATION (self->app));
    window = gtk_application_get_active_window (self->app);
  }
  return window;
}

static void
show_next (YoinkSavePrompt *self)
{
  if (self->current || g_queue_is_empty (&self->queue))
    return;

  self->current = g_queue_pop_head (&self->queue);
  YoinkRequest *r = self->current;
  guint n = yoink_request_count (r);
  g_autofree char *dir = yoink_settings_get_save_dir ();

  GtkWindow *window = window_for_popup (self);
  if (!window) {  /* nowhere to ask: save where the last one went */
    on_answered (dir, NULL, self);
    return;
  }
  gtk_window_present (window);

  g_autofree char *heading = n == 1 ? g_strdup ("Save Download") : g_strdup_printf ("Save %u Downloads", n);
  g_autofree char *name = NULL, *body = NULL;
  if (renamable (r)) {
    name = r->out ? g_strdup (r->out) : item_name (r, 0);
    self->guessed = r->out ? NULL : g_strdup (name);
    body = describe_source (r);
  } else {
    body = describe_items (r);
  }

  YoinkSaveDialogInfo info = { .heading = heading, .body = body, .name = name, .dir = dir };
  yoink_save_dialog_present (GTK_WIDGET (window), &info, on_answered, self);
}
