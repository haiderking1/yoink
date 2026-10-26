import { pingDy } from '../src/native.js';
import { getSettings, onSettingsChanged, updateSettings } from '../src/settings.js';

const $ = (id) => document.getElementById(id);

async function load() {
  const s = await getSettings();
  $('enabled').checked = s.enabled;
  $('minSizeMB').value = s.minSizeMB;
  $('takeUnknownSize').checked = s.takeUnknownSize;
  $('excludedSites').value = s.excludedSites.join('\n');
}

// accepts "example.com", "*.example.com" or a pasted URL
function parseSites(text) {
  return text
    .split(/\s+/)
    .map((s) => s.toLowerCase().replace(/^[a-z]+:\/\//, '').replace(/^\*\./, '').split('/')[0])
    .filter(Boolean);
}

function bind() {
  $('enabled').addEventListener('change', (e) => updateSettings({ enabled: e.target.checked }));
  $('takeUnknownSize').addEventListener('change', (e) => updateSettings({ takeUnknownSize: e.target.checked }));
  $('minSizeMB').addEventListener('change', (e) => {
    const mb = Math.max(0, Number(e.target.value) || 0);
    e.target.value = mb;
    updateSettings({ minSizeMB: mb });
  });
  $('excludedSites').addEventListener('change', (e) => updateSettings({ excludedSites: parseSites(e.target.value) }));
}

async function checkDy() {
  const status = $('status');
  try {
    const reply = await pingDy();
    status.dataset.state = 'ok';
    $('status-text').textContent = `Connected to DY ${reply.version}`;
  } catch (err) {
    status.dataset.state = 'error';
    $('status-text').textContent = `Can't reach DY (${err.message}). Run "make install" in the DY folder.`;
  }
}

load().then(bind);
onSettingsChanged(load); // e.g. toggled from the toolbar while this page is open
checkDy();
