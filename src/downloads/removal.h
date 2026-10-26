#pragma once

#include "downloads/control.h"

/* Removing downloads, cleaning up what unfinished ones left on disk.
 * Finished files are never deleted. */

/* Stops an active, queued or paused download and deletes its partial files. */
void dy_removal_cancel         (Aria2Client *c, const char *gid, DyControlDone done, gpointer user_data);
/* Forgets a stopped download; a failed one's partial files go too. */
void dy_removal_dismiss        (Aria2Client *c, const char *gid, DyControlDone done, gpointer user_data);
/* Forgets every stopped download, deleting partial files of the failed ones. */
void dy_removal_clear_finished (Aria2Client *c, DyControlDone done, gpointer user_data);
