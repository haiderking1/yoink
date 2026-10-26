#include "ui/support/format.h"

char *
dy_format_speed (gint64 bytes_per_sec)
{
  if (bytes_per_sec < 1000)
    return g_strdup_printf ("%d B/s", (int) bytes_per_sec);
  g_autofree char *size = g_format_size (bytes_per_sec);
  return g_strdup_printf ("%s/s", size);
}

char *
dy_format_eta (gint64 secs)
{
  if (secs < 60)
    return g_strdup_printf ("%d s left", (int) secs);
  if (secs < 3600)
    return g_strdup_printf ("%d min left", (int) (secs / 60));
  return g_strdup_printf ("%d h %d min left", (int) (secs / 3600), (int) (secs % 3600 / 60));
}

static char *
format_active (const DyItem *item, const char *done, const char *total)
{
  if (item->seeding) {
    g_autofree char *up = dy_format_speed (item->upload_speed);
    return g_strdup_printf ("Seeding · ↑ %s", up);
  }

  g_autofree char *speed = dy_format_speed (item->download_speed);
  if (item->total == 0)
    return item->done > 0 ? g_strdup_printf ("%s · %s", done, speed) : g_strdup ("Connecting…");

  g_autofree char *eta = item->download_speed > 0
                           ? dy_format_eta ((item->total - item->done) / item->download_speed)
                           : g_strdup ("Stalled");
  return g_strdup_printf ("%s of %s · %s · %s", done, total, speed, eta);
}

char *
dy_format_status (const DyItem *item)
{
  g_autofree char *total = g_format_size (item->total);
  g_autofree char *done = g_format_size (item->done);

  switch (item->state) {
  case DY_STATE_ACTIVE:
    return format_active (item, done, total);
  case DY_STATE_WAITING:
    return item->total ? g_strdup_printf ("Queued · %s of %s", done, total) : g_strdup ("Queued");
  case DY_STATE_PAUSED:
    return item->total ? g_strdup_printf ("Paused · %s of %s", done, total) : g_strdup ("Paused");
  case DY_STATE_COMPLETE:
    return g_strdup_printf ("%s · Done", total);
  case DY_STATE_ERROR:
    return g_strdup_printf ("Failed · %s", item->error_message ? item->error_message : "unknown error");
  default:
    return g_strdup (dy_state_name (item->state));
  }
}

char *
dy_format_subtitle (const DyStats *stats)
{
  if (stats->active && stats->waiting)
    return g_strdup_printf ("%" G_GINT64_FORMAT " active · %" G_GINT64_FORMAT " queued", stats->active, stats->waiting);
  if (stats->active)
    return g_strdup_printf ("%" G_GINT64_FORMAT " active", stats->active);
  if (stats->waiting)
    return g_strdup_printf ("%" G_GINT64_FORMAT " queued", stats->waiting);
  return g_strdup ("");
}
