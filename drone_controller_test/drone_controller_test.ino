//This is is the main controlling part of the tx as a UI on web hosting using Access point via esp32
//The for now to camera feed is unavailable due to no on board camera . Future scope for using the ESPcam development board 

#include<WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

const char* AP_SSID = "Test001";
const char* AP_PASS = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
volatile float control[4];

//webUI interface of the controller

const char index_html[] PROGMEM = R"rawliteral(
  
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
<title>ESP32 Drone Controller</title>
<style>
  :root {
    --bg: #0b1020;
    --card: rgba(15, 23, 36, 0.8);
    --accent: #21a1f1;
    --danger: #ff4757;
    --muted: #9aa4b2;
    --stick-size: 180px;
  }
 
  * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
 
  html, body {
    height: 100%;
    margin: 0;
    overflow: hidden;
    font-family: system-ui, -apple-system, sans-serif;
    background: var(--bg);
    color: #e6eef8;
  }
 
  .app {
    display: grid;
    grid-template-areas:
      "header header header"
      "left   center right"
      "footer footer footer";
    grid-template-columns: var(--stick-size) 1fr var(--stick-size);
    grid-template-rows: auto 1fr auto;
    height: 100vh;
    height: 100dvh;
    padding: 10px;
    gap: 10px;
  }
 
  header {
    grid-area: header;
    display: flex;
    justify-content: space-between;
    align-items: center;
    z-index: 10;
  }
 
  .title { font-weight: 700; font-size: 1.1rem; letter-spacing: 0.5px; }
 
  .stick-container {
    display: flex;
    align-items: center;
    justify-content: center;
  }
  .area-left { grid-area: left; }
  .area-right { grid-area: right; }
 
  .stick {
    width: var(--stick-size);
    height: var(--stick-size);
    background: rgba(255,255,255,0.05);
    border: 2px solid rgba(255,255,255,0.1);
    border-radius: 50%;
    position: relative;
    touch-action: none;
    user-select: none;
    -webkit-user-select: none;
  }
 
  .thumb {
    position: absolute;
    width: 60px;
    height: 60px;
    background: var(--accent);
    border-radius: 50%;
    transform: translate(-50%, -50%);
    box-shadow: 0 0 15px var(--accent);
    pointer-events: none;
  }
 
  .area-center {
    grid-area: center;
    position: relative;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    background: #000;
    border-radius: 12px;
    overflow: hidden;
  }
 
  .video-feed {
    width: 100%;
    height: 100%;
    object-fit: contain;
  }
 
  .overlay-telemetry {
    position: absolute;
    top: 10px;
    left: 10px;
    right: 10px;
    display: flex;
    justify-content: space-between;
    pointer-events: none;
    font-size: 0.85rem;
    text-shadow: 1px 1px 2px #000;
  }
 
  .center-actions {
    position: absolute;
    bottom: 15px;
    display: flex;
    gap: 10px;
  }
 
  button {
    background: var(--card);
    border: 1px solid rgba(255,255,255,0.2);
    color: white;
    padding: 8px 16px;
    border-radius: 8px;
    font-weight: 600;
    cursor: pointer;
    backdrop-filter: blur(4px);
  }
 
  .btn-arm { border-color: var(--danger); color: var(--danger); }
  .btn-arm.armed { background: var(--danger); color: white; }
 
  footer {
    grid-area: footer;
    font-size: 11px;
    text-align: center;
    color: var(--muted);
  }
 
  .status-dot {
    display: inline-block;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #ccc;
    margin-right: 5px;
  }
  .online { background: #2ecc71; box-shadow: 0 0 5px #2ecc71; }
  .offline { background: #e74c3c; }
 
  @media (orientation: portrait) {
    body::after {
      content: "Please rotate your device to landscape";
      position: fixed;
      inset: 0;
      background: var(--bg);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 9999;
      padding: 20px;
      text-align: center;
    }
  }
</style>
</head>
<body>
 
<div class="app">
  <header>
    <div class="title">DRONE PILOT</div>
    <div style="display:flex; gap:15px; align-items:center">
      <span id="battery">--V</span>
      <button id="fsBtn" style="padding:4px 8px; font-size:10px;">FS</button>
      <div><span id="dot" class="status-dot offline"></span><span id="conn">DISCONNECTED</span></div>
    </div>
  </header>
 
  <div class="stick-container area-left">
    <div id="leftStick" class="stick">
      <div id="leftThumb" class="thumb" style="left:50%; top:50%"></div>
    </div>
  </div>
 
  <div class="area-center">
    <div class="overlay-telemetry">
      <div>Alt: <span id="alt">0.0</span>m</div>
      <div>Spd: <span id="speed">0.0</span>m/s</div>
      <div>RSSI: <span id="rssi">--</span>dBm</div>
    </div>
 
    <img id="camera" class="video-feed" src="/camera" alt="Live Feed" onerror="this.onerror=null; this.style.display='none'">
 
    <div class="center-actions">
      <button id="armBtn" class="btn-arm">ARM</button>
      <button id="takeoffBtn">TAKEOFF</button>
      <button id="landBtn">LAND</button>
    </div>
  </div>
 
  <div class="stick-container area-right">
    <div id="rightStick" class="stick">
      <div id="rightThumb" class="thumb" style="left:50%; top:50%"></div>
    </div>
  </div>
 
  <footer>
    Sens: <input id="sens" type="range" min="0.5" max="2" step="0.1" value="1" style="vertical-align:middle; width:100px;">
    &nbsp; | &nbsp; WebSocket: /ws
  </footer>
</div>
 
<script>
var connEl = document.getElementById('conn');
var dotEl = document.getElementById('dot');
var armBtn = document.getElementById('armBtn');
var ws = null;
var isArmed = false;
 
function setArmedUI(v) {
  isArmed = v;
  armBtn.textContent = v ? 'DISARM' : 'ARM';
  armBtn.classList.toggle('armed', v);
}
 
function connect() {
  ws = new WebSocket('ws://' + location.host + '/ws');
 
  ws.onopen = function () {
    connEl.textContent = 'CONNECTED';
    dotEl.className = 'status-dot online';
  };
 
  ws.onclose = function () {
    connEl.textContent = 'DISCONNECTED';
    dotEl.className = 'status-dot offline';
    setArmedUI(false);
    setTimeout(connect, 1000);
  };
 
  ws.onmessage = function (ev) {
    try {
      var j = JSON.parse(ev.data);
      if (j.type === 'telemetry') updateTelemetry(j);
    } catch (e) {}
  };
}
 
function sendJSON(o) {
  if (ws && ws.readyState === 1) ws.send(JSON.stringify(o));
}
 
function updateTelemetry(t) {
  if (t.battery !== undefined) document.getElementById('battery').textContent = t.battery.toFixed(1) + 'V';
  if (t.alt !== undefined) document.getElementById('alt').textContent = t.alt.toFixed(1);
  if (t.speed !== undefined) document.getElementById('speed').textContent = t.speed.toFixed(1);
  if (t.rssi !== undefined) document.getElementById('rssi').textContent = t.rssi;
}
 
/* Multi-touch sticks: each stick locks onto the finger that started on it */
var allSticks = [];
 
function makeStick(stickEl, thumbEl, onChange) {
  var activeId = null;
  var r = null;
 
  function update(e) {
    var maxRadius = r.width / 2;
    var dx = e.clientX - (r.left + r.width / 2);
    var dy = e.clientY - (r.top + r.height / 2);
    var dist = Math.hypot(dx, dy);
    if (dist > maxRadius) {
      dx *= maxRadius / dist;
      dy *= maxRadius / dist;
    }
    thumbEl.style.left = (r.width / 2 + dx) + 'px';
    thumbEl.style.top = (r.height / 2 + dy) + 'px';
    onChange(dx / maxRadius, dy / maxRadius);
  }
 
  function release() {
    if (activeId === null) return;
    activeId = null;
    r = null;
    thumbEl.style.left = '50%';
    thumbEl.style.top = '50%';
    onChange(0, 0);
    sendSticks();
  }
 
  stickEl.addEventListener('pointerdown', function (e) {
    if (activeId !== null) return;
    activeId = e.pointerId;
    r = stickEl.getBoundingClientRect();
    stickEl.setPointerCapture(e.pointerId);
    update(e);
    e.preventDefault();
  });
 
  stickEl.addEventListener('pointermove', function (e) {
    if (e.pointerId !== activeId) return;
    update(e);
    e.preventDefault();
  });
 
  function end(e) {
    if (e.pointerId !== activeId) return;
    release();
  }
  stickEl.addEventListener('pointerup', end);
  stickEl.addEventListener('pointercancel', end);
  stickEl.addEventListener('lostpointercapture', end);
 
  allSticks.push(release);
}
 
function releaseAllSticks() { allSticks.forEach(function (fn) { fn(); }); }
window.addEventListener('blur', releaseAllSticks);
document.addEventListener('visibilitychange', function () { if (document.hidden) releaseAllSticks(); });
 
var left = { x: 0, y: 0 };
var right = { x: 0, y: 0 };
var sensEl = document.getElementById('sens');
 
makeStick(document.getElementById('leftStick'), document.getElementById('leftThumb'), function (nx, ny) {
  left.x = nx; left.y = -ny;
});
 
makeStick(document.getElementById('rightStick'), document.getElementById('rightThumb'), function (nx, ny) {
  right.x = nx; right.y = -ny;
});
 
function sendSticks() {
  var s = parseFloat(sensEl.value) || 1;
  sendJSON({
    type: 'cmd',
    id: 'stick',
    payload: {
      lx: +(left.x * s).toFixed(3),
      ly: +(left.y * s).toFixed(3),
      rx: +(right.x * s).toFixed(3),
      ry: +(right.y * s).toFixed(3)
    }
  });
}
 
/* Fixed-rate send (20 Hz). Also acts as a heartbeat so the ESP32
   failsafe does not trigger while a stick is held still. */
setInterval(sendSticks, 50);
 
document.getElementById('fsBtn').addEventListener('click', function () {
  if (!document.fullscreenElement) {
    document.documentElement.requestFullscreen().catch(function () {});
  } else {
    document.exitFullscreen();
  }
});
 
armBtn.addEventListener('click', function () {
  setArmedUI(!isArmed);
  sendJSON({ type: 'action', id: 'toggle_arm', value: isArmed });
});
 
document.getElementById('takeoffBtn').addEventListener('click', function () {
  sendJSON({ type: 'action', id: 'takeoff' });
});
document.getElementById('landBtn').addEventListener('click', function () {
  sendJSON({ type: 'action', id: 'land' });
});
 
document.addEventListener('touchmove', function (e) {
  if (e.touches.length > 1) e.preventDefault();
}, { passive: false });
 
connect();
</script>
</body>
</html>
 
)rawliteral";

void onWsEvent(AsyncWebSocket *server,
               AsyncWebSocketClient *client,
               AwsEventType type,
               void *arg,
               uint8_t *data,
               size_t len){
  if(type == WS_EVT_DATA){
    AwsFrameInfo *info = (AwsFrameInfo*)arg;

    if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
      JsonDocument doc;

      if (deserializeJson(doc, data, len)) {
        Serial.println("Invalid JSON");
        return;
      }

      JsonObject payload = doc["payload"];

      control[0] = payload["lx"].as<float>();
      control[1] = payload["ly"].as<float>();
      control[2] = payload["rx"].as<float>();
      control[3] = payload["ry"].as<float>();
      Serial.println("data:");
      serializeJson(doc, Serial);
      Serial.println();
      Serial.printf(
                  "LX: %.3f | LY: %.3f | RX: %.3f | RY: %.3f\n",
                  control[0],
                  control[1],
                  control[2],
                  control[3]
                  );
    }
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASS);
  Serial.println(WiFi.localIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html",index_html);
  });
  server.begin();

  Serial.println("WebSocket ready at /ws");

}

void loop() {
  // put your main code here, to run repeatedly:
  ws.cleanupClients();
}
