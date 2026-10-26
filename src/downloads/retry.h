#pragma once

#include "downloads/control.h"

/* Re-adds a failed download with its original links and options (cookies
 * included), then forgets the failed one. */
void yoink_retry (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data);
