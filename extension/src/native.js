// Talks to yoink-host, the native messaging bridge installed with Yoink.

const HOST = 'dev.soka.yoink';

async function send(message) {
  const reply = await chrome.runtime.sendNativeMessage(HOST, message);
  if (!reply?.ok) throw new Error(reply?.error || 'No reply from Yoink');
  return reply;
}

export const sendToDy = (request) => send({ type: 'add', request });

export const pingDy = () => send({ type: 'ping' });
