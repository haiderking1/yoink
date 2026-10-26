#include "downloads/removal.h"

#include <string.h>

#include "downloads/leftovers.h"
#include "util/json.h"

#define STOP_POLL_MS   100
#define STOP_POLL_MAX  50   /* give aria2 up to 5 s to let go of the files */

typedef struct {
  Aria2Client     *client;
  char            *gid;
  GPtrArray       *leftovers;  /* YoinkLeftovers*, deleted once aria2 is done with them */
  guint            attempts;
  YoinkControlDone done;
  gpointer         user_data;
} Removal;

static Removal *
removal_new (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data)
{
  Removal *r = g_new0 (Removal, 1);
  r->client = aria2_client_ref (c);
  r->gid = g_strdup (gid);
  r->leftovers = g_ptr_array_new_with_free_func ((GDestroyNotify) yoink_leftovers_free);
  r->done = done;
  r->user_data = user_data;
  return r;
}

static void
finish (Removal *r, const char *error)
{
  if (r->done)
    r->done (error, r->user_data);
  aria2_client_unref (r->client);
  g_ptr_array_unref (r->leftovers);
  g_free (r->gid);
  g_free (r);
}

static void
delete_leftovers (Removal *r)
{
  for (guint i = 0; i < r->leftovers->len; i++)
    yoink_leftovers_delete (g_ptr_array_steal_index_fast (r->leftovers, i--));
}

static void
keep_leftover (Removal *r, JsonObject *status)
{
  if (g_strcmp0 (yoink_json_str (status, "status"), "complete") == 0)
    return;
  YoinkLeftovers *l = yoink_leftovers_from_status (status);
  if (l)
    g_ptr_array_add (r->leftovers, l);
}

static JsonArray *
leftover_keys (void)
{
  JsonArray *keys = json_array_new ();
  for (const char *const *k = yoink_leftovers_status_keys (); *k; k++)
    json_array_add_string_element (keys, *k);
  return keys;
}

static void
tell_status (Removal *r, JsonArray *keys, Aria2Callback cb)
{
  JsonArray *params = json_array_new ();
  json_array_add_string_element (params, r->gid);
  json_array_add_array_element (params, keys);
  aria2_call (r->client, "aria2.tellStatus", params, cb, r);
}

static gboolean
is_not_found (const GError *error)
{
  return error && strstr (error->message, "is not found");
}

/* ---------- cancel ---------- */

static void wait_until_stopped (Removal *r);

static void
wait_again (gpointer r)
{
  wait_until_stopped (r);
}

static void
on_stop_check (JsonNode *result, const GError *error, gpointer user_data)
{
  Removal *r = user_data;
  const char *status = NULL;

  /* "not found": the poller already forgot the removed download, so it's stopped */
  if (error && !is_not_found (error)) {
    finish (r, error->message);
    return;
  }
  if (result && JSON_NODE_HOLDS_OBJECT (result))
    status = yoink_json_str (json_node_get_object (result), "status");

  /* aria2 rewrites the control file while it tears a download down, so
   * deleting before it's fully stopped would just leave a fresh one behind */
  if (g_strcmp0 (status, "active") == 0 || g_strcmp0 (status, "waiting") == 0 ||
      g_strcmp0 (status, "paused") == 0) {
    if (++r->attempts > STOP_POLL_MAX)
      finish (r, "aria2 took too long to stop the download; its partial files were kept");
    else
      g_timeout_add_once (STOP_POLL_MS, wait_again, r);
    return;
  }

  /* it may have finished just before the cancel landed: keep the file then */
  if (g_strcmp0 (status, "complete") != 0)
    delete_leftovers (r);
  aria2_call_gid (r->client, "aria2.removeDownloadResult", r->gid, NULL, NULL);
  finish (r, NULL);
}

static void
wait_until_stopped (Removal *r)
{
  JsonArray *keys = json_array_new ();
  json_array_add_string_element (keys, "status");
  tell_status (r, keys, on_stop_check);
}

static void
on_force_removed (JsonNode *result, const GError *error, gpointer user_data)
{
  Removal *r = user_data;
  if (error)
    finish (r, error->message);
  else
    wait_until_stopped (r);
}

static void
on_cancel_status (JsonNode *result, const GError *error, gpointer user_data)
{
  Removal *r = user_data;
  if (error) {
    finish (r, error->message);
    return;
  }

  /* collect the paths now; aria2 forgets them once the download is gone */
  keep_leftover (r, json_node_get_object (result));
  aria2_call_gid (r->client, "aria2.forceRemove", r->gid, on_force_removed, r);
}

void
yoink_removal_cancel (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data)
{
  Removal *r = removal_new (c, gid, done, user_data);
  tell_status (r, leftover_keys (), on_cancel_status);
}

/* ---------- dismiss ---------- */

static void
on_result_removed (JsonNode *result, const GError *error, gpointer user_data)
{
  Removal *r = user_data;
  if (!error)
    delete_leftovers (r);
  finish (r, error ? error->message : NULL);
}

static void
on_dismiss_status (JsonNode *result, const GError *error, gpointer user_data)
{
  Removal *r = user_data;
  if (error) {
    finish (r, error->message);
    return;
  }

  keep_leftover (r, json_node_get_object (result));
  aria2_call_gid (r->client, "aria2.removeDownloadResult", r->gid, on_result_removed, r);
}

void
yoink_removal_dismiss (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data)
{
  Removal *r = removal_new (c, gid, done, user_data);
  tell_status (r, leftover_keys (), on_dismiss_status);
}

/* ---------- clear finished ---------- */

static void
on_stopped_listed (JsonNode *result, const GError *error, gpointer user_data)
{
  Removal *r = user_data;
  if (error) {
    finish (r, error->message);
    return;
  }

  JsonArray *list = JSON_NODE_HOLDS_ARRAY (result) ? json_node_get_array (result) : NULL;
  guint n = list ? json_array_get_length (list) : 0;
  for (guint i = 0; i < n; i++)
    keep_leftover (r, yoink_json_arr_obj (list, i));

  aria2_call (r->client, "aria2.purgeDownloadResult", NULL, on_result_removed, r);
}

void
yoink_removal_clear_finished (Aria2Client *c, YoinkControlDone done, gpointer user_data)
{
  Removal *r = removal_new (c, NULL, done, user_data);

  JsonArray *params = json_array_new ();
  json_array_add_int_element (params, 0);
  json_array_add_int_element (params, 1000);
  json_array_add_array_element (params, leftover_keys ());
  aria2_call (c, "aria2.tellStopped", params, on_stopped_listed, r);
}
