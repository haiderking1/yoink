#pragma once

#include <gio/gio.h>
#include <json-glib/json-glib.h>

/* Chrome native messaging framing: a 32-bit native-endian length, then that
 * many bytes of UTF-8 JSON. */

/* NULL without error at a clean EOF (the browser closed the pipe). */
JsonNode *dy_host_read_message  (GInputStream *in, GError **error);
gboolean  dy_host_write_message (GOutputStream *out, JsonNode *message, GError **error);
