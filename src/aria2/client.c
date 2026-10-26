#include "aria2/client.h"

#include <string.h>

#include "aria2/http.h"
#include "util/json.h"

struct _Aria2Client {
  guint16 port;
  char   *token;
};

typedef struct {
  guint16       port;
  char         *body;
  Aria2Callback cb;
  gpointer      user_data;
} Call;

static void
client_clear (gpointer p)
{
  g_free (((Aria2Client *) p)->token);
}

Aria2Client *
aria2_client_new (guint16 port, const char *secret)
{
  Aria2Client *c = g_atomic_rc_box_new0 (Aria2Client);
  c->port = port;
  c->token = g_strdup_printf ("token:%s", secret);
  return c;
}

Aria2Client *
aria2_client_ref (Aria2Client *c)
{
  return g_atomic_rc_box_acquire (c);
}

void
aria2_client_unref (Aria2Client *c)
{
  g_atomic_rc_box_release_full (c, client_clear);
}

static void
call_free (gpointer p)
{
  Call *call = p;
  g_free (call->body);
  g_free (call);
}

static JsonNode *
parse_envelope (GBytes *body, GError **error)
{
  gsize len;
  const char *data = g_bytes_get_data (body, &len);

  g_autoptr (JsonParser) parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, data, len, error))
    return NULL;

  JsonNode *root = json_parser_get_root (parser);
  JsonObject *obj = JSON_NODE_HOLDS_OBJECT (root) ? json_node_get_object (root) : NULL;
  if (!obj) {
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Unexpected response from aria2");
    return NULL;
  }

  JsonObject *err = dy_json_obj (obj, "error");
  if (err) {
    const char *msg = dy_json_str (err, "message");
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, msg ? msg : "aria2 returned an error");
    return NULL;
  }

  JsonNode *result = json_object_get_member (obj, "result");
  return result ? json_node_copy (result) : json_node_new (JSON_NODE_NULL);
}

static void
call_thread (GTask *task, gpointer src, gpointer task_data, GCancellable *cancel)
{
  Call *call = task_data;
  GError *error = NULL;

  g_autoptr (GBytes) body = dy_http_post_json (call->port, "/jsonrpc", call->body, cancel, &error);
  JsonNode *result = body ? parse_envelope (body, &error) : NULL;

  if (result)
    g_task_return_pointer (task, result, (GDestroyNotify) json_node_unref);
  else
    g_task_return_error (task, error);
}

static void
call_done (GObject *src, GAsyncResult *res, gpointer user_data)
{
  Call *call = g_task_get_task_data (G_TASK (res));
  g_autoptr (GError) error = NULL;
  g_autoptr (JsonNode) result = g_task_propagate_pointer (G_TASK (res), &error);

  if (call->cb)
    call->cb (result, error, call->user_data);
}

static JsonArray *
with_token (Aria2Client *c, const char *method, JsonArray *params)
{
  JsonArray *full = json_array_new ();

  /* system.* methods (multicall, listMethods) don't take the token */
  if (g_str_has_prefix (method, "aria2."))
    json_array_add_string_element (full, c->token);

  if (params) {
    guint n = json_array_get_length (params);
    for (guint i = 0; i < n; i++)
      json_array_add_element (full, json_node_copy (json_array_get_element (params, i)));
    json_array_unref (params);
  }
  return full;
}

void
aria2_call (Aria2Client *c, const char *method, JsonArray *params, Aria2Callback cb, gpointer user_data)
{
  g_autoptr (JsonObject) req = json_object_new ();
  json_object_set_string_member (req, "jsonrpc", "2.0");
  json_object_set_string_member (req, "id", "dy");
  json_object_set_string_member (req, "method", method);
  json_object_set_array_member (req, "params", with_token (c, method, params));
  g_autoptr (JsonNode) node = json_node_init_object (json_node_alloc (), req);

  Call *call = g_new0 (Call, 1);
  call->port = c->port;
  call->body = json_to_string (node, FALSE);
  call->cb = cb;
  call->user_data = user_data;

  g_autoptr (GTask) task = g_task_new (NULL, NULL, call_done, NULL);
  g_task_set_task_data (task, call, call_free);
  g_task_run_in_thread (task, call_thread);
}

void
aria2_call_gid (Aria2Client *c, const char *method, const char *gid, Aria2Callback cb, gpointer user_data)
{
  JsonArray *params = json_array_new ();
  json_array_add_string_element (params, gid);
  aria2_call (c, method, params, cb, user_data);
}

JsonNode *
aria2_multicall_entry (Aria2Client *c, const char *method, JsonArray *params)
{
  g_autoptr (JsonObject) call = json_object_new ();
  json_object_set_string_member (call, "methodName", method);
  json_object_set_array_member (call, "params", with_token (c, method, params));
  return json_node_init_object (json_node_alloc (), call);
}

JsonNode *
aria2_multicall_value (JsonArray *results, guint index)
{
  if (!results || index >= json_array_get_length (results))
    return NULL;
  JsonNode *n = json_array_get_element (results, index);
  if (!JSON_NODE_HOLDS_ARRAY (n) || json_array_get_length (json_node_get_array (n)) == 0)
    return NULL;
  return json_array_get_element (json_node_get_array (n), 0);
}
