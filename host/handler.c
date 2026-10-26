#include "handler.h"

#include "launch.h"

#define HOST_VERSION "0.2.0"

static JsonNode *
reply (gboolean ok, const char *error)
{
  g_autoptr (JsonObject) o = json_object_new ();
  json_object_set_boolean_member (o, "ok", ok);
  if (error)
    json_object_set_string_member (o, "error", error);
  else
    json_object_set_string_member (o, "version", HOST_VERSION);
  return json_node_init_object (json_node_alloc (), o);
}

static JsonNode *
handle_add (JsonObject *msg)
{
  JsonNode *request = json_object_get_member (msg, "request");
  if (!request || !JSON_NODE_HOLDS_OBJECT (request))
    return reply (FALSE, "Missing request");

  /* DY validates the request itself; we only carry it over */
  g_autofree char *json = json_to_string (request, FALSE);
  g_autoptr (GError) error = NULL;
  if (!dy_host_launch (json, &error))
    return reply (FALSE, error->message);
  return reply (TRUE, NULL);
}

JsonNode *
dy_host_handle (JsonNode *message)
{
  JsonObject *msg = JSON_NODE_HOLDS_OBJECT (message) ? json_node_get_object (message) : NULL;
  const char *type = msg ? json_object_get_string_member_with_default (msg, "type", NULL) : NULL;

  if (g_strcmp0 (type, "ping") == 0)
    return reply (TRUE, NULL);
  if (g_strcmp0 (type, "add") == 0)
    return handle_add (msg);
  return reply (FALSE, "Unknown message type");
}
