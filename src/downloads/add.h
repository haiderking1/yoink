#pragma once

#include <gio/gio.h>

#include "aria2/client.h"
#include "downloads/request.h"

/* Called once per batch. error is the first failure, or NULL. */
typedef void (*YoinkAddDone) (guint added, const char *error, gpointer user_data);

/* One aria2 download per link in the request, all sharing its options. */
void yoink_add_request  (Aria2Client *client, const YoinkRequest *request, YoinkAddDone done, gpointer user_data);

/* files: GListModel of GFile pointing at .torrent files */
void yoink_add_torrents (Aria2Client *client, GListModel *files, YoinkAddDone done, gpointer user_data);
