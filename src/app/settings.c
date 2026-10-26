#include "app/settings.h"

#include <errno.h>

#include "app/paths.h"

#define GROUP "downloads"
#define KEY   "folder"

static GKeyFile *
load (const char *path)
{
  GKeyFile *kf = g_key_file_new ();
  g_key_file_load_from_file (kf, path, G_KEY_FILE_KEEP_COMMENTS, NULL);  /* missing is fine */
  return kf;
}

char *
yoink_settings_get_save_dir (void)
{
  g_autofree char *path = yoink_paths_settings_file ();
  g_autoptr (GKeyFile) kf = load (path);
  g_autofree char *dir = g_key_file_get_string (kf, GROUP, KEY, NULL);

  if (dir && g_path_is_absolute (dir) && g_file_test (dir, G_FILE_TEST_IS_DIR))
    return g_steal_pointer (&dir);
  return yoink_paths_download_dir ();
}

void
yoink_settings_set_save_dir (const char *dir)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *path = yoink_paths_settings_file ();
  g_autofree char *parent = g_path_get_dirname (path);
  g_autoptr (GKeyFile) kf = load (path);

  g_key_file_set_string (kf, GROUP, KEY, dir);
  if (g_mkdir_with_parents (parent, 0700) != 0 || !g_key_file_save_to_file (kf, path, &error))
    g_warning ("couldn't save %s: %s", path, error ? error->message : g_strerror (errno));
}
