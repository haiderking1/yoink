#include "app/paths.h"

char *
yoink_paths_download_dir (void)
{
  const char *dir = g_getenv ("YOINK_DOWNLOAD_DIR");
  if (!dir)
    dir = g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD);
  return dir ? g_strdup (dir) : g_build_filename (g_get_home_dir (), "Downloads", NULL);
}

char *
yoink_paths_session_file (void)
{
  return g_build_filename (g_get_user_state_dir (), "yoink", "aria2.session", NULL);
}

char *
yoink_paths_settings_file (void)
{
  return g_build_filename (g_get_user_config_dir (), "yoink", "settings.ini", NULL);
}
