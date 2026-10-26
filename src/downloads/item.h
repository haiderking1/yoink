#pragma once

#include <json-glib/json-glib.h>

typedef enum {
  YOINK_STATE_UNKNOWN,
  YOINK_STATE_ACTIVE,
  YOINK_STATE_WAITING,
  YOINK_STATE_PAUSED,
  YOINK_STATE_ERROR,
  YOINK_STATE_COMPLETE,
  YOINK_STATE_REMOVED,
} YoinkState;

/* One download as reported by aria2's tellStatus, flattened for display. */
typedef struct {
  char      *gid;
  YoinkState state;
  gint64     total;
  gint64     done;
  gint64     download_speed;
  gint64     upload_speed;
  gboolean   seeding;
  gboolean   is_dir;      /* multi-file torrent */
  gboolean   is_handoff;  /* finished .torrent/magnet that turned into the real download */
  char      *name;
  char      *path;        /* NULL until known */
  char      *uri;
  char      *dir;
  char      *error_message;
} YoinkItem;

YoinkItem     *yoink_item_from_json   (JsonObject *status);
void        yoink_item_free        (YoinkItem *item);

/* The tellStatus keys yoink_item_from_json() reads. NULL-terminated. */
const char *const *yoink_item_status_keys (void);

YoinkState     yoink_state_parse      (const char *s);
/* aria2's own name for the state; also used as a CSS class */
const char *yoink_state_name       (YoinkState state);
gboolean    yoink_state_is_stopped (YoinkState state);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (YoinkItem, yoink_item_free)
