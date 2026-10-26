// Toolbar button: click to pause/resume catching downloads. The badge shows
// "off" while paused and flashes "!" when Yoink couldn't take a download.

import { getSettings, onSettingsChanged, updateSettings } from './settings.js';

const PROBLEM_MS = 8000;
let problemTimer = null;

export async function refreshToolbar() {
  if (problemTimer) return;
  const { enabled } = await getSettings();
  await chrome.action.setBadgeBackgroundColor({ color: '#5e5c64' });
  await chrome.action.setBadgeText({ text: enabled ? '' : 'off' });
  await chrome.action.setTitle({
    title: enabled ? 'Yoink: catching downloads (click to pause)' : 'Yoink: paused (click to resume)',
  });
}

export function showProblem(message) {
  clearTimeout(problemTimer);
  chrome.action.setBadgeBackgroundColor({ color: '#e01b24' });
  chrome.action.setBadgeText({ text: '!' });
  chrome.action.setTitle({ title: `Yoink: ${message}` });
  problemTimer = setTimeout(() => {
    problemTimer = null;
    refreshToolbar();
  }, PROBLEM_MS);
}

export function installToolbar() {
  chrome.action.onClicked.addListener(async () => {
    const { enabled } = await getSettings();
    await updateSettings({ enabled: !enabled });
  });
  onSettingsChanged(refreshToolbar);
  refreshToolbar();
}
