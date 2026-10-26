// Decides whether a browser download should go to DY: returns why it should
// stay in the browser, or null to take it over.

import { wasPosted } from './post-tracker.js';

const MB = 1024 * 1024;
const FETCHABLE = /^(https?|ftp):/i;

export function skipReason(item, settings) {
  const url = item.finalUrl || item.url;

  if (!settings.enabled) return 'takeover is off';
  if (item.incognito) return 'incognito window';
  if (item.byExtensionId) return 'started by an extension';
  // blob: and data: downloads only exist inside the page
  if (!FETCHABLE.test(url)) return 'not a web URL';
  if (wasPosted(url)) return 'came from a form submission';
  if (item.totalBytes > 0 && item.totalBytes < settings.minSizeMB * MB) return 'below the size threshold';
  if (item.totalBytes <= 0 && !settings.takeUnknownSize) return 'unknown size';
  if (isExcluded([url, item.referrer], settings.excludedSites)) return 'site is excluded';
  return null;
}

function hostOf(url) {
  try {
    return new URL(url).hostname;
  } catch {
    return '';
  }
}

function isExcluded(urls, sites) {
  const hosts = urls.map(hostOf).filter(Boolean);
  return sites.some((site) => hosts.some((host) => host === site || host.endsWith(`.${site}`)));
}
