#pragma once

#include <glib.h>

/* http(s), ftp, sftp or magnet: things aria2 can download from */
gboolean yoink_is_link       (const char *s);

/* Last path segment of a URI, unescaped; falls back to the host or the URI itself. */
char    *yoink_uri_basename  (const char *uri);
