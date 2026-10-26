// "Yoink it" on links and media, which works even while catching is paused.

import { sendToDy } from './native.js';
import { buildRequest } from './request.js';
import { showProblem } from './toolbar.js';

const MENU_ID = 'yoink-download';

export function installMenu() {
  chrome.runtime.onInstalled.addListener(() => {
    chrome.contextMenus.create({
      id: MENU_ID,
      title: 'Yoink it',
      contexts: ['link', 'image', 'video', 'audio'],
      targetUrlPatterns: ['http://*/*', 'https://*/*', 'ftp://*/*'],
    });
  });

  chrome.contextMenus.onClicked.addListener(async (info) => {
    if (info.menuItemId !== MENU_ID) return;
    const url = info.linkUrl || info.srcUrl;
    if (!url) return;

    try {
      await sendToDy(await buildRequest({ url, referrer: info.pageUrl }));
    } catch (err) {
      showProblem(err.message);
    }
  });
}
