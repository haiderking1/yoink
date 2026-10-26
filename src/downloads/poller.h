#pragma once

#include "aria2/client.h"
#include "downloads/item.h"

typedef struct {
  gint64 active;
  gint64 waiting;
  gint64 download_speed;
  gint64 upload_speed;
} DyStats;

/* items: DyItem*, in display order (active, queued, then stopped newest
 * first). On error, items and stats are NULL. */
typedef void (*DyPollFunc) (GPtrArray *items, const DyStats *stats, const GError *error, gpointer user_data);

/* Asks aria2 for every download once a second. */
typedef struct _DyPoller DyPoller;

DyPoller *dy_poller_new       (Aria2Client *client, DyPollFunc fn, gpointer user_data);
void      dy_poller_poll_soon (DyPoller *p);
/* No callbacks fire after this, even for requests already in flight. */
void      dy_poller_free      (DyPoller *p);
