#include "downloads/item.h"

#include "util/json.h"
#include "util/links.h"

static const char *state_names[] = {
  [DY_STATE_UNKNOWN]  = "unknown",
  [DY_STATE_ACTIVE]   = "active",
  [DY_STATE_WAITING]  = "waiting",
  [DY_STATE_PAUSED]   = "paused",
  [DY_STATE_ERROR]    = "error",
  [DY_STATE_COMPLETE] = "complete",
  [DY_STATE_REMOVED]  = "removed",
};

DyState
dy_state_parse (const char *s)
{
  for (guint i = 1; s && i < G_N_ELEMENTS (state_names); i++)
    if (g_str_equal (s, state_names[i]))
      return i;
  return DY_STATE_UNKNOWN;
}

const char *
dy_state_name (DyState state)
{
  return state < G_N_ELEMENTS (state_names) ? state_names[state] : state_names[DY_STATE_UNKNOWN];
}

gboolean
dy_state_is_stopped (DyState state)
{
  return state == DY_STATE_COMPLETE || state == DY_STATE_ERROR || state == DY_STATE_REMOVED;
}

const char *const *
dy_item_status_keys (void)
{
  static const char *const keys[] = {
    "gid", "status", "totalLength", "completedLength", "downloadSpeed", "uploadSpeed",
    "errorMessage", "dir", "files", "bittorrent", "followedBy", "seeder",
    NULL,
  };
  return keys;
}

/* Works out what to call a download and where it lives on disk. */
static void
resolve_names (DyItem *item, JsonObject *o)
{
  JsonObject *bt = dy_json_obj (o, "bittorrent");
  JsonObject *info = dy_json_obj (bt, "info");
  JsonObject *f0 = dy_json_arr_obj (dy_json_arr (o, "files"), 0);
  JsonObject *u0 = dy_json_arr_obj (dy_json_arr (f0, "uris"), 0);
  const char *p0 = dy_json_str (f0, "path");

  item->uri = g_strdup (dy_json_str (u0, "uri"));

  if (dy_json_str (info, "name")) {
    item->name = g_strdup (dy_json_str (info, "name"));
    item->is_dir = g_strcmp0 (dy_json_str (bt, "mode"), "multi") == 0;
    item->path = item->is_dir && item->dir ? g_build_filename (item->dir, item->name, NULL) : g_strdup (p0);
  } else if (p0 && g_str_has_prefix (p0, "[METADATA]")) {
    item->name = g_strdup ("Fetching torrent metadata…");
  } else if (p0 && *p0) {
    item->name = g_path_get_basename (p0);
    item->path = g_strdup (p0);
  } else if (item->uri) {
    item->name = dy_uri_basename (item->uri);
  } else {
    item->name = g_strdup ("Unknown");
  }
}

DyItem *
dy_item_from_json (JsonObject *o)
{
  const char *gid = dy_json_str (o, "gid");
  const char *status = dy_json_str (o, "status");
  if (!gid || !status)
    return NULL;

  DyItem *item = g_new0 (DyItem, 1);
  item->gid = g_strdup (gid);
  item->state = dy_state_parse (status);
  item->total = dy_json_num (o, "totalLength");
  item->done = dy_json_num (o, "completedLength");
  item->download_speed = dy_json_num (o, "downloadSpeed");
  item->upload_speed = dy_json_num (o, "uploadSpeed");
  item->dir = g_strdup (dy_json_str (o, "dir"));
  item->error_message = g_strdup (dy_json_str (o, "errorMessage"));

  JsonArray *followed = dy_json_arr (o, "followedBy");
  item->is_handoff = followed && json_array_get_length (followed) > 0;

  gboolean torrent = dy_json_obj (o, "bittorrent") != NULL;
  item->seeding = item->state == DY_STATE_ACTIVE &&
                  (g_strcmp0 (dy_json_str (o, "seeder"), "true") == 0 ||
                   (torrent && item->total > 0 && item->done >= item->total));

  resolve_names (item, o);
  return item;
}

void
dy_item_free (DyItem *item)
{
  if (!item)
    return;
  g_free (item->gid);
  g_free (item->name);
  g_free (item->path);
  g_free (item->uri);
  g_free (item->dir);
  g_free (item->error_message);
  g_free (item);
}
