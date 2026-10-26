#pragma once

#include <adwaita.h>

#include "downloads/item.h"

/* One download in the list: icon, name, progress bar, status line and the
 * pause/resume/retry, show-in-folder and remove buttons. Buttons fire app
 * actions with the gid as target. */
#define DY_TYPE_ROW (dy_row_get_type ())
G_DECLARE_FINAL_TYPE (DyRow, dy_row, DY, ROW, GtkListBoxRow)

DyRow      *dy_row_new          (const char *gid);
void        dy_row_update       (DyRow *self, const DyItem *item);

const char *dy_row_get_gid      (DyRow *self);
const char *dy_row_get_path     (DyRow *self);
DyState     dy_row_get_state    (DyRow *self);

int         dy_row_get_sort_key (DyRow *self);
/* TRUE if the key changed and the list needs re-sorting */
gboolean    dy_row_set_sort_key (DyRow *self, int key);
