#pragma once

#include "downloads/control.h"

/* Re-adds a failed download with its original links and options (cookies
 * included), then forgets the failed one. */
void dy_retry (Aria2Client *c, const char *gid, DyControlDone done, gpointer user_data);
