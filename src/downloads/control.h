#pragma once

#include "aria2/client.h"

/* error is NULL on success. Shared by every download operation. */
typedef void (*YoinkControlDone) (const char *error, gpointer user_data);

void yoink_control_pause      (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data);
void yoink_control_resume     (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data);
void yoink_control_pause_all  (Aria2Client *c, YoinkControlDone done, gpointer user_data);
void yoink_control_resume_all (Aria2Client *c, YoinkControlDone done, gpointer user_data);
