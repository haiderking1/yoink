#pragma once

#include "downloads/item.h"
#include "downloads/poller.h"

char *yoink_format_speed    (gint64 bytes_per_sec);
char *yoink_format_eta      (gint64 seconds);
/* "1.4 GB of 2.1 GB · 12 MB/s · 2 min left" and friends */
char *yoink_format_status   (const YoinkItem *item);
/* "2 active · 1 queued", or "" when idle */
char *yoink_format_subtitle (const YoinkStats *stats);
