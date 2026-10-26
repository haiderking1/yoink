#include "app/paths.h"

char *
dy_paths_download_dir (void)
{
  const char *dir = g_getenv ("DY_DOWNLOAD_DIR");
  if (!dir)
    dir = g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD);
  return dir ? g_strdup (dir) : g_build_filename (g_get_home_dir (), "Downloads", NULL);
}

char *
dy_paths_session_file (void)
{
  return g_build_filename (g_get_user_data_dir (), "dy", "aria2.session", NULL);
}
