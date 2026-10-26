#include "downloads/add.h"

typedef struct {
  Aria2Client *client;
  guint        pending;
  guint        added;
  char        *error;
  YoinkAddDone done;
  gpointer     user_data;
} Batch;

typedef struct {
  Batch      *batch;
  JsonObject *options;
} TorrentLoad;

static Batch *
batch_new (Aria2Client *client, guint pending, YoinkAddDone done, gpointer user_data)
{
  Batch *b = g_new0 (Batch, 1);
  b->client = aria2_client_ref (client);
  b->pending = pending;
  b->done = done;
  b->user_data = user_data;
  return b;
}

static void
batch_finish_one (Batch *b, const GError *error)
{
  if (error && !b->error)
    b->error = g_strdup (error->message);
  else if (!error)
    b->added++;

  if (--b->pending > 0)
    return;

  if (b->done)
    b->done (b->added, b->error, b->user_data);
  aria2_client_unref (b->client);
  g_free (b->error);
  g_free (b);
}

static void
on_added (JsonNode *result, const GError *error, gpointer user_data)
{
  if (result && JSON_NODE_HOLDS_VALUE (result))
    g_debug ("added download %s", json_node_get_string (result));
  batch_finish_one (user_data, error);
}

static void
on_torrent_loaded (GObject *src, GAsyncResult *res, gpointer user_data)
{
  TorrentLoad *tl = user_data;
  g_autoptr (GError) error = NULL;
  g_autofree char *contents = NULL;
  gsize len = 0;

  if (g_file_load_contents_finish (G_FILE (src), res, &contents, &len, NULL, &error)) {
    g_autofree char *b64 = g_base64_encode ((const guchar *) contents, len);
    JsonArray *params = json_array_new ();
    json_array_add_string_element (params, b64);
    json_array_add_array_element (params, json_array_new ());  /* no web seeds */
    json_array_add_object_element (params, g_steal_pointer (&tl->options));
    aria2_call (tl->batch->client, "aria2.addTorrent", params, on_added, tl->batch);
  } else {
    batch_finish_one (tl->batch, error);
  }

  g_clear_pointer (&tl->options, json_object_unref);
  g_free (tl);
}

static void
add_links (Batch *b, const YoinkRequest *request)
{
  g_autoptr (JsonObject) options = yoink_request_to_options (request);

  /* one addUri per link: several uris in one call means mirrors of the same file */
  for (guint i = 0; i < request->uris->len; i++) {
    JsonArray *uris = json_array_new ();
    json_array_add_string_element (uris, g_ptr_array_index (request->uris, i));

    JsonArray *params = json_array_new ();
    json_array_add_array_element (params, uris);
    json_array_add_object_element (params, json_object_ref (options));
    aria2_call (b->client, "aria2.addUri", params, on_added, b);
  }
}

static void
add_torrents (Batch *b, const YoinkRequest *request)
{
  for (guint i = 0; i < request->torrents->len; i++) {
    TorrentLoad *tl = g_new0 (TorrentLoad, 1);
    tl->batch = b;
    tl->options = yoink_request_torrent_options (request);
    g_file_load_contents_async (g_ptr_array_index (request->torrents, i), NULL, on_torrent_loaded, tl);
  }
}

void
yoink_add_request (Aria2Client *client, const YoinkRequest *request, YoinkAddDone done, gpointer user_data)
{
  guint n = yoink_request_count (request);
  if (n == 0) {
    if (done)
      done (0, NULL, user_data);
    return;
  }

  Batch *b = batch_new (client, n, done, user_data);
  add_links (b, request);
  add_torrents (b, request);
}
