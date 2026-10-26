#include "util/links.h"

#include <string.h>

gboolean
yoink_is_link (const char *s)
{
  static const char *prefixes[] = { "http://", "https://", "ftp://", "sftp://", "magnet:" };

  if (!s)
    return FALSE;
  for (guint i = 0; i < G_N_ELEMENTS (prefixes); i++)
    if (g_ascii_strncasecmp (s, prefixes[i], strlen (prefixes[i])) == 0)
      return TRUE;
  return FALSE;
}

char *
yoink_uri_basename (const char *uri)
{
  g_autoptr (GUri) u = g_uri_parse (uri, G_URI_FLAGS_NONE, NULL);
  const char *path = u ? g_uri_get_path (u) : NULL;

  if (path && *path && !g_str_has_suffix (path, "/")) {
    g_autofree char *base = g_path_get_basename (path);
    char *unescaped = g_uri_unescape_string (base, NULL);
    return unescaped ? unescaped : g_steal_pointer (&base);
  }
  if (u && g_uri_get_host (u))
    return g_strdup (g_uri_get_host (u));
  return g_strdup (uri);
}
