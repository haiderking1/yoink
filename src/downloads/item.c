#include "downloads/item.h"

#include "util/json.h"
#include "util/links.h"

static const char *state_names[] = {
  [YOINK_STATE_UNKNOWN]  = "unknown",
  [YOINK_STATE_ACTIVE]   = "active",
  [YOINK_STATE_WAITING]  = "waiting",
  [YOINK_STATE_PAUSED]   = "paused",
  [YOINK_STATE_ERROR]    = "error",
  [YOINK_STATE_COMPLETE] = "complete",
  [YOINK_STATE_REMOVED]  = "removed",
};

YoinkState
yoink_state_parse (const char *s)
{
  for (guint i = 1; s && i < G_N_ELEMENTS (state_names); i++)
    if (g_str_equal (s, state_names[i]))
      return i;
  return YOINK_STATE_UNKNOWN;
}

const char *
yoink_state_name (YoinkState state)
{
  return state < G_N_ELEMENTS (state_names) ? state_names[state] : state_names[YOINK_STATE_UNKNOWN];
}

gboolean
yoink_state_is_stopped (YoinkState state)
{
  return state == YOINK_STATE_COMPLETE || state == YOINK_STATE_ERROR || state == YOINK_STATE_REMOVED;
}

const char *const *
yoink_item_status_keys (void)
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
resolve_names (YoinkItem *item, JsonObject *o)
{
  JsonObject *bt = yoink_json_obj (o, "bittorrent");
  JsonObject *info = yoink_json_obj (bt, "info");
  JsonObject *f0 = yoink_json_arr_obj (yoink_json_arr (o, "files"), 0);
  JsonObject *u0 = yoink_json_arr_obj (yoink_json_arr (f0, "uris"), 0);
  const char *p0 = yoink_json_str (f0, "path");

  item->uri = g_strdup (yoink_json_str (u0, "uri"));

  if (yoink_json_str (info, "name")) {
    item->name = g_strdup (yoink_json_str (info, "name"));
    item->is_dir = g_strcmp0 (yoink_json_str (bt, "mode"), "multi") == 0;
    item->path = item->is_dir && item->dir ? g_build_filename (item->dir, item->name, NULL) : g_strdup (p0);
  } else if (p0 && g_str_has_prefix (p0, "[METADATA]")) {
    item->name = g_strdup ("Fetching torrent metadata…");
  } else if (p0 && *p0) {
    item->name = g_path_get_basename (p0);
    item->path = g_strdup (p0);
  } else if (item->uri) {
    item->name = yoink_uri_basename (item->uri);
  } else {
    item->name = g_strdup ("Unknown");
  }
}

YoinkItem *
yoink_item_from_json (JsonObject *o)
{
  const char *gid = yoink_json_str (o, "gid");
  const char *status = yoink_json_str (o, "status");
  if (!gid || !status)
    return NULL;

  YoinkItem *item = g_new0 (YoinkItem, 1);
  item->gid = g_strdup (gid);
  item->state = yoink_state_parse (status);
  item->total = yoink_json_num (o, "totalLength");
  item->done = yoink_json_num (o, "completedLength");
  item->download_speed = yoink_json_num (o, "downloadSpeed");
  item->upload_speed = yoink_json_num (o, "uploadSpeed");
  item->dir = g_strdup (yoink_json_str (o, "dir"));
  item->error_message = g_strdup (yoink_json_str (o, "errorMessage"));

  JsonArray *followed = yoink_json_arr (o, "followedBy");
  item->is_handoff = followed && json_array_get_length (followed) > 0;

  gboolean torrent = yoink_json_obj (o, "bittorrent") != NULL;
  item->seeding = item->state == YOINK_STATE_ACTIVE &&
                  (g_strcmp0 (yoink_json_str (o, "seeder"), "true") == 0 ||
                   (torrent && item->total > 0 && item->done >= item->total));

  resolve_names (item, o);
  return item;
}

void
yoink_item_free (YoinkItem *item)
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
