#pragma once

#include <glib.h>

/* $DY_DOWNLOAD_DIR, else the XDG download dir, else ~/Downloads */
char *dy_paths_download_dir (void);
/* where aria2 keeps unfinished downloads between runs */
char *dy_paths_session_file (void);
