#pragma once

#include <json-glib/json-glib.h>

/* JSON-RPC client for one aria2 instance. Reference counted so in-flight
 * work can keep it alive across a daemon restart. */
typedef struct _Aria2Client Aria2Client;

/* result is only valid during the callback. */
typedef void (*Aria2Callback) (JsonNode *result, const GError *error, gpointer user_data);

Aria2Client *aria2_client_new   (guint16 port, const char *secret);
Aria2Client *aria2_client_ref   (Aria2Client *c);
void         aria2_client_unref (Aria2Client *c);

/* Takes ownership of params (may be NULL). The secret token is added automatically. */
void      aria2_call            (Aria2Client *c, const char *method, JsonArray *params,
                                 Aria2Callback cb, gpointer user_data);

/* For the common single-gid methods (pause, unpause, remove, ...). */
void      aria2_call_gid        (Aria2Client *c, const char *method, const char *gid,
                                 Aria2Callback cb, gpointer user_data);

/* One entry of a system.multicall batch. Takes ownership of params. */
JsonNode *aria2_multicall_entry (Aria2Client *c, const char *method, JsonArray *params);

/* Each multicall result is either [value] or a fault object; NULL for faults. */
JsonNode *aria2_multicall_value (JsonArray *results, guint index);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (Aria2Client, aria2_client_unref)
