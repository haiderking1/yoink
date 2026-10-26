#pragma once

#include <json-glib/json-glib.h>

/* Handles one message from the extension and returns the reply:
 *   { "type": "ping" }                    → { "ok": true, "version": "…" }
 *   { "type": "add", "request": { … } }   → { "ok": true } or { "ok": false, "error": "…" } */
JsonNode *dy_host_handle (JsonNode *message);
