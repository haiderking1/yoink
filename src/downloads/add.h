#pragma once

#include <gio/gio.h>

#include "aria2/client.h"
#include "downloads/request.h"

/* Called once per batch. error is the first failure, or NULL. */
typedef void (*YoinkAddDone) (guint added, const char *error, gpointer user_data);

/* One aria2 download per link and per torrent in the request. */
void yoink_add_request (Aria2Client *client, const YoinkRequest *request, YoinkAddDone done, gpointer user_data);
