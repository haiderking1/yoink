// User settings, kept in chrome.storage.local.

export const DEFAULTS = Object.freeze({
  enabled: true,
  minSizeMB: 1, // smaller downloads stay in the browser
  takeUnknownSize: true,
  excludedSites: [], // hostnames; subdomains match too
});

export async function getSettings() {
  return chrome.storage.local.get({ ...DEFAULTS });
}

export async function updateSettings(patch) {
  await chrome.storage.local.set(patch);
}

export function onSettingsChanged(listener) {
  chrome.storage.onChanged.addListener((changes, area) => {
    if (area === 'local') listener();
  });
}
