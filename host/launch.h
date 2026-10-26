#pragma once

#include <glib.h>

/* Hands a JSON download request to Yoink through `yoink --request-stdin`.
 * If Yoink is running we wait for its verdict; otherwise the new process
 * becomes the Yoink window and we return as soon as it has the request. */
gboolean yoink_host_launch (const char *request_json, GError **error);
