#pragma once

#include <gio/gio.h>

#include "aria2/client.h"

/* Owns the aria2c process: starts it with RPC on a free localhost port and
 * shuts it down gracefully so the session file gets written. */
typedef struct _Aria2Daemon Aria2Daemon;

Aria2Daemon *aria2_daemon_spawn      (const char *download_dir, const char *session_file, GError **error);
Aria2Client *aria2_daemon_get_client (Aria2Daemon *d);
gboolean     aria2_daemon_is_running (Aria2Daemon *d);
void         aria2_daemon_stop       (Aria2Daemon *d);
