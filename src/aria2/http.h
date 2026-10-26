#pragma once

#include <gio/gio.h>

/* Blocking HTTP/1.1 POST of a JSON body to 127.0.0.1:port/path. Returns the
 * response body whatever the status code, since JSON-RPC errors come back as
 * HTTP 400 with a JSON body. Meant to run in a worker thread. */
GBytes *dy_http_post_json (guint16       port,
                           const char   *path,
                           const char   *body,
                           GCancellable *cancel,
                           GError      **error);
