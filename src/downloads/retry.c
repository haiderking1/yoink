#include "downloads/retry.h"

#include "util/json.h"

typedef struct {
  Aria2Client  *client;
  char         *gid;
  DyControlDone done;
  gpointer      user_data;
} Retry;

/* Options worth carrying over to the new download. */
static const char *carried_options[] = {
  "dir", "out", "referer", "user-agent", "split", "max-connection-per-server", "min-split-size",
};

static void
finish (Retry *r, const char *error)
{
  if (r->done)
    r->done (error, r->user_data);
  aria2_client_unref (r->client);
  g_free (r->gid);
  g_free (r);
}

static void
on_added (JsonNode *result, const GError *error, gpointer user_data)
{
  Retry *r = user_data;
  if (!error)
    aria2_call_gid (r->client, "aria2.removeDownloadResult", r->gid, NULL, NULL);
  finish (r, error ? error->message : NULL);
}

static JsonArray *
collect_uris (JsonObject *status)
{
  JsonArray *uris = json_array_new ();
  g_autoptr (GHashTable) seen = g_hash_table_new (g_str_hash, g_str_equal);
  JsonArray *list = dy_json_arr (dy_json_arr_obj (dy_json_arr (status, "files"), 0), "uris");
  guint n = list ? json_array_get_length (list) : 0;

  for (guint i = 0; i < n; i++) {
    const char *uri = dy_json_str (dy_json_arr_obj (list, i), "uri");
    if (uri && g_hash_table_add (seen, (gpointer) uri))
      json_array_add_string_element (uris, uri);
  }
  return uris;
}

static JsonObject *
collect_options (JsonObject *status, JsonObject *current)
{
  JsonObject *opts = json_object_new ();

  for (guint i = 0; i < G_N_ELEMENTS (carried_options); i++) {
    const char *v = dy_json_str (current, carried_options[i]);
    if (v)
      json_object_set_string_member (opts, carried_options[i], v);
  }
  if (!json_object_has_member (opts, "dir") && dy_json_str (status, "dir"))
    json_object_set_string_member (opts, "dir", dy_json_str (status, "dir"));

  /* getOption joins repeated headers with newlines; addUri wants an array */
  const char *headers = dy_json_str (current, "header");
  if (headers && *headers) {
    g_auto (GStrv) lines = g_strsplit (headers, "\n", -1);
    JsonArray *arr = json_array_new ();
    for (char **l = lines; *l; l++)
      if (**l)
        json_array_add_string_element (arr, *l);
    json_object_set_array_member (opts, "header", arr);
  }
  return opts;
}

static JsonObject *
value_object (JsonArray *results, guint index)
{
  JsonNode *n = aria2_multicall_value (results, index);
  return n && JSON_NODE_HOLDS_OBJECT (n) ? json_node_get_object (n) : NULL;
}

static void
on_info (JsonNode *result, const GError *error, gpointer user_data)
{
  Retry *r = user_data;
  if (error) {
    finish (r, error->message);
    return;
  }

  JsonArray *results = json_node_get_array (result);
  JsonObject *status = value_object (results, 0);
  JsonObject *current = value_object (results, 1);

  JsonArray *uris = collect_uris (status);
  if (json_array_get_length (uris) == 0) {
    json_array_unref (uris);
    finish (r, "This download has no link to retry");
    return;
  }

  JsonArray *params = json_array_new ();
  json_array_add_array_element (params, uris);
  json_array_add_object_element (params, collect_options (status, current));
  aria2_call (r->client, "aria2.addUri", params, on_added, r);
}

void
dy_retry (Aria2Client *c, const char *gid, DyControlDone done, gpointer user_data)
{
  Retry *r = g_new0 (Retry, 1);
  r->client = aria2_client_ref (c);
  r->gid = g_strdup (gid);
  r->done = done;
  r->user_data = user_data;

  JsonArray *keys = json_array_new ();
  json_array_add_string_element (keys, "files");
  json_array_add_string_element (keys, "dir");
  JsonArray *status_params = json_array_new ();
  json_array_add_string_element (status_params, gid);
  json_array_add_array_element (status_params, keys);

  JsonArray *option_params = json_array_new ();
  json_array_add_string_element (option_params, gid);

  JsonArray *calls = json_array_new ();
  json_array_add_element (calls, aria2_multicall_entry (c, "aria2.tellStatus", status_params));
  json_array_add_element (calls, aria2_multicall_entry (c, "aria2.getOption", option_params));
  JsonArray *params = json_array_new ();
  json_array_add_array_element (params, calls);

  aria2_call (c, "system.multicall", params, on_info, r);
}
