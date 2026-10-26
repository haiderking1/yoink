#include "util/json.h"

static JsonNode *
member (JsonObject *o, const char *key)
{
  return o ? json_object_get_member (o, key) : NULL;
}

const char *
yoink_json_str (JsonObject *o, const char *key)
{
  JsonNode *n = member (o, key);
  if (!n || !JSON_NODE_HOLDS_VALUE (n) || json_node_get_value_type (n) != G_TYPE_STRING)
    return NULL;
  return json_node_get_string (n);
}

/* aria2 sends every number as a string, but accept real numbers too */
gint64
yoink_json_num (JsonObject *o, const char *key)
{
  JsonNode *n = member (o, key);
  if (!n || !JSON_NODE_HOLDS_VALUE (n))
    return 0;
  if (json_node_get_value_type (n) == G_TYPE_INT64)
    return json_node_get_int (n);
  const char *s = yoink_json_str (o, key);
  return s ? g_ascii_strtoll (s, NULL, 10) : 0;
}

JsonObject *
yoink_json_obj (JsonObject *o, const char *key)
{
  JsonNode *n = member (o, key);
  return n && JSON_NODE_HOLDS_OBJECT (n) ? json_node_get_object (n) : NULL;
}

JsonArray *
yoink_json_arr (JsonObject *o, const char *key)
{
  JsonNode *n = member (o, key);
  return n && JSON_NODE_HOLDS_ARRAY (n) ? json_node_get_array (n) : NULL;
}

JsonObject *
yoink_json_arr_obj (JsonArray *a, guint index)
{
  if (!a || index >= json_array_get_length (a))
    return NULL;
  JsonNode *n = json_array_get_element (a, index);
  return JSON_NODE_HOLDS_OBJECT (n) ? json_node_get_object (n) : NULL;
}
