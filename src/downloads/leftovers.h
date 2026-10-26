#pragma once

#include <json-glib/json-glib.h>

/* What an unfinished download left on disk: partial files plus aria2's
 * .aria2 control files. Only paths inside the download's own directory are
 * collected, so nothing else can ever be deleted. */
typedef struct _YoinkLeftovers YoinkLeftovers;

/* status: tellStatus result with at least "dir", "files" and "bittorrent" */
YoinkLeftovers *yoink_leftovers_from_status (JsonObject *status);
/* Deletes in a worker thread. Takes ownership. */
void         yoink_leftovers_delete      (YoinkLeftovers *l);
void         yoink_leftovers_free        (YoinkLeftovers *l);

/* The tellStatus keys yoink_leftovers_from_status() needs. NULL-terminated. */
const char *const *yoink_leftovers_status_keys (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (YoinkLeftovers, yoink_leftovers_free)
