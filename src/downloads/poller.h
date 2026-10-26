#pragma once

#include "aria2/client.h"
#include "downloads/item.h"

typedef struct {
  gint64 active;
  gint64 waiting;
  gint64 download_speed;
  gint64 upload_speed;
} YoinkStats;

/* items: YoinkItem*, in display order (active, queued, then stopped newest
 * first). On error, items and stats are NULL. */
typedef void (*YoinkPollFunc) (GPtrArray *items, const YoinkStats *stats, const GError *error, gpointer user_data);

/* Asks aria2 for every download once a second. */
typedef struct _YoinkPoller YoinkPoller;

YoinkPoller *yoink_poller_new       (Aria2Client *client, YoinkPollFunc fn, gpointer user_data);
void      yoink_poller_poll_soon (YoinkPoller *p);
/* No callbacks fire after this, even for requests already in flight. */
void      yoink_poller_free      (YoinkPoller *p);
