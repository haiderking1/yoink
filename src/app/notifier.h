#pragma once

#include <gio/gio.h>

/* Sends a desktop notification when a download finishes or fails. */
typedef struct _YoinkNotifier YoinkNotifier;

YoinkNotifier *yoink_notifier_new    (GApplication *app);
/* items: YoinkItem* from the latest poll */
void        yoink_notifier_update (YoinkNotifier *n, GPtrArray *items);
void        yoink_notifier_free   (YoinkNotifier *n);
