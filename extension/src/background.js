// Service worker entry. Listeners have to be registered synchronously at
// startup so Chromium can wake the worker for them.

import { installMenu } from './menu.js';
import { installPostTracker } from './post-tracker.js';
import { installTakeover } from './takeover.js';
import { installToolbar } from './toolbar.js';

installPostTracker();
installTakeover();
installMenu();
installToolbar();
