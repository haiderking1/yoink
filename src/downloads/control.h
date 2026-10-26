#pragma once

#include "aria2/client.h"

/* error is NULL on success. Shared by every download operation. */
typedef void (*DyControlDone) (const char *error, gpointer user_data);

void dy_control_pause      (Aria2Client *c, const char *gid, DyControlDone done, gpointer user_data);
void dy_control_resume     (Aria2Client *c, const char *gid, DyControlDone done, gpointer user_data);
void dy_control_pause_all  (Aria2Client *c, DyControlDone done, gpointer user_data);
void dy_control_resume_all (Aria2Client *c, DyControlDone done, gpointer user_data);
