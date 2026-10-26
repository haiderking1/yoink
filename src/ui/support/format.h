#pragma once

#include "downloads/item.h"
#include "downloads/poller.h"

char *dy_format_speed    (gint64 bytes_per_sec);
char *dy_format_eta      (gint64 seconds);
/* "1.4 GB of 2.1 GB · 12 MB/s · 2 min left" and friends */
char *dy_format_status   (const DyItem *item);
/* "2 active · 1 queued", or "" when idle */
char *dy_format_subtitle (const DyStats *stats);
