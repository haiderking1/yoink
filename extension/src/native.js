// Talks to dy-host, the native messaging bridge installed with DY.

const HOST = 'dev.soka.dy';

async function send(message) {
  const reply = await chrome.runtime.sendNativeMessage(HOST, message);
  if (!reply?.ok) throw new Error(reply?.error || 'No reply from DY');
  return reply;
}

export const sendToDy = (request) => send({ type: 'add', request });

export const pingDy = () => send({ type: 'ping' });
