#include "app/notifier.h"

#include "downloads/item.h"

struct _YoinkNotifier {
  GApplication *app;
  GHashTable   *states;  /* gid -> YoinkState seen last poll */
};

YoinkNotifier *
yoink_notifier_new (GApplication *app)
{
  YoinkNotifier *n = g_new0 (YoinkNotifier, 1);
  n->app = app;
  n->states = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  return n;
}

static void
notify (YoinkNotifier *n, const char *title, const YoinkItem *item)
{
  g_autoptr (GNotification) note = g_notification_new (title);
  g_notification_set_body (note, item->name);
  g_application_send_notification (n->app, NULL, note);
}

void
yoink_notifier_update (YoinkNotifier *n, GPtrArray *items)
{
  GHashTable *next = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  for (guint i = 0; i < items->len; i++) {
    const YoinkItem *item = g_ptr_array_index (items, i);
    gpointer prev;

    /* only transitions we watched happen: no burst of notifications on startup */
    if (g_hash_table_lookup_extended (n->states, item->gid, NULL, &prev) &&
        !yoink_state_is_stopped (GPOINTER_TO_INT (prev))) {
      if (item->state == YOINK_STATE_COMPLETE)
        notify (n, "Download finished", item);
      else if (item->state == YOINK_STATE_ERROR)
        notify (n, "Download failed", item);
    }
    g_hash_table_insert (next, g_strdup (item->gid), GINT_TO_POINTER (item->state));
  }

  g_hash_table_unref (n->states);
  n->states = next;
}

void
yoink_notifier_free (YoinkNotifier *n)
{
  if (!n)
    return;
  g_hash_table_unref (n->states);
  g_free (n);
}
