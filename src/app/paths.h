#pragma once

#include <glib.h>

/* $YOINK_DOWNLOAD_DIR, else the XDG download dir, else ~/Downloads */
char *yoink_paths_download_dir (void);
/* where aria2 keeps unfinished downloads between runs: $XDG_STATE_HOME/yoink.
 * Its own dir, since the install puts the browser extension under XDG_DATA_HOME. */
char *yoink_paths_session_file (void);
/* Yoink's own settings (GKeyFile): $XDG_CONFIG_HOME/yoink/settings.ini */
char *yoink_paths_settings_file (void);
