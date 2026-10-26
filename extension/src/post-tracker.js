// Remembers URLs the browser recently sent a non-GET request to. A download
// that came straight out of a form POST can't be replayed by Yoink (it would
// GET the URL), so those stay in the browser.

const TTL_MS = 60_000;
const posted = new Map(); // url -> time

function prune() {
  const cutoff = Date.now() - TTL_MS;
  for (const [url, time] of posted) if (time < cutoff) posted.delete(url);
}

export function installPostTracker() {
  chrome.webRequest.onBeforeRequest.addListener(
    (details) => {
      if (details.method !== 'GET' && details.method !== 'HEAD') {
        posted.set(details.url, Date.now());
        prune();
      }
    },
    { urls: ['<all_urls>'], types: ['main_frame', 'sub_frame', 'xmlhttprequest', 'other'] },
  );
}

export function wasPosted(url) {
  prune();
  return posted.has(url);
}
