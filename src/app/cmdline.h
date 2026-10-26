#pragma once

#include <gio/gio.h>

#include "downloads/request.h"

/* Receives what the command line asked for. Takes ownership of both;
 * either may be NULL. */
typedef void (*DyCmdlineSubmit) (DyRequest *request, GListModel *torrents, gpointer user_data);

/* Registers --header, --referer, --user-agent, --out and --request-stdin. */
void dy_cmdline_register (GApplication *app);

/* Turns `dy [OPTIONS] LINK|FILE.torrent…` (or a JSON request on stdin) into
 * a submit call. Runs in the primary instance; returns the exit status. */
int  dy_cmdline_handle   (GApplicationCommandLine *cmd, DyCmdlineSubmit submit, gpointer user_data);
