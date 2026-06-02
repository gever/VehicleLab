#include "web_server.h"
#include "vehicle.h"
#include "wifi_manager.h"

#include <ESPAsyncWebServer.h>
#include <Arduino.h>

// ---------------------------------------------------------------------------
// web_server.cpp – All HTTP routes for the VehicleLab remote-control server.
// ---------------------------------------------------------------------------

static AsyncWebServer server(80);

// ============================================================
// Helper: send a simple JSON response
// ============================================================
static void json_ok(AsyncWebServerRequest* req, const String& body) {
    req->send(200, "application/json", body);
}

// ============================================================
// /drive  –  HTML controller UI
// ============================================================
static const char DRIVE_UI[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>VehicleLab Controller</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap');

  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  :root {
    --bg:       #0d0f14;
    --surface:  #161b26;
    --border:   #2a3040;
    --accent:   #4f8ef7;
    --accent2:  #8b5cf6;
    --danger:   #ef4444;
    --success:  #22c55e;
    --text:     #e4e8f0;
    --muted:    #6b7a99;
    --radius:   16px;
    --btn-size: clamp(80px, 18vw, 120px);
  }

  html, body {
    height: 100%;
    background: var(--bg);
    color: var(--text);
    font-family: 'Inter', sans-serif;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 24px;
    padding: 24px;
  }

  h1 {
    font-size: 1.4rem;
    font-weight: 700;
    letter-spacing: .05em;
    background: linear-gradient(135deg, var(--accent), var(--accent2));
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }

  /* ---- Status bar ---- */
  #status-bar {
    display: flex;
    gap: 20px;
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 40px;
    padding: 10px 28px;
    font-size: .85rem;
    color: var(--muted);
  }
  #status-bar span { color: var(--text); font-weight: 600; }

  /* ---- Control grid ---- */
  .ctrl-grid {
    display: grid;
    grid-template-columns: repeat(3, var(--btn-size));
    grid-template-rows: repeat(3, var(--btn-size));
    gap: 12px;
  }

  /* ---- Buttons ---- */
  .btn {
    border: none;
    border-radius: var(--radius);
    background: var(--surface);
    border: 1px solid var(--border);
    color: var(--text);
    font-family: inherit;
    font-size: 2rem;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: transform .1s ease, background .15s ease, box-shadow .15s ease;
    user-select: none;
    -webkit-user-select: none;
    touch-action: none;
  }

  .btn:active, .btn.active {
    transform: scale(0.92);
    background: var(--accent);
    border-color: var(--accent);
    box-shadow: 0 0 20px rgba(79,142,247,.45);
  }

  .btn.stop {
    background: rgba(239,68,68,.15);
    border-color: var(--danger);
    font-size: 1.1rem;
    font-weight: 700;
    color: var(--danger);
    letter-spacing: .03em;
  }
  .btn.stop:active, .btn.stop.active {
    background: var(--danger);
    color: #fff;
    box-shadow: 0 0 20px rgba(239,68,68,.5);
  }

  /* grid positions */
  .btn-fwd   { grid-column: 2; grid-row: 1; }
  .btn-left  { grid-column: 1; grid-row: 2; }
  .btn-stop  { grid-column: 2; grid-row: 2; }
  .btn-right { grid-column: 3; grid-row: 2; }
  .btn-rev   { grid-column: 2; grid-row: 3; }

  /* ---- Sliders ---- */
  .sliders {
    width: 100%;
    max-width: calc(3 * var(--btn-size) + 24px);
    display: flex;
    flex-direction: column;
    gap: 14px;
  }
  .slider-row {
    display: flex;
    align-items: center;
    gap: 12px;
    font-size: .8rem;
    color: var(--muted);
  }
  .slider-row label { min-width: 70px; font-weight: 600; }
  .slider-row input[type=range] {
    flex: 1;
    accent-color: var(--accent);
    height: 4px;
    cursor: pointer;
  }
  .slider-val { min-width: 36px; text-align: right; color: var(--text); font-weight: 600; }

  /* ---- Settings link ---- */
  a.settings-link {
    font-size: .78rem;
    color: var(--muted);
    text-decoration: none;
    border-bottom: 1px dashed var(--border);
    padding-bottom: 2px;
    transition: color .2s;
  }
  a.settings-link:hover { color: var(--accent); }

  /* ---- Toast ---- */
  #toast {
    position: fixed;
    bottom: 24px;
    left: 50%;
    transform: translateX(-50%) translateY(80px);
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 40px;
    padding: 10px 24px;
    font-size: .82rem;
    color: var(--muted);
    transition: transform .3s ease, opacity .3s ease;
    opacity: 0;
    pointer-events: none;
  }
  #toast.show { transform: translateX(-50%) translateY(0); opacity: 1; }
</style>
</head>
<body>

<h1>&#x1F697; VehicleLab Controller</h1>

<div id="status-bar">
  Speed:&nbsp;<span id="stat-speed">0</span>
  &nbsp;|&nbsp;
  Turn:&nbsp;<span id="stat-turn">0&deg;</span>
</div>

<div class="ctrl-grid">
  <button class="btn btn-fwd"   id="btn-fwd"   title="Forward"  >&#x2B06;</button>
  <button class="btn btn-left"  id="btn-left"  title="Turn Left">&#x2B05;</button>
  <button class="btn btn-stop stop" id="btn-stop" title="Stop"  >STOP</button>
  <button class="btn btn-right" id="btn-right" title="Turn Right">&#x27A1;</button>
  <button class="btn btn-rev"   id="btn-rev"   title="Reverse"  >&#x2B07;</button>
</div>

<div class="sliders">
  <div class="slider-row">
    <label>Speed step</label>
    <input type="range" id="speed-step" min="5" max="50" value="20" step="5">
    <div class="slider-val" id="speed-step-val">20</div>
  </div>
  <div class="slider-row">
    <label>Turn step</label>
    <input type="range" id="turn-step" min="1" max="20" value="5" step="1">
    <div class="slider-val" id="turn-step-val">5&deg;</div>
  </div>
</div>

<div style="display:flex;gap:24px;">
  <a class="settings-link" href="/prefs">&#x1F3AE; Vehicle Prefs</a>
  <a class="settings-link" href="/settings">&#x2699; WiFi Settings</a>
</div>

<div id="toast"></div>

<script>
'use strict';

let currentSpeed = 0;
let currentTurn  = 0;

const statSpeed = document.getElementById('stat-speed');
const statTurn  = document.getElementById('stat-turn');
const toast     = document.getElementById('toast');
let toastTimer  = null;

function showToast(msg, ok = true) {
  toast.textContent = msg;
  toast.style.borderColor = ok ? 'var(--success)' : 'var(--danger)';
  toast.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove('show'), 1800);
}

async function api(url) {
  try {
    const r = await fetch(url);
    if (!r.ok) throw new Error(r.status);
    return await r.json();
  } catch(e) {
    showToast('Error: ' + e.message, false);
    return null;
  }
}

async function setSpeed(speed) {
  speed = Math.max(-100, Math.min(100, speed));
  const data = await api('/drive/' + speed);
  if (data) {
    currentSpeed = data.speed;
    statSpeed.textContent = currentSpeed;
    showToast('Speed: ' + currentSpeed);
  }
}

async function setTurn(deg) {
  deg = Math.max(-45, Math.min(45, deg));
  const data = await api('/turn/' + deg);
  if (data) {
    currentTurn = data.turn;
    statTurn.textContent = currentTurn + '\u00b0';
    showToast('Turn: ' + (currentTurn >= 0 ? '+' : '') + currentTurn + '\u00b0');
  }
}

async function stop() {
  const data = await api('/stop');
  if (data) {
    currentSpeed = 0;
    currentTurn  = 0;
    statSpeed.textContent = '0';
    statTurn.textContent  = '0\u00b0';
    showToast('Stopped');
  }
}

function getSpeedStep() { return parseInt(document.getElementById('speed-step').value, 10); }
function getTurnStep()  { return parseInt(document.getElementById('turn-step').value,  10); }

// Button events (touch + mouse)
function bindBtn(id, action) {
  const el = document.getElementById(id);
  function down(e) { e.preventDefault(); el.classList.add('active'); action(); }
  el.addEventListener('pointerdown', down);
}

bindBtn('btn-fwd',   () => setSpeed(currentSpeed + getSpeedStep()));
bindBtn('btn-rev',   () => setSpeed(currentSpeed - getSpeedStep()));
bindBtn('btn-left',  () => setTurn(currentTurn  - getTurnStep()));
bindBtn('btn-right', () => setTurn(currentTurn  + getTurnStep()));
bindBtn('btn-stop',  () => stop());

document.querySelectorAll('.btn').forEach(b =>
  b.addEventListener('pointerup', () => b.classList.remove('active'))
);

// Keyboard support
const keyMap = {
  'ArrowUp':    () => setSpeed(currentSpeed + getSpeedStep()),
  'ArrowDown':  () => setSpeed(currentSpeed - getSpeedStep()),
  'ArrowLeft':  () => setTurn(currentTurn  - getTurnStep()),
  'ArrowRight': () => setTurn(currentTurn  + getTurnStep()),
  ' ':          () => stop(),
};
document.addEventListener('keydown', e => {
  if (keyMap[e.key]) { e.preventDefault(); keyMap[e.key](); }
});

// Slider labels
document.getElementById('speed-step').addEventListener('input', function() {
  document.getElementById('speed-step-val').textContent = this.value;
});
document.getElementById('turn-step').addEventListener('input', function() {
  document.getElementById('turn-step-val').textContent = this.value + '\u00b0';
});

// Poll status every 3 s
async function pollStatus() {
  const data = await api('/status');
  if (data) {
    currentSpeed = data.speed;
    currentTurn  = data.turn;
    statSpeed.textContent = currentSpeed;
    statTurn.textContent  = currentTurn + '\u00b0';
  }
}
setInterval(pollStatus, 3000);
pollStatus();
</script>
</body>
</html>
)rawhtml";

// ============================================================
// /settings  –  WiFi configuration page
// ============================================================
static const char SETTINGS_UI[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>VehicleLab – WiFi Settings</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap');
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
  :root {
    --bg:      #0d0f14;
    --surface: #161b26;
    --border:  #2a3040;
    --accent:  #4f8ef7;
    --danger:  #ef4444;
    --text:    #e4e8f0;
    --muted:   #6b7a99;
    --radius:  16px;
  }
  body {
    min-height: 100vh;
    background: var(--bg);
    color: var(--text);
    font-family: 'Inter', sans-serif;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
  }
  .card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 36px 40px;
    width: 100%;
    max-width: 440px;
    display: flex;
    flex-direction: column;
    gap: 28px;
  }
  h1 {
    font-size: 1.3rem;
    font-weight: 700;
    background: linear-gradient(135deg, #4f8ef7, #8b5cf6);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }
  .info-row {
    display: flex;
    justify-content: space-between;
    font-size: .82rem;
    color: var(--muted);
    border-bottom: 1px solid var(--border);
    padding-bottom: 16px;
  }
  .info-row span { color: var(--text); font-weight: 600; }
  fieldset {
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 20px;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }
  legend {
    font-size: .78rem;
    font-weight: 600;
    color: var(--muted);
    padding: 0 8px;
    letter-spacing: .06em;
    text-transform: uppercase;
  }
  label {
    display: flex;
    flex-direction: column;
    gap: 6px;
    font-size: .82rem;
    color: var(--muted);
    font-weight: 600;
  }
  input[type=text], input[type=password] {
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 10px;
    padding: 10px 14px;
    color: var(--text);
    font-family: inherit;
    font-size: .92rem;
    outline: none;
    transition: border-color .2s;
  }
  input:focus { border-color: var(--accent); }
  .btn-row { display: flex; gap: 12px; }
  button {
    flex: 1;
    padding: 12px;
    border-radius: 10px;
    border: none;
    font-family: inherit;
    font-size: .9rem;
    font-weight: 600;
    cursor: pointer;
    transition: opacity .2s, transform .1s;
  }
  button:active { transform: scale(0.97); }
  .btn-save  { background: var(--accent); color: #fff; }
  .btn-clear { background: rgba(239,68,68,.15); border: 1px solid var(--danger); color: var(--danger); }
  .back-link {
    font-size: .78rem;
    color: var(--muted);
    text-decoration: none;
    text-align: center;
    border-bottom: 1px dashed var(--border);
    padding-bottom: 2px;
    align-self: center;
    transition: color .2s;
  }
  .back-link:hover { color: var(--accent); }
</style>
</head>
<body>
<div class="card">
  <h1>&#x2699; WiFi Settings</h1>

  <div class="info-row">
    <div>Mode: <span>%MODE%</span></div>
    <div>SSID: <span>%SSID%</span></div>
    <div>IP: <span>%IP%</span></div>
  </div>

  <form method="POST" action="/settings">
    <fieldset>
      <legend>Network Credentials</legend>
      <label>
        SSID
        <input type="text" name="ssid" placeholder="Your WiFi network name" autocomplete="off" spellcheck="false">
      </label>
      <label>
        Password
        <input type="password" name="password" placeholder="Password" autocomplete="off">
      </label>
      <div class="btn-row">
        <button type="submit" class="btn-save">Save &amp; Reboot</button>
        <button type="submit" formaction="/settings/clear" class="btn-clear">Reset to AP</button>
      </div>
    </fieldset>
  </form>

  <a class="back-link" href="/drive">&#x2190; Back to Controller</a>
</div>
</body>
</html>
)rawhtml";

// ============================================================
// /prefs  –  Vehicle preferences page
// ============================================================
static const char PREFS_UI[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>VehicleLab – Vehicle Preferences</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap');
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
  :root {
    --bg:      #0d0f14;
    --surface: #161b26;
    --border:  #2a3040;
    --accent:  #4f8ef7;
    --accent2: #8b5cf6;
    --danger:  #ef4444;
    --success: #22c55e;
    --text:    #e4e8f0;
    --muted:   #6b7a99;
    --radius:  16px;
  }
  body {
    min-height: 100vh;
    background: var(--bg);
    color: var(--text);
    font-family: 'Inter', sans-serif;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
  }
  .card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 36px 40px;
    width: 100%;
    max-width: 460px;
    display: flex;
    flex-direction: column;
    gap: 28px;
  }
  h1 {
    font-size: 1.3rem;
    font-weight: 700;
    background: linear-gradient(135deg, #4f8ef7, #8b5cf6);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }
  fieldset {
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 20px;
    display: flex;
    flex-direction: column;
    gap: 20px;
  }
  legend {
    font-size: .78rem;
    font-weight: 600;
    color: var(--muted);
    padding: 0 8px;
    letter-spacing: .06em;
    text-transform: uppercase;
  }
  .slider-row {
    display: flex;
    align-items: center;
    gap: 12px;
    font-size: .82rem;
  }
  .slider-row label { min-width: 84px; font-weight: 600; color: var(--muted); }
  .slider-row input[type=range] {
    flex: 1;
    accent-color: var(--accent);
    height: 4px;
    cursor: pointer;
  }
  .slider-val {
    min-width: 44px;
    text-align: right;
    font-weight: 700;
    color: var(--text);
  }
  .hint {
    font-size: .75rem;
    color: var(--muted);
    line-height: 1.5;
  }
  button {
    width: 100%;
    padding: 13px;
    border-radius: 10px;
    border: none;
    font-family: inherit;
    font-size: .95rem;
    font-weight: 600;
    cursor: pointer;
    background: var(--accent);
    color: #fff;
    transition: opacity .2s, transform .1s;
  }
  button:active { transform: scale(0.97); }
  button:disabled { opacity: .4; cursor: default; }
  .link-row {
    display: flex;
    justify-content: center;
    gap: 28px;
  }
  a.nav-link {
    font-size: .78rem;
    color: var(--muted);
    text-decoration: none;
    border-bottom: 1px dashed var(--border);
    padding-bottom: 2px;
    transition: color .2s;
  }
  a.nav-link:hover { color: var(--accent); }
  #toast {
    position: fixed;
    bottom: 24px;
    left: 50%;
    transform: translateX(-50%) translateY(80px);
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 40px;
    padding: 10px 24px;
    font-size: .82rem;
    color: var(--muted);
    transition: transform .3s ease, opacity .3s ease;
    opacity: 0;
    pointer-events: none;
  }
  #toast.show { transform: translateX(-50%) translateY(0); opacity: 1; }
</style>
</head>
<body>
<div class="card">
  <h1>&#x1F3AE; Vehicle Preferences</h1>

  <fieldset>
    <legend>Steering Limits</legend>

    <div class="slider-row">
      <label>Min Steer</label>
      <input type="range" id="steer-min" min="-45" max="0" value="%STEER_MIN%" step="1">
      <div class="slider-val" id="steer-min-val">%STEER_MIN%&deg;</div>
    </div>

    <div class="slider-row">
      <label>Max Steer</label>
      <input type="range" id="steer-max" min="0" max="45" value="%STEER_MAX%" step="1">
      <div class="slider-val" id="steer-max-val">%STEER_MAX%&deg;</div>
    </div>

    <p class="hint">Limits how far the steering servo travels. Reduce these for cars with less physical lock-to-lock range. Changes apply immediately &ndash; no reboot required.</p>
  </fieldset>

  <button id="btn-save">Save Preferences</button>

  <div class="link-row">
    <a class="nav-link" href="/drive">&#x2190; Controller</a>
    <a class="nav-link" href="/settings">&#x2699; WiFi Settings</a>
  </div>
</div>

<div id="toast"></div>

<script>
'use strict';
const minSlider = document.getElementById('steer-min');
const maxSlider = document.getElementById('steer-max');
const minVal    = document.getElementById('steer-min-val');
const maxVal    = document.getElementById('steer-max-val');
const btnSave   = document.getElementById('btn-save');
const toast     = document.getElementById('toast');
let toastTimer;

minSlider.addEventListener('input', () => { minVal.textContent = minSlider.value + '\u00b0'; });
maxSlider.addEventListener('input', () => { maxVal.textContent = maxSlider.value + '\u00b0'; });

function showToast(msg, ok) {
  toast.textContent = msg;
  toast.style.borderColor = ok ? 'var(--success)' : 'var(--danger)';
  toast.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove('show'), 2000);
}

btnSave.addEventListener('click', async () => {
  btnSave.disabled = true;
  try {
    const r = await fetch('/prefs', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'steer_min=' + minSlider.value + '&steer_max=' + maxSlider.value
    });
    const data = await r.json();
    if (data.ok) {
      showToast('Saved! (' + data.steer_min + '\u00b0 \u2026 +' + data.steer_max + '\u00b0)', true);
    } else {
      showToast('Error: ' + (data.error || 'unknown'), false);
    }
  } catch(e) {
    showToast('Request failed', false);
  }
  btnSave.disabled = false;
});
</script>
</body>
</html>
)rawhtml";

// ============================================================
// Route registration
// ============================================================
void web_server_begin() {

    // ---- GET /prefs  (vehicle preferences page) ----------------------------
    server.on("/prefs", HTTP_GET, [](AsyncWebServerRequest* req) {
        AsyncResponseStream* resp = req->beginResponseStream("text/html");
        String page = FPSTR(PREFS_UI);
        page.replace("%STEER_MIN%", String(vehicle_get_steer_min()));
        page.replace("%STEER_MAX%", String(vehicle_get_steer_max()));
        resp->print(page);
        req->send(resp);
    });

    // ---- POST /prefs  (save vehicle preferences) ---------------------------
    server.on("/prefs", HTTP_POST, [](AsyncWebServerRequest* req) {
        int min_deg = vehicle_get_steer_min();
        int max_deg = vehicle_get_steer_max();
        if (req->hasParam("steer_min", true))
            min_deg = req->getParam("steer_min", true)->value().toInt();
        if (req->hasParam("steer_max", true))
            max_deg = req->getParam("steer_max", true)->value().toInt();

        if (min_deg >= max_deg) {
            json_ok(req, "{\"ok\":false,\"error\":\"min must be less than max\"}");
            return;
        }
        vehicle_set_steer_limits(min_deg, max_deg);
        String body = "{\"ok\":true,\"steer_min\":" + String(vehicle_get_steer_min()) +
                      ",\"steer_max\":"              + String(vehicle_get_steer_max()) + "}";
        json_ok(req, body);
    });

    // ---- GET /ping ---------------------------------------------------------
    server.on("/ping", HTTP_GET, [](AsyncWebServerRequest* req) {
        json_ok(req, "{\"status\":\"ok\"}");
    });

    // ---- GET /status -------------------------------------------------------
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        String body = "{\"speed\":" + String(vehicle_get_speed()) +
                      ",\"turn\":"  + String(vehicle_get_turn())  + "}";
        json_ok(req, body);
    });

    // ---- GET /drive  (HTML controller UI) ----------------------------------
    server.on("/drive", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", DRIVE_UI);
    });

    // ---- GET /drive/<speed> ------------------------------------------------
    server.on("^/drive/(-?[0-9]+)$", HTTP_GET, [](AsyncWebServerRequest* req) {
        int speed = req->pathArg(0).toInt();
        vehicle_set_speed(speed);
        String body = "{\"speed\":" + String(vehicle_get_speed()) + "}";
        json_ok(req, body);
    });

    // ---- GET /turn/<degrees> -----------------------------------------------
    server.on("^/turn/(-?[0-9]+)$", HTTP_GET, [](AsyncWebServerRequest* req) {
        int deg = req->pathArg(0).toInt();
        vehicle_set_turn(deg);
        String body = "{\"turn\":" + String(vehicle_get_turn()) + "}";
        json_ok(req, body);
    });

    // ---- GET /stop ---------------------------------------------------------
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest* req) {
        vehicle_stop();
        json_ok(req, "{\"status\":\"stopped\",\"speed\":0,\"turn\":0}");
    });

    // ---- GET /settings  (HTML form) ----------------------------------------
    // NOTE: We must NOT pass a local String to req->send() – ESPAsyncWebServer
    // is fully async and will read the buffer after this lambda returns, causing
    // a use-after-free / hang.  Use AsyncResponseStream so the library owns the
    // buffer for the entire lifetime of the async response.
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* req) {
        AsyncResponseStream* resp = req->beginResponseStream("text/html");
        String page = FPSTR(SETTINGS_UI);
        page.replace("%MODE%", wifi_manager_is_sta_mode() ? "Station" : "Access Point");
        page.replace("%SSID%", wifi_manager_current_ssid());
        page.replace("%IP%",   wifi_manager_local_ip());
        resp->print(page);
        req->send(resp);
    });

    // ---- POST /settings  (save credentials) --------------------------------
    server.on("/settings", HTTP_POST, [](AsyncWebServerRequest* req) {
        String ssid, password;
        if (req->hasParam("ssid", true))     ssid     = req->getParam("ssid",     true)->value();
        if (req->hasParam("password", true)) password = req->getParam("password", true)->value();

        // Send the response *before* rebooting
        req->send(200, "text/html",
            "<html><body style='font-family:sans-serif;background:#0d0f14;color:#e4e8f0;"
            "display:flex;align-items:center;justify-content:center;height:100vh;'>"
            "<p>Credentials saved. Rebooting&hellip;</p></body></html>");

        wifi_manager_save_and_reboot(ssid, password);
    });

    // ---- POST /settings/clear  (reset to AP mode) --------------------------
    server.on("/settings/clear", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html",
            "<html><body style='font-family:sans-serif;background:#0d0f14;color:#e4e8f0;"
            "display:flex;align-items:center;justify-content:center;height:100vh;'>"
            "<p>WiFi credentials cleared. Rebooting into AP mode&hellip;</p></body></html>");
        wifi_manager_clear_credentials();
    });

    // ---- Root redirect to /drive -------------------------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("/drive");
    });

    // ---- 404 ---------------------------------------------------------------
    server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    });

    server.begin();
    Serial.println("[web] HTTP server started on port 80");
}
