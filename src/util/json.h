#pragma once

#include <json-glib/json-glib.h>

/* Null-safe accessors: missing members or wrong types give NULL / 0. */
const char *yoink_json_str     (JsonObject *o, const char *key);
gint64      yoink_json_num     (JsonObject *o, const char *key);
JsonObject *yoink_json_obj     (JsonObject *o, const char *key);
JsonArray  *yoink_json_arr     (JsonObject *o, const char *key);
JsonObject *yoink_json_arr_obj (JsonArray *a, guint index);
