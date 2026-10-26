#pragma once

#include <adwaita.h>

/* Scrollable list of YoinkRows kept in sync with what aria2 reports. */
#define YOINK_TYPE_DOWNLOAD_LIST (yoink_download_list_get_type ())
G_DECLARE_FINAL_TYPE (YoinkDownloadList, yoink_download_list, YOINK, DOWNLOAD_LIST, AdwBin)

YoinkDownloadList *yoink_download_list_new       (void);
/* items: YoinkItem*, in display order */
void            yoink_download_list_update    (YoinkDownloadList *self, GPtrArray *items);
void            yoink_download_list_clear     (YoinkDownloadList *self);
guint           yoink_download_list_get_count (YoinkDownloadList *self);
