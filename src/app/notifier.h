#pragma once

#include <gio/gio.h>

/* Sends a desktop notification when a download finishes or fails. */
typedef struct _DyNotifier DyNotifier;

DyNotifier *dy_notifier_new    (GApplication *app);
/* items: DyItem* from the latest poll */
void        dy_notifier_update (DyNotifier *n, GPtrArray *items);
void        dy_notifier_free   (DyNotifier *n);
