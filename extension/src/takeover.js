// IDM-style takeover: every download the browser starts is offered to DY
// first. If DY can't take it, the browser carries on as if nothing happened.

import { skipReason } from './filter.js';
import { sendToDy } from './native.js';
import { buildRequest } from './request.js';
import { getSettings } from './settings.js';
import { showProblem } from './toolbar.js';

async function takeOver(item) {
  const settings = await getSettings();
  if (skipReason(item, settings)) return false;

  const request = await buildRequest({
    url: item.finalUrl || item.url,
    referrer: item.referrer,
    filename: item.filename,
  });

  try {
    await sendToDy(request);
  } catch (err) {
    console.warn('DY: hand-off failed, the browser keeps this download:', err);
    showProblem(err.message);
    return false;
  }

  await chrome.downloads.cancel(item.id).catch(() => {});
  await chrome.downloads.erase({ id: item.id }).catch(() => {});
  return true;
}

export function installTakeover() {
  // Fires once the response headers are in (size, final URL and file name are
  // known) and before the browser writes the file.
  chrome.downloads.onDeterminingFilename.addListener((item, suggest) => {
    takeOver(item).then(
      (taken) => taken || suggest(),
      () => suggest(),
    );
    return true; // answering asynchronously
  });
}
