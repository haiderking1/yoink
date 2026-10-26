#pragma once

#include <json-glib/json-glib.h>

/* Null-safe accessors: missing members or wrong types give NULL / 0. */
const char *dy_json_str     (JsonObject *o, const char *key);
gint64      dy_json_num     (JsonObject *o, const char *key);
JsonObject *dy_json_obj     (JsonObject *o, const char *key);
JsonArray  *dy_json_arr     (JsonObject *o, const char *key);
JsonObject *dy_json_arr_obj (JsonArray *a, guint index);
