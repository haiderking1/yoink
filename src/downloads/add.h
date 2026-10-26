#pragma once

#include <gio/gio.h>

#include "aria2/client.h"
#include "downloads/request.h"

/* Called once per batch. error is the first failure, or NULL. */
typedef void (*DyAddDone) (guint added, const char *error, gpointer user_data);

/* One aria2 download per link in the request, all sharing its options. */
void dy_add_request  (Aria2Client *client, const DyRequest *request, DyAddDone done, gpointer user_data);

/* files: GListModel of GFile pointing at .torrent files */
void dy_add_torrents (Aria2Client *client, GListModel *files, DyAddDone done, gpointer user_data);
