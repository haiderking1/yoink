#pragma once

#include <json-glib/json-glib.h>

/* What an unfinished download left on disk: partial files plus aria2's
 * .aria2 control files. Only paths inside the download's own directory are
 * collected, so nothing else can ever be deleted. */
typedef struct _DyLeftovers DyLeftovers;

/* status: tellStatus result with at least "dir", "files" and "bittorrent" */
DyLeftovers *dy_leftovers_from_status (JsonObject *status);
/* Deletes in a worker thread. Takes ownership. */
void         dy_leftovers_delete      (DyLeftovers *l);
void         dy_leftovers_free        (DyLeftovers *l);

/* The tellStatus keys dy_leftovers_from_status() needs. NULL-terminated. */
const char *const *dy_leftovers_status_keys (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DyLeftovers, dy_leftovers_free)
