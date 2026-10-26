// Builds what Yoink needs to fetch a URL the way the browser would: same
// cookies, referer and user agent, so logged-in downloads keep working.

export async function buildRequest({ url, referrer, filename }) {
  const request = { url, userAgent: navigator.userAgent, headers: [] };

  const cookie = await cookieHeader(url);
  if (cookie) request.headers.push(`Cookie: ${cookie}`);
  if (referrer) request.referer = referrer;

  const name = basename(filename);
  if (name) request.filename = name;
  return request;
}

async function cookieHeader(url) {
  try {
    const cookies = await chrome.cookies.getAll({ url });
    // browsers send more specific paths first
    cookies.sort((a, b) => b.path.length - a.path.length);
    return cookies.map((c) => `${c.name}=${c.value}`).join('; ');
  } catch {
    return '';
  }
}

function basename(path) {
  return path ? path.split(/[\\/]/).pop() : '';
}
