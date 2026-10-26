#pragma once

#include "app/application.h"

/* Registers the app.* actions the UI and D-Bus use, plus their shortcuts.
 * Per-download actions take the gid as a string target. */
void dy_actions_install (DyApplication *app);
