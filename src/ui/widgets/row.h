#pragma once

#include <adwaita.h>

#include "downloads/item.h"

/* One download in the list: icon, name, progress bar, status line and the
 * pause/resume/retry, show-in-folder and remove buttons. Buttons fire app
 * actions with the gid as target. */
#define YOINK_TYPE_ROW (yoink_row_get_type ())
G_DECLARE_FINAL_TYPE (YoinkRow, yoink_row, YOINK, ROW, GtkListBoxRow)

YoinkRow      *yoink_row_new          (const char *gid);
void        yoink_row_update       (YoinkRow *self, const YoinkItem *item);

const char *yoink_row_get_gid      (YoinkRow *self);
const char *yoink_row_get_path     (YoinkRow *self);
YoinkState     yoink_row_get_state    (YoinkRow *self);

int         yoink_row_get_sort_key (YoinkRow *self);
/* TRUE if the key changed and the list needs re-sorting */
gboolean    yoink_row_set_sort_key (YoinkRow *self, int key);
