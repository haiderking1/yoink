#include "downloads/control.h"

typedef struct {
  YoinkControlDone done;
  gpointer         user_data;
} Op;

static void
on_done (JsonNode *result, const GError *error, gpointer user_data)
{
  Op *op = user_data;
  if (op->done)
    op->done (error ? error->message : NULL, op->user_data);
  g_free (op);
}

static void
call (Aria2Client *c, const char *method, const char *gid, YoinkControlDone done, gpointer user_data)
{
  Op *op = g_new0 (Op, 1);
  op->done = done;
  op->user_data = user_data;

  if (gid)
    aria2_call_gid (c, method, gid, on_done, op);
  else
    aria2_call (c, method, NULL, on_done, op);
}

void
yoink_control_pause (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data)
{
  /* forcePause skips tracker announces, so the button feels instant */
  call (c, "aria2.forcePause", gid, done, user_data);
}

void
yoink_control_resume (Aria2Client *c, const char *gid, YoinkControlDone done, gpointer user_data)
{
  call (c, "aria2.unpause", gid, done, user_data);
}

void
yoink_control_pause_all (Aria2Client *c, YoinkControlDone done, gpointer user_data)
{
  call (c, "aria2.forcePauseAll", NULL, done, user_data);
}

void
yoink_control_resume_all (Aria2Client *c, YoinkControlDone done, gpointer user_data)
{
  call (c, "aria2.unpauseAll", NULL, done, user_data);
}
