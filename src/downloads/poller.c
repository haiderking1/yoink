#include "downloads/poller.h"

#include "util/json.h"

#define INTERVAL_MS 1000
#define SOON_MS     80

enum { GROUP_ACTIVE, GROUP_WAITING, GROUP_STOPPED, GROUP_STATS };

struct _DyPoller {
  Aria2Client *client;
  DyPollFunc   fn;
  gpointer     user_data;
  guint        timer_id;
  guint        soon_id;
  gboolean     in_flight;
  gboolean     dead;
  int          refs;  /* main thread only: the owner plus a request in flight */
};

static void poll (DyPoller *p);

static void
poller_unref (DyPoller *p)
{
  if (--p->refs > 0)
    return;
  aria2_client_unref (p->client);
  g_free (p);
}

static JsonArray *
status_keys (void)
{
  JsonArray *arr = json_array_new ();
  for (const char *const *k = dy_item_status_keys (); *k; k++)
    json_array_add_string_element (arr, *k);
  return arr;
}

static JsonArray *
range_params (void)
{
  JsonArray *p = json_array_new ();
  json_array_add_int_element (p, 0);
  json_array_add_int_element (p, 1000);
  json_array_add_array_element (p, status_keys ());
  return p;
}

static void
collect_group (DyPoller *p, JsonArray *results, guint group, GPtrArray *items)
{
  JsonNode *node = aria2_multicall_value (results, group);
  JsonArray *list = node && JSON_NODE_HOLDS_ARRAY (node) ? json_node_get_array (node) : NULL;
  guint n = list ? json_array_get_length (list) : 0;

  for (guint i = 0; i < n; i++) {
    /* aria2 lists stopped downloads oldest first; show the newest first */
    guint index = group == GROUP_STOPPED ? n - 1 - i : i;
    DyItem *item = dy_item_from_json (dy_json_arr_obj (list, index));
    if (!item)
      continue;

    /* Removed downloads and finished hand-offs aren't interesting;
     * drop them from aria2's memory too. */
    if (group == GROUP_STOPPED && (item->state == DY_STATE_REMOVED || item->is_handoff)) {
      aria2_call_gid (p->client, "aria2.removeDownloadResult", item->gid, NULL, NULL);
      dy_item_free (item);
      continue;
    }
    g_ptr_array_add (items, item);
  }
}

static void
on_poll (JsonNode *result, const GError *error, gpointer user_data)
{
  DyPoller *p = user_data;
  p->in_flight = FALSE;

  if (p->dead) {
    poller_unref (p);
    return;
  }

  if (error) {
    p->fn (NULL, NULL, error, p->user_data);
    poller_unref (p);
    return;
  }

  JsonArray *results = json_node_get_array (result);
  g_autoptr (GPtrArray) items = g_ptr_array_new_with_free_func ((GDestroyNotify) dy_item_free);
  collect_group (p, results, GROUP_ACTIVE, items);
  collect_group (p, results, GROUP_WAITING, items);
  collect_group (p, results, GROUP_STOPPED, items);

  JsonNode *stat_node = aria2_multicall_value (results, GROUP_STATS);
  JsonObject *stat = stat_node && JSON_NODE_HOLDS_OBJECT (stat_node) ? json_node_get_object (stat_node) : NULL;
  DyStats stats = {
    .active = dy_json_num (stat, "numActive"),
    .waiting = dy_json_num (stat, "numWaiting"),
    .download_speed = dy_json_num (stat, "downloadSpeed"),
    .upload_speed = dy_json_num (stat, "uploadSpeed"),
  };

  p->fn (items, &stats, NULL, p->user_data);
  poller_unref (p);
}

static void
poll (DyPoller *p)
{
  if (p->in_flight || p->dead)
    return;

  JsonArray *active = json_array_new ();
  json_array_add_array_element (active, status_keys ());

  JsonArray *calls = json_array_new ();
  json_array_add_element (calls, aria2_multicall_entry (p->client, "aria2.tellActive", active));
  json_array_add_element (calls, aria2_multicall_entry (p->client, "aria2.tellWaiting", range_params ()));
  json_array_add_element (calls, aria2_multicall_entry (p->client, "aria2.tellStopped", range_params ()));
  json_array_add_element (calls, aria2_multicall_entry (p->client, "aria2.getGlobalStat", NULL));

  JsonArray *params = json_array_new ();
  json_array_add_array_element (params, calls);

  p->in_flight = TRUE;
  p->refs++;
  aria2_call (p->client, "system.multicall", params, on_poll, p);
}

static gboolean
on_tick (gpointer user_data)
{
  poll (user_data);
  return G_SOURCE_CONTINUE;
}

static void
on_soon (gpointer user_data)
{
  DyPoller *p = user_data;
  p->soon_id = 0;
  poll (p);
}

DyPoller *
dy_poller_new (Aria2Client *client, DyPollFunc fn, gpointer user_data)
{
  DyPoller *p = g_new0 (DyPoller, 1);
  p->client = aria2_client_ref (client);
  p->fn = fn;
  p->user_data = user_data;
  p->refs = 1;
  p->timer_id = g_timeout_add (INTERVAL_MS, on_tick, p);
  dy_poller_poll_soon (p);
  return p;
}

void
dy_poller_poll_soon (DyPoller *p)
{
  if (!p->soon_id)
    p->soon_id = g_timeout_add_once (SOON_MS, on_soon, p);
}

void
dy_poller_free (DyPoller *p)
{
  if (!p)
    return;
  p->dead = TRUE;
  g_clear_handle_id (&p->timer_id, g_source_remove);
  g_clear_handle_id (&p->soon_id, g_source_remove);
  poller_unref (p);
}
