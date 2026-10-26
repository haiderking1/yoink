#pragma once

#include <adwaita.h>

/* Scrollable list of DyRows kept in sync with what aria2 reports. */
#define DY_TYPE_DOWNLOAD_LIST (dy_download_list_get_type ())
G_DECLARE_FINAL_TYPE (DyDownloadList, dy_download_list, DY, DOWNLOAD_LIST, AdwBin)

DyDownloadList *dy_download_list_new       (void);
/* items: DyItem*, in display order */
void            dy_download_list_update    (DyDownloadList *self, GPtrArray *items);
void            dy_download_list_clear     (DyDownloadList *self);
guint           dy_download_list_get_count (DyDownloadList *self);
