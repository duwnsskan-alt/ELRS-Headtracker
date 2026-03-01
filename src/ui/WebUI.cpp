#include "WebUI.h"
#include "config/Config.h"
#include "config/Settings.h"
#include "sensor/Orientation.h"
#include "SharedState.h"
#include "LEDController.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Update.h>

// ── 정적 멤버 ───────────────────────────────────────────────────
bool WebUI::_running = false;

static AsyncWebServer server(WEBUI_PORT);

// ═══════════════════════════════════════════════════════════════
//  HTML  —  단일 파일 SPA (Settings / Bind / Debug 3D)
// ═══════════════════════════════════════════════════════════════
// PROGMEM에 저장해 RAM 절약
static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HeadTracker</title>
<style>
  :root{--bg:#0f1117;--bg2:#1a1d2e;--bg3:#252840;--acc:#00c896;--acc2:#3d7fff;
        --warn:#ffb300;--err:#ff4d4d;--txt:#e0e4f0;--txt2:#7a8099;--border:#2d3150;}
  *{box-sizing:border-box;margin:0;padding:0;}
  body{background:var(--bg);color:var(--txt);font:14px/1.5 'Segoe UI',system-ui,sans-serif;min-height:100vh;}
  /* ── Header ── */
  header{display:flex;align-items:center;justify-content:space-between;
         padding:12px 20px;background:var(--bg2);border-bottom:1px solid var(--border);}
  header h1{font-size:16px;font-weight:700;letter-spacing:.5px;color:var(--acc);}
  .status-pill{display:flex;align-items:center;gap:6px;font-size:12px;color:var(--txt2);}
  .dot{width:8px;height:8px;border-radius:50%;background:var(--err);}
  .dot.ok{background:var(--acc);}
  /* ── Tabs ── */
  .tabs{display:flex;gap:2px;padding:8px 16px 0;background:var(--bg2);}
  .tab{padding:8px 20px;border-radius:6px 6px 0 0;cursor:pointer;
       font-size:13px;color:var(--txt2);border:1px solid transparent;
       border-bottom:none;transition:.15s;}
  .tab.active{background:var(--bg);color:var(--txt);border-color:var(--border);}
  .tab:hover:not(.active){color:var(--txt);}
  /* ── Content ── */
  .content{padding:20px;max-width:800px;}
  .pane{display:none;}.pane.active{display:block;}
  /* ── Card ── */
  .card{background:var(--bg2);border:1px solid var(--border);border-radius:10px;
        padding:16px;margin-bottom:14px;}
  .card h2{font-size:13px;font-weight:600;color:var(--txt2);
           text-transform:uppercase;letter-spacing:.8px;margin-bottom:14px;}
  /* ── Form elements ── */
  .row{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:10px;}
  .row.triple{grid-template-columns:1fr 1fr 1fr;}
  label{display:block;font-size:12px;color:var(--txt2);margin-bottom:4px;}
  input[type=number],select{width:100%;background:var(--bg3);border:1px solid var(--border);
    border-radius:6px;padding:7px 10px;color:var(--txt);font-size:14px;outline:none;}
  input[type=number]:focus,select:focus{border-color:var(--acc);}
  .toggle-row{display:flex;align-items:center;justify-content:space-between;
              padding:8px 0;border-bottom:1px solid var(--border);}
  .toggle-row:last-child{border-bottom:none;}
  .toggle-row span{font-size:13px;}
  /* toggle switch */
  .toggle{position:relative;width:38px;height:20px;cursor:pointer;}
  .toggle input{opacity:0;width:0;height:0;}
  .slider{position:absolute;inset:0;background:var(--bg3);border-radius:20px;transition:.2s;}
  .slider:before{content:'';position:absolute;width:14px;height:14px;left:3px;bottom:3px;
    background:#fff;border-radius:50%;transition:.2s;}
  input:checked+.slider{background:var(--acc);}
  input:checked+.slider:before{transform:translateX(18px);}
  /* ── Buttons ── */
  .btn{padding:8px 18px;border-radius:6px;border:none;cursor:pointer;
       font-size:13px;font-weight:600;transition:.15s;}
  .btn-primary{background:var(--acc);color:#000;}
  .btn-primary:hover{filter:brightness(1.15);}
  .btn-danger{background:var(--err);color:#fff;}
  .btn-secondary{background:var(--bg3);color:var(--txt);border:1px solid var(--border);}
  .btn-secondary:hover{border-color:var(--acc);color:var(--acc);}
  .btn-row{display:flex;gap:8px;flex-wrap:wrap;margin-top:14px;}
  .btn:disabled{opacity:.4;cursor:not-allowed;}
  /* ── Bind tab ── */
  .peer-list{display:flex;flex-direction:column;gap:8px;margin:10px 0;}
  .peer-item{display:flex;align-items:center;justify-content:space-between;
             background:var(--bg3);border:1px solid var(--border);
             border-radius:8px;padding:10px 14px;cursor:pointer;transition:.15s;}
  .peer-item:hover{border-color:var(--acc);}
  .peer-item.selected{border-color:var(--acc);background:#00c89612;}
  .peer-mac{font-family:monospace;font-size:14px;}
  .peer-rssi{font-size:12px;color:var(--txt2);}
  .badge{display:inline-block;padding:2px 8px;border-radius:12px;font-size:11px;}
  .badge-ok{background:#00c89620;color:var(--acc);}
  .badge-warn{background:#ffb30020;color:var(--warn);}
  /* ── Dashboard 3D ── */
  #canvas3d{width:100%;height:320px;border-radius:10px;background:var(--bg3);
            display:block;border:1px solid var(--border);}
  .gauge-row{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-top:12px;}
  .gauge{background:var(--bg3);border:1px solid var(--border);border-radius:8px;padding:12px;}
  .gauge-label{font-size:11px;color:var(--txt2);text-transform:uppercase;letter-spacing:.6px;}
  .gauge-val{font-size:22px;font-weight:700;margin:4px 0;}
  .gauge-bar{height:6px;background:var(--bg2);border-radius:3px;margin-top:4px;overflow:hidden;}
  .gauge-fill{height:100%;border-radius:3px;background:var(--acc);transition:width .1s;}
  /* ── Profile selector ── */
  .profile-btns{display:flex;gap:6px;}
  .profile-btn{flex:1;padding:7px;border-radius:6px;border:1px solid var(--border);
               background:var(--bg3);color:var(--txt2);cursor:pointer;text-align:center;
               font-size:13px;transition:.15s;}
  .profile-btn.active{border-color:var(--acc);color:var(--acc);background:#00c89612;}
  /* ── Notification ── */
  #toast{position:fixed;bottom:20px;right:20px;padding:10px 18px;border-radius:8px;
         background:var(--bg2);border:1px solid var(--border);font-size:13px;
         opacity:0;transition:.3s;pointer-events:none;z-index:999;}
  #toast.show{opacity:1;}
  #toast.ok{border-color:var(--acc);color:var(--acc);}
  #toast.err{border-color:var(--err);color:var(--err);}
</style>
</head>
<body>
<header>
  <h1>⚡ HeadTracker</h1>
  <div class="status-pill">
    <div class="dot" id="sensorDot"></div>
    <span id="sensorLbl">Sensor</span>
    <div class="dot" style="margin-left:8px" id="espnowDot"></div>
    <span id="espnowLbl">ESP-NOW</span>
  </div>
</header>

<div class="tabs">
  <div class="tab active" onclick="switchTab('settings')">Settings</div>
  <div class="tab" onclick="switchTab('bind')">Bind</div>
  <div class="tab" onclick="switchTab('debug')">Debug 3D</div>
</div>

<!-- ════════════════ SETTINGS TAB ════════════════ -->
<div class="content">
<div class="pane active" id="pane-settings">

  <div class="card">
    <h2>Profile</h2>
    <div class="profile-btns">
      <div class="profile-btn" id="p0" onclick="setProfile(0)">Profile 1</div>
      <div class="profile-btn" id="p1" onclick="setProfile(1)">Profile 2</div>
      <div class="profile-btn" id="p2" onclick="setProfile(2)">Profile 3</div>
    </div>
  </div>

  <div class="card">
    <h2>Angle Range (degrees)</h2>
    <div class="row triple">
      <div><label>Pan max</label><input type="number" id="maxPan" min="10" max="180" step="5"></div>
      <div><label>Tilt max</label><input type="number" id="maxTilt" min="10" max="90" step="5"></div>
      <div><label>Roll max</label><input type="number" id="maxRoll" min="10" max="90" step="5"></div>
    </div>
  </div>

  <div class="card">
    <h2>Invert Axis</h2>
    <div class="toggle-row">
      <span>Invert Pan</span>
      <label class="toggle"><input type="checkbox" id="invPan"><span class="slider"></span></label>
    </div>
    <div class="toggle-row">
      <span>Invert Tilt</span>
      <label class="toggle"><input type="checkbox" id="invTilt"><span class="slider"></span></label>
    </div>
    <div class="toggle-row">
      <span>Invert Roll</span>
      <label class="toggle"><input type="checkbox" id="invRoll"><span class="slider"></span></label>
    </div>
  </div>

  <div class="card">
    <h2>Communication Mode</h2>
    <div class="toggle-row">
      <span>Mode</span>
      <select id="commsMode" style="width:140px">
        <option value="0">ESP-NOW</option>
        <option value="1">SBUS</option>
      </select>
    </div>
  </div>

  <div class="btn-row">
    <button class="btn btn-primary" onclick="saveConfig()">Save</button>
    <button class="btn btn-secondary" onclick="sendCenter()">Center Reset</button>
    <button class="btn btn-danger" onclick="factoryReset()">Factory Reset</button>
  </div>

</div>

<!-- ════════════════ BIND TAB ════════════════ -->
<div class="pane" id="pane-bind">

  <div class="card">
    <h2>Current Binding</h2>
    <div style="display:flex;align-items:center;justify-content:space-between">
      <div>
        <div style="font-family:monospace;font-size:16px" id="boundMac">—</div>
        <div style="font-size:12px;color:var(--txt2);margin-top:4px" id="boundStatus">Not bound (broadcast)</div>
      </div>
      <button class="btn btn-secondary" onclick="unbind()">Unbind</button>
    </div>
  </div>

  <div class="card">
    <h2>Scan for ELRS Backpack</h2>
    <p style="font-size:12px;color:var(--txt2);margin-bottom:12px">
      Press <b>Scan</b> — detected ESP-NOW peers appear below.<br>
      Select your TX module's Backpack and press <b>Bind</b>.
    </p>
    <button class="btn btn-primary" id="scanBtn" onclick="startScan()">Scan</button>
    <div class="peer-list" id="peerList"></div>
    <button class="btn btn-primary" id="bindBtn" onclick="doBindSelected()" disabled>Bind Selected</button>
  </div>

</div>

<!-- ════════════════ DEBUG 3D TAB ════════════════ -->
<div class="pane" id="pane-debug">

  <canvas id="canvas3d"></canvas>

  <div class="gauge-row">
    <div class="gauge">
      <div class="gauge-label">Pan</div>
      <div class="gauge-val" id="gPan">0°</div>
      <div class="gauge-bar"><div class="gauge-fill" id="gPanBar" style="width:50%"></div></div>
      <div style="font-size:11px;color:var(--txt2);margin-top:4px">CH: <span id="gChPan">992</span></div>
    </div>
    <div class="gauge">
      <div class="gauge-label">Tilt</div>
      <div class="gauge-val" id="gTilt">0°</div>
      <div class="gauge-bar"><div class="gauge-fill" id="gTiltBar" style="width:50%"></div></div>
      <div style="font-size:11px;color:var(--txt2);margin-top:4px">CH: <span id="gChTilt">992</span></div>
    </div>
    <div class="gauge">
      <div class="gauge-label">Roll</div>
      <div class="gauge-val" id="gRoll">0°</div>
      <div class="gauge-bar"><div class="gauge-fill" id="gRollBar" style="width:50%"></div></div>
      <div style="font-size:11px;color:var(--txt2);margin-top:4px">CH: <span id="gChRoll">992</span></div>
    </div>
  </div>

  <div class="card" style="margin-top:12px">
    <h2>Statistics</h2>
    <div class="row">
      <div><label>TX Sent</label><span id="statSent">—</span></div>
      <div><label>TX Failed</label><span id="statFail">—</span></div>
      <div><label>Sensor OK</label><span id="statSensor">—</span></div>
      <div><label>Tracking</label><span id="statTracking">—</span></div>
    </div>
  </div>

</div>
</div><!-- /content -->

<div id="toast"></div>

<!-- ════ Three.js 3D (CDN) ════ -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
<script>
// ══════════════════════════════════════════════════════════════
//  STATE
// ══════════════════════════════════════════════════════════════
let currentProfile = 0;
let selectedPeerMac = null;
let scanInterval   = null;
let statusInterval = null;
let activeTab = 'settings';

// ══════════════════════════════════════════════════════════════
//  TABS
// ══════════════════════════════════════════════════════════════
function switchTab(name) {
  document.querySelectorAll('.tab').forEach((t,i)=>{
    t.classList.toggle('active', ['settings','bind','debug'][i]===name);
  });
  document.querySelectorAll('.pane').forEach(p=>p.classList.remove('active'));
  document.getElementById('pane-'+name).classList.add('active');
  activeTab = name;
  if(name==='debug') startStatusPoll();
  else stopStatusPoll();
  if(name==='settings') loadConfig();
  if(name==='bind') loadBindStatus();
}

// ══════════════════════════════════════════════════════════════
//  API HELPERS
// ══════════════════════════════════════════════════════════════
async function api(method, path, body) {
  const opt = {method, headers:{'Content-Type':'application/json'}};
  if(body) opt.body = JSON.stringify(body);
  const r = await fetch(path, opt);
  return r.json();
}
function toast(msg, ok=true) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'show ' + (ok?'ok':'err');
  clearTimeout(t._tid);
  t._tid = setTimeout(()=>t.classList.remove('show'), 2500);
}

// ══════════════════════════════════════════════════════════════
//  SETTINGS
// ══════════════════════════════════════════════════════════════
async function loadConfig() {
  const d = await api('GET','/api/config');
  document.getElementById('maxPan').value  = d.maxPan;
  document.getElementById('maxTilt').value = d.maxTilt;
  document.getElementById('maxRoll').value = d.maxRoll;
  document.getElementById('invPan').checked  = d.invPan;
  document.getElementById('invTilt').checked = d.invTilt;
  document.getElementById('invRoll').checked = d.invRoll;
  document.getElementById('commsMode').value = d.commsMode;
  currentProfile = d.profile;
  for(let i=0;i<3;i++)
    document.getElementById('p'+i).classList.toggle('active', i===currentProfile);
}
async function saveConfig() {
  const body = {
    maxPan:  parseFloat(document.getElementById('maxPan').value),
    maxTilt: parseFloat(document.getElementById('maxTilt').value),
    maxRoll: parseFloat(document.getElementById('maxRoll').value),
    invPan:  document.getElementById('invPan').checked,
    invTilt: document.getElementById('invTilt').checked,
    invRoll: document.getElementById('invRoll').checked,
    commsMode: parseInt(document.getElementById('commsMode').value),
  };
  const r = await api('POST','/api/config', body);
  toast(r.ok ? 'Saved!' : 'Error: '+r.err, r.ok);
}
async function setProfile(idx) {
  const r = await api('POST','/api/profile', {index: idx});
  if(r.ok){ currentProfile=idx; loadConfig(); toast('Profile '+(idx+1)+' loaded'); }
}
async function sendCenter() {
  await api('POST','/api/center');
  toast('Center reset ✓');
}
async function factoryReset() {
  if(!confirm('Factory reset? All settings will be lost.')) return;
  await api('POST','/api/reset');
  toast('Factory reset done');
  loadConfig();
}

// ══════════════════════════════════════════════════════════════
//  BIND
// ══════════════════════════════════════════════════════════════
async function loadBindStatus() {
  const d = await api('GET','/api/status');
  const mac = d.boundMac;
  const isBcast = mac==='FF:FF:FF:FF:FF:FF';
  document.getElementById('boundMac').textContent = isBcast ? '—' : mac;
  document.getElementById('boundStatus').textContent =
    isBcast ? 'Not bound (broadcast mode)' : 'Bound ✓';
}
async function startScan() {
  document.getElementById('scanBtn').disabled = true;
  document.getElementById('scanBtn').textContent = 'Scanning...';
  document.getElementById('peerList').innerHTML =
    '<div style="color:var(--txt2);font-size:13px;padding:10px">Scanning 5s...</div>';
  selectedPeerMac = null;
  document.getElementById('bindBtn').disabled = true;

  await api('GET','/api/scan');

  // 5초 후 결과 조회
  setTimeout(async ()=>{
    const d = await api('GET','/api/scan/result');
    renderPeers(d.peers || []);
    document.getElementById('scanBtn').disabled = false;
    document.getElementById('scanBtn').textContent = 'Scan';
  }, 5500);
}
function renderPeers(peers) {
  const el = document.getElementById('peerList');
  if(!peers.length){
    el.innerHTML='<div style="color:var(--txt2);font-size:13px;padding:10px">No peers found. Is your TX powered on?</div>';
    return;
  }
  el.innerHTML = peers.map(p=>`
    <div class="peer-item" onclick="selectPeer('${p.mac}',this)"
         data-mac="${p.mac}">
      <div>
        <div class="peer-mac">${p.mac}</div>
        <div class="peer-rssi">RSSI: ${p.rssi} dBm</div>
      </div>
      <span class="badge ${p.rssi>-70?'badge-ok':'badge-warn'}">
        ${p.rssi>-70?'Strong':'Weak'}
      </span>
    </div>`).join('');
}
function selectPeer(mac, el) {
  document.querySelectorAll('.peer-item').forEach(e=>e.classList.remove('selected'));
  el.classList.add('selected');
  selectedPeerMac = mac;
  document.getElementById('bindBtn').disabled = false;
}
async function doBindSelected() {
  if(!selectedPeerMac) return;
  const r = await api('POST','/api/bind',{mac: selectedPeerMac});
  if(r.ok){
    toast('Bound to '+selectedPeerMac+' ✓');
    loadBindStatus();
    selectedPeerMac = null;
    document.getElementById('bindBtn').disabled = true;
  } else {
    toast('Bind failed: '+r.err, false);
  }
}
async function unbind() {
  if(!confirm('Unbind and switch to broadcast?')) return;
  const r = await api('POST','/api/unbind');
  if(r.ok){ toast('Unbound'); loadBindStatus(); }
}

// ══════════════════════════════════════════════════════════════
//  DEBUG — STATUS POLL
// ══════════════════════════════════════════════════════════════
function startStatusPoll() {
  if(statusInterval) return;
  statusInterval = setInterval(updateStatus, 100);
}
function stopStatusPoll() {
  clearInterval(statusInterval);
  statusInterval = null;
}
async function updateStatus() {
  let d;
  try { d = await api('GET','/api/status'); } catch(e){ return; }

  // Header dots
  document.getElementById('sensorDot').className  = 'dot'+(d.sensorOk?' ok':'');
  document.getElementById('espnowDot').className  = 'dot'+(d.espnowConnected?' ok':'');
  document.getElementById('sensorLbl').textContent  = d.sensorOk  ? 'Sensor OK' : 'No Sensor';
  document.getElementById('espnowLbl').textContent  = d.espnowConnected ? 'Connected' : 'Searching';

  // Gauges
  const setGauge = (id, barId, chId, val, chVal, max) => {
    document.getElementById(id).textContent = val.toFixed(1)+'°';
    document.getElementById(chId).textContent = chVal;
    document.getElementById(barId).style.width =
      Math.round(((val + max) / (2*max)) * 100)+'%';
  };
  setGauge('gPan',  'gPanBar',  'gChPan',  d.pan,  d.chPan,  90);
  setGauge('gTilt', 'gTiltBar', 'gChTilt', d.tilt, d.chTilt, 60);
  setGauge('gRoll', 'gRollBar', 'gChRoll', d.roll, d.chRoll, 45);

  // Stats
  document.getElementById('statSent').textContent    = d.txSent;
  document.getElementById('statFail').textContent    = d.txFailed;
  document.getElementById('statSensor').textContent  = d.sensorOk  ? 'OK' : 'ERR';
  document.getElementById('statTracking').textContent= d.tracking  ? 'ON' : 'OFF';

  // 3D update
  if(head3D) {
    head3D.rotation.y = d.pan  * Math.PI / 180;   // yaw
    head3D.rotation.x = d.tilt * Math.PI / 180;   // pitch
    head3D.rotation.z = -d.roll * Math.PI / 180;  // roll
  }
}

// ══════════════════════════════════════════════════════════════
//  THREE.JS  —  3D 헤드 모델
// ══════════════════════════════════════════════════════════════
let head3D = null;
(function init3D() {
  const canvas = document.getElementById('canvas3d');
  const W = canvas.clientWidth  || 600;
  const H = canvas.clientHeight || 320;

  const renderer = new THREE.WebGLRenderer({canvas, antialias:true, alpha:true});
  renderer.setSize(W, H);
  renderer.setPixelRatio(window.devicePixelRatio);

  const scene  = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(50, W/H, 0.1, 100);
  camera.position.set(0, 0.5, 3.5);

  // Lights
  scene.add(new THREE.AmbientLight(0xffffff, 0.6));
  const dlight = new THREE.DirectionalLight(0xffffff, 0.8);
  dlight.position.set(2,3,4);
  scene.add(dlight);

  // ── 간단한 헤드 모델 (박스 조합) ───────────────────────────
  const headGroup = new THREE.Group();

  // 머리 (둥근 박스)
  const headGeo  = new THREE.BoxGeometry(1.2, 1.4, 1.1);
  const headMat  = new THREE.MeshPhongMaterial({color:0x3d7fff, transparent:true, opacity:.85});
  const headMesh = new THREE.Mesh(headGeo, headMat);
  headGroup.add(headMesh);

  // 코
  const noseGeo  = new THREE.BoxGeometry(.18, .18, .28);
  const noseMat  = new THREE.MeshPhongMaterial({color:0x00c896});
  const noseMesh = new THREE.Mesh(noseGeo, noseMat);
  noseMesh.position.set(0, -.1, .62);
  headGroup.add(noseMesh);

  // 눈 L/R
  const eyeGeo = new THREE.BoxGeometry(.22, .16, .06);
  const eyeMat = new THREE.MeshPhongMaterial({color:0xffffff});
  const eyeL   = new THREE.Mesh(eyeGeo, eyeMat);
  const eyeR   = new THREE.Mesh(eyeGeo, eyeMat);
  eyeL.position.set(-.3, .18, .56);
  eyeR.position.set( .3, .18, .56);
  headGroup.add(eyeL, eyeR);

  // 방향 참조축 (앞=파랑, 위=초록, 오른쪽=빨강)
  const axisLen = 1.0;
  const mkAxis = (color, dir) => {
    const geo = new THREE.BufferGeometry().setFromPoints([
      new THREE.Vector3(0,0,0), new THREE.Vector3(...dir)]);
    return new THREE.Line(geo, new THREE.LineBasicMaterial({color}));
  };
  headGroup.add(mkAxis(0x4488ff, [0,0,axisLen]));    // forward (Z+)
  headGroup.add(mkAxis(0x00cc66, [0,axisLen,0]));    // up (Y+)
  headGroup.add(mkAxis(0xff4455, [axisLen,0,0]));    // right (X+)

  // 그리드 바닥
  const grid = new THREE.GridHelper(6, 12, 0x2d3150, 0x1a1d2e);
  grid.position.y = -1.2;
  scene.add(grid);

  scene.add(headGroup);
  head3D = headGroup;

  // Render loop
  function animate() {
    requestAnimationFrame(animate);
    renderer.render(scene, camera);
  }
  animate();

  // Resize handler
  new ResizeObserver(()=>{
    const w = canvas.clientWidth;
    const h = canvas.clientHeight || 320;
    camera.aspect = w/h;
    camera.updateProjectionMatrix();
    renderer.setSize(w,h);
  }).observe(canvas);
})();

// ══════════════════════════════════════════════════════════════
//  INIT
// ══════════════════════════════════════════════════════════════
loadConfig();
</script>
</body>
</html>
)rawhtml";

// ═══════════════════════════════════════════════════════════════
//  start
// ═══════════════════════════════════════════════════════════════
void WebUI::start() {
    if (_running) return;

    // ── WiFi AP 시작 ──────────────────────────────────────────
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD[0] ? WIFI_AP_PASSWORD : nullptr,
                WIFI_AP_CHANNEL);
    delay(100);

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[WebUI] AP: %s  IP: %s\n", WIFI_AP_SSID, ip.toString().c_str());

    // mDNS: headtracker.local
    // (ESP32 Arduino에 MDNS 내장)
    // MDNS.begin(WEBUI_HOSTNAME);

    setupRoutes();
    server.begin();
    _running = true;

    if (gState.lock()) {
        gState.wifiApActive = true;
        gState.unlock();
    }
    LEDController::setState(LEDState::WIFI_AP_ON);
    Serial.println("[WebUI] Server started on http://192.168.4.1");
}

// ═══════════════════════════════════════════════════════════════
//  stop
// ═══════════════════════════════════════════════════════════════
void WebUI::stop() {
    if (!_running) return;
    server.end();
    WiFi.softAPdisconnect(true);
    _running = false;

    if (gState.lock()) {
        gState.wifiApActive = false;
        gState.unlock();
    }
    Serial.println("[WebUI] Stopped");
}

// ═══════════════════════════════════════════════════════════════
//  setupRoutes  —  REST API 전부
// ═══════════════════════════════════════════════════════════════
void WebUI::setupRoutes() {

    // ── GET /  → SPA HTML ──────────────────────────────────────
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
        req->send_P(200, "text/html", INDEX_HTML);
    });

    // ── GET /api/status ────────────────────────────────────────
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req){
        JsonDocument doc;

        if (gState.lock()) {
            doc["sensorOk"]       = gState.sensorOk;
            doc["tracking"]       = gState.tracking;
            doc["espnowConnected"]= gState.espnowConnected;
            doc["pan"]            = gState.orientation.pan;
            doc["tilt"]           = gState.orientation.tilt;
            doc["roll"]           = gState.orientation.roll;
            doc["chPan"]          = gState.channels.pan();
            doc["chTilt"]         = gState.channels.tilt();
            doc["chRoll"]         = gState.channels.roll();

            // Backpack MAC
            const uint8_t* m = gState.config.espnowMac;
            char mac[18];
            snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                     m[0],m[1],m[2],m[3],m[4],m[5]);
            doc["boundMac"] = mac;
            gState.unlock();
        }

        // ESP-NOW 통계 (ESPNowComms에서 가져옴)
        extern uint32_t g_espnow_sent, g_espnow_failed;
        doc["txSent"]   = g_espnow_sent;
        doc["txFailed"] = g_espnow_failed;

        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // ── GET /api/config ────────────────────────────────────────
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req){
        JsonDocument doc;
        if (gState.lock()) {
            doc["maxPan"]   = gState.config.maxPan;
            doc["maxTilt"]  = gState.config.maxTilt;
            doc["maxRoll"]  = gState.config.maxRoll;
            doc["invPan"]   = gState.config.invPan;
            doc["invTilt"]  = gState.config.invTilt;
            doc["invRoll"]  = gState.config.invRoll;
            doc["commsMode"]= (int)gState.config.commsMode;
            gState.unlock();
        }
        doc["profile"] = Settings::currentProfile();
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // ── POST /api/config ───────────────────────────────────────
    AsyncCallbackJsonWebHandler* cfgHandler =
        new AsyncCallbackJsonWebHandler("/api/config",
            [](AsyncWebServerRequest* req, JsonVariant& json){
                JsonObject obj = json.as<JsonObject>();
                if (gState.lock()) {
                    if (obj["maxPan"].is<float>())  gState.config.maxPan   = obj["maxPan"];
                    if (obj["maxTilt"].is<float>()) gState.config.maxTilt  = obj["maxTilt"];
                    if (obj["maxRoll"].is<float>()) gState.config.maxRoll  = obj["maxRoll"];
                    if (obj["invPan"].is<bool>())   gState.config.invPan   = obj["invPan"];
                    if (obj["invTilt"].is<bool>())  gState.config.invTilt  = obj["invTilt"];
                    if (obj["invRoll"].is<bool>())  gState.config.invRoll  = obj["invRoll"];
                    if (obj["commsMode"].is<int>()) gState.config.commsMode= (CommsMode)(int)obj["commsMode"];
                    gState.unlock();
                }
                Settings::save(gState);
                req->send(200, "application/json", "{\"ok\":true}");
            });
    server.addHandler(cfgHandler);

    // ── POST /api/center ───────────────────────────────────────
    server.on("/api/center", HTTP_POST, [](AsyncWebServerRequest* req){
        Orientation::resetCenter(gState);
        LEDController::flashWhite();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── POST /api/profile ──────────────────────────────────────
    AsyncCallbackJsonWebHandler* profHandler =
        new AsyncCallbackJsonWebHandler("/api/profile",
            [](AsyncWebServerRequest* req, JsonVariant& json){
                int idx = json["index"] | 0;
                Settings::switchProfile(gState, (uint8_t)idx);
                req->send(200, "application/json", "{\"ok\":true}");
            });
    server.addHandler(profHandler);

    // ── POST /api/reset ────────────────────────────────────────
    server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest* req){
        Settings::factoryReset(gState);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── GET /api/scan  (ESP-NOW 피어 스캔 시작) ────────────────
    server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest* req){
        // WiFi promiscuous scan으로 ESP-NOW beacon 탐지
        // ESPNowComms::startScan() 호출 (5초간 수집)
        extern void espnow_start_scan();
        espnow_start_scan();
        req->send(200, "application/json", "{\"ok\":true,\"msg\":\"Scanning 5s\"}");
    });

    // ── GET /api/scan/result ───────────────────────────────────
    server.on("/api/scan/result", HTTP_GET, [](AsyncWebServerRequest* req){
        extern void espnow_get_scan_results(JsonDocument&);
        JsonDocument doc;
        espnow_get_scan_results(doc);
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // ── POST /api/bind ─────────────────────────────────────────
    AsyncCallbackJsonWebHandler* bindHandler =
        new AsyncCallbackJsonWebHandler("/api/bind",
            [](AsyncWebServerRequest* req, JsonVariant& json){
                String macStr = json["mac"] | "";
                if (macStr.length() != 17) {
                    req->send(400, "application/json", "{\"ok\":false,\"err\":\"Invalid MAC\"}");
                    return;
                }
                uint8_t mac[6];
                if (sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                           &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]) != 6) {
                    req->send(400, "application/json", "{\"ok\":false,\"err\":\"Parse error\"}");
                    return;
                }

                // SharedState + NVS에 저장
                if (gState.lock()) {
                    memcpy(gState.config.espnowMac, mac, 6);
                    gState.unlock();
                }
                Settings::saveBackpackMac(mac);

                // ESPNowComms 피어 업데이트
                extern void espnow_set_peer(const uint8_t*);
                espnow_set_peer(mac);

                // LED 바인드 완료 플래시
                LEDController::flashWhite();

                req->send(200, "application/json", "{\"ok\":true}");
            });
    server.addHandler(bindHandler);

    // ── POST /api/unbind ───────────────────────────────────────
    server.on("/api/unbind", HTTP_POST, [](AsyncWebServerRequest* req){
        uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        if (gState.lock()) {
            memcpy(gState.config.espnowMac, bcast, 6);
            gState.unlock();
        }
        Settings::saveBackpackMac(bcast);
        extern void espnow_set_peer(const uint8_t*);
        espnow_set_peer(bcast);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── POST /api/ota ──────────────────────────────────────────
    // OTA: multipart upload
    server.on("/api/ota", HTTP_POST,
        [](AsyncWebServerRequest* req){
            bool ok = !Update.hasError();
            req->send(200, "application/json",
                      ok ? "{\"ok\":true}" : "{\"ok\":false,\"err\":\"OTA failed\"}");
            if (ok) {
                delay(200);
                ESP.restart();
            }
        },
        [](AsyncWebServerRequest* req, String filename,
           size_t index, uint8_t* data, size_t len, bool final){
            if (!index) {
                Serial.printf("[OTA] Start: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("[OTA] Done: %u bytes\n", index+len);
                } else {
                    Update.printError(Serial);
                }
            }
        });

    // 404
    server.onNotFound([](AsyncWebServerRequest* req){
        req->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}
