#include "downloads/leftovers.h"

#include <gio/gio.h>
#include <glib/gstdio.h>
#include <string.h>

#include "util/json.h"

struct _YoinkLeftovers {
  char      *dir;    /* with a trailing slash */
  GPtrArray *files;  /* partial data files */
  char      *root;   /* folder of a multi-file torrent, or NULL */
};

const char *const *
yoink_leftovers_status_keys (void)
{
  static const char *const keys[] = { "gid", "status", "dir", "files", "bittorrent", NULL };
  return keys;
}

static gboolean
is_inside (const char *dir, const char *path)
{
  return g_str_has_prefix (path, dir) && path[strlen (dir)] != '\0' &&
         !strstr (path, "/../") && !g_str_has_suffix (path, "/..");
}

YoinkLeftovers *
yoink_leftovers_from_status (JsonObject *status)
{
  const char *dir = yoink_json_str (status, "dir");
  if (!dir || !*dir)
    return NULL;

  YoinkLeftovers *l = g_new0 (YoinkLeftovers, 1);
  l->dir = g_str_has_suffix (dir, "/") ? g_strdup (dir) : g_strconcat (dir, "/", NULL);
  l->files = g_ptr_array_new_with_free_func (g_free);

  JsonArray *files = yoink_json_arr (status, "files");
  guint n = files ? json_array_get_length (files) : 0;
  for (guint i = 0; i < n; i++) {
    const char *path = yoink_json_str (yoink_json_arr_obj (files, i), "path");
    /* magnet links have a "[METADATA]…" placeholder before the real files */
    if (path && *path && !g_str_has_prefix (path, "[METADATA]") && is_inside (l->dir, path))
      g_ptr_array_add (l->files, g_strdup (path));
  }

  JsonObject *bt = yoink_json_obj (status, "bittorrent");
  const char *name = yoink_json_str (yoink_json_obj (bt, "info"), "name");
  if (name && g_strcmp0 (yoink_json_str (bt, "mode"), "multi") == 0) {
    char *root = g_strconcat (l->dir, name, NULL);
    if (is_inside (l->dir, root))
      l->root = root;
    else
      g_free (root);
  }
  return l;
}

void
yoink_leftovers_free (YoinkLeftovers *l)
{
  if (!l)
    return;
  g_free (l->dir);
  g_ptr_array_unref (l->files);
  g_free (l->root);
  g_free (l);
}

static void
remove_with_control_file (const char *path)
{
  g_autofree char *control = g_strconcat (path, ".aria2", NULL);
  g_unlink (path);
  g_unlink (control);
}

/* Removes now-empty folders from path's parent up to, not including, dir. */
static void
prune_empty_parents (const char *dir, const char *path)
{
  g_autofree char *parent = g_path_get_dirname (path);
  g_autofree char *stop = g_strndup (dir, strlen (dir) - 1);  /* without the slash */

  while (is_inside (dir, parent) && g_strcmp0 (parent, stop) != 0 && g_rmdir (parent) == 0) {
    char *up = g_path_get_dirname (parent);
    g_free (parent);
    parent = up;
  }
}

static void
delete_thread (GTask *task, gpointer src, gpointer task_data, GCancellable *cancel)
{
  YoinkLeftovers *l = task_data;

  for (guint i = 0; i < l->files->len; i++) {
    const char *path = g_ptr_array_index (l->files, i);
    remove_with_control_file (path);
    prune_empty_parents (l->dir, path);
  }

  /* a multi-file torrent keeps one control file next to its folder */
  if (l->root) {
    g_autofree char *control = g_strconcat (l->root, ".aria2", NULL);
    g_unlink (control);
    g_rmdir (l->root);
  }

  g_debug ("cleaned up %u leftover file(s) in %s", l->files->len, l->dir);
  g_task_return_boolean (task, TRUE);
}

void
yoink_leftovers_delete (YoinkLeftovers *l)
{
  if (!l)
    return;
  g_autoptr (GTask) task = g_task_new (NULL, NULL, NULL, NULL);
  g_task_set_task_data (task, l, (GDestroyNotify) yoink_leftovers_free);
  g_task_run_in_thread (task, delete_thread);
}
