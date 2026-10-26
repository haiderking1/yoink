#pragma once

#include <glib.h>

/* Hands a JSON download request to DY through `dy --request-stdin`.
 * If DY is running we wait for its verdict; otherwise the new process
 * becomes the DY window and we return as soon as it has the request. */
gboolean dy_host_launch (const char *request_json, GError **error);
