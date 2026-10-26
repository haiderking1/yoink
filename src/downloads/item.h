#pragma once

#include <json-glib/json-glib.h>

typedef enum {
  DY_STATE_UNKNOWN,
  DY_STATE_ACTIVE,
  DY_STATE_WAITING,
  DY_STATE_PAUSED,
  DY_STATE_ERROR,
  DY_STATE_COMPLETE,
  DY_STATE_REMOVED,
} DyState;

/* One download as reported by aria2's tellStatus, flattened for display. */
typedef struct {
  char    *gid;
  DyState  state;
  gint64   total;
  gint64   done;
  gint64   download_speed;
  gint64   upload_speed;
  gboolean seeding;
  gboolean is_dir;      /* multi-file torrent */
  gboolean is_handoff;  /* finished .torrent/magnet that turned into the real download */
  char    *name;
  char    *path;        /* NULL until known */
  char    *uri;
  char    *dir;
  char    *error_message;
} DyItem;

DyItem     *dy_item_from_json   (JsonObject *status);
void        dy_item_free        (DyItem *item);

/* The tellStatus keys dy_item_from_json() reads. NULL-terminated. */
const char *const *dy_item_status_keys (void);

DyState     dy_state_parse      (const char *s);
/* aria2's own name for the state; also used as a CSS class */
const char *dy_state_name       (DyState state);
gboolean    dy_state_is_stopped (DyState state);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DyItem, dy_item_free)
