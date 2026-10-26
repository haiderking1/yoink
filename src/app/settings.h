#pragma once

#include <glib.h>

/* The folder downloads were last saved to. Falls back to the default download
 * dir when nothing was picked yet or the folder is gone. */
char *yoink_settings_get_save_dir (void);
void  yoink_settings_set_save_dir (const char *dir);
