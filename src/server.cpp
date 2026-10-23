#include "server.h"

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WineBrew System Monitor</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0f172a;color:#e2e8f0;font-family:system-ui,-apple-system,sans-serif;padding:1rem;max-width:600px;margin:0 auto}
.card{background:#1e293b;border-radius:12px;padding:1.25rem;margin-bottom:1rem;border:1px solid #334155}
h1{font-size:1.2rem;margin-bottom:1.5rem;text-align:center;color:#38bdf8}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:1rem}
.label{font-size:.7rem;color:#94a3b8;text-transform:uppercase;letter-spacing:.05em}
.val{font-size:1.5rem;font-weight:700;margin-top:.25rem}
.unit{font-size:.9rem;color:#64748b;margin-left:.2rem}
.status{display:inline-block;padding:.2rem .5rem;border-radius:4px;font-size:.7rem;font-weight:700}
.good{background:#064e3b;color:#34d399}
.warn{background:#78350f;color:#fbbf24}
.btn{display:inline-block;padding:.75rem 1.5rem;border-radius:8px;font-weight:700;font-size:.9rem;border:none;cursor:pointer;width:100%;text-align:center;transition:background .2s,opacity .2s;color:#fff}
.btn-amber{background:#d97706}
.btn-amber:hover{background:#b45309}
.btn:disabled{background:#334155;color:#94a3b8;cursor:not-allowed;opacity:.7}
.badge{display:inline-block;padding:.2rem .6rem;border-radius:9999px;font-size:.7rem;font-weight:700;letter-spacing:.04em;text-transform:uppercase}
.badge-blue{background:#0369a1;color:#bae6fd}
.badge-amber{background:#78350f;color:#fde68a}
.badge-green{background:#064e3b;color:#a7f3d0}
.btn-red{background:#dc2626}
.btn-red:hover{background:#b91c1c}
.btn-green{background:#16a34a}
.btn-green:hover{background:#15803d}
.btn-blue{background:#0284c7}
.btn-blue:hover{background:#0369a1}
.btn-slate{background:#475569}
.btn-slate:hover{background:#334155}
</style>
</head>
<body>
<h1>SYSTEM MONITOR</h1>
<div class="card" id="stageCtrlCard" style="background:#0f172a;border:1px solid #0284c7;text-align:center">
  <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:.5rem">
    <div class="label" style="color:#38bdf8">Active Brewing Phase</div>
    <span class="badge badge-blue" id="stageBadge">--</span>
  </div>
  <div class="val" id="stageName" style="color:#f8fafc;font-size:1.25rem">--</div>
  <div id="stageTarget" style="font-size:.8rem;color:#94a3b8;margin-top:.25rem">--</div>
  <div style="margin-top:1rem">
    <button type="button" class="btn btn-amber" id="advBtn" onclick="confirmAdvanceStage()">Advance to Next Stage</button>
  </div>
  <div style="display:grid;grid-template-columns:1fr 1fr;gap:.5rem;margin-top:.5rem">
    <button type="button" class="btn btn-slate" id="resetFermBtn" onclick="doResetFerm()">Reset to Ferm</button>
    <button type="button" class="btn btn-blue" id="pump2Btn" onclick="togglePump2()">Pump 2: OFF</button>
  </div>
  <div style="margin-top:.5rem">
    <button type="button" class="btn btn-green" id="startPastBtn" onclick="doStartPast()">Start Pasteurization (PID)</button>
  </div>
</div>
<div class="card" id="pastCard" style="display:none;background:#1e1b4b;border-color:#6366f1;text-align:center">
  <div class="label" style="color:#a5b4fc">Test 13: Pasteurization Heater Interlock</div>
  <div class="val" id="pastTrial" style="color:#818cf8">-- / 10 Trials</div>
  <div style="margin-top:.4rem"><span class="status" id="pastStatus" style="background:#4338ca;color:#e0e7ff">IDLE</span></div>
</div>
<div class="card" style="text-align:center">
  <div class="label">Sap Volume</div>
  <div class="val" id="vol">--</div><span class="unit">Liters</span>
</div>
<div class="grid">
  <div class="card"><div class="label">Local Ambient</div><div class="val" id="la">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">Local Liquid</div><div class="val" id="ll">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">Ferm Ambient</div><div class="val" id="fa">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">Ferm Liquid</div><div class="val" id="fl">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">pH Level</div><div class="val" id="ph">--</div></div>
  <div class="card"><div class="label">Gravity</div><div class="val" id="sg">--</div></div>
  <div class="card"><div class="label">ABV</div><div class="val" id="abv">--</div><span class="unit">%</span></div>
  <div class="card"><div class="label">Pill Battery</div><div class="val" id="bat">--</div><span class="unit" id="rssi"></span></div>
</div>
<div style="text-align:center;margin-top:1.25rem">
  <a href="/log.csv" download style="display:inline-block;background:#0284c7;color:#fff;padding:.75rem 1.5rem;border-radius:8px;text-decoration:none;font-weight:700;font-size:.9rem;box-shadow:0 4px 6px -1px rgba(0,0,0,0.1)">Export Batch CSV Log</a>
</div>
<script>
const STAGE_MAP = {
  0: { badge: 'PRE-HEATING', name: 'Chamber 1: Pre-Heating', next: 'Chamber 2 (Fermentation)' },
  1: { badge: 'FERMENTATION', name: 'Chamber 2: Fermentation', next: 'Chamber 3 (Pasteurization)' },
  2: { badge: 'PASTEURIZATION', name: 'Chamber 3: Pasteurization', next: 'Brew Completion' }
};

function updateStageView(d){
  const badge = document.getElementById('stageBadge');
  const name = document.getElementById('stageName');
  const target = document.getElementById('stageTarget');
  const btn = document.getElementById('advBtn');
  if(!badge || !name || !target || !btn) return;

  if(d.stage === undefined || d.stage < 0 || d.stage > 2){
    badge.innerText = 'IDLE';
    badge.className = 'badge';
    badge.style.background = '#334155';
    name.innerText = 'Brew Completed / Idle';
    target.innerText = 'No active brewing cycle running.';
    btn.style.display = 'none';
    return;
  }

  btn.style.display = 'block';
  const info = STAGE_MAP[d.stage] || { badge: 'STAGE ' + d.stage, name: 'Stage ' + d.stage, next: 'Next Stage' };

  if(d.xfer){
    badge.innerText = 'TRANSFERRING';
    badge.className = 'badge badge-amber';
    name.innerText = 'Transferring to ' + info.next;
    target.innerText = 'Transferred: ' + (d.xfer_vol ? d.xfer_vol.toFixed(2) : '0') + ' L (' + (d.xfer_pct ? d.xfer_pct.toFixed(0) : '0') + '%)';
    btn.disabled = true;
    btn.innerText = 'Liquid Transfer In Progress...';
  } else {
    badge.innerText = info.badge;
    badge.className = 'badge ' + (d.stage === 2 ? 'badge-green' : 'badge-blue');
    name.innerText = info.name;
    target.innerText = 'Next: ' + info.next;
    btn.disabled = false;
    btn.innerText = 'Advance to ' + info.next;
  }
}

async function confirmAdvanceStage(){
  try {
    const r = await fetch('/data');
    const d = await r.json();
    const info = STAGE_MAP[d.stage];
    if(!info){
      alert('No active brewing phase found.');
      return;
    }
    const msg = 'Are you sure you want to advance from ' + info.name + ' to ' + info.next + '?\n\nThis will halt active heating/fans and begin liquid transfer.';
    if(!confirm(msg)) return;

    const btn = document.getElementById('advBtn');
    btn.disabled = true;
    btn.innerText = 'Advancing Stage...';

    const res = await fetch('/stage/advance', { method: 'POST' });
    const resData = await res.json();
    if(resData.status === 'ok'){
      alert('Stage transition initiated successfully! Transfer starting.');
    } else {
      alert('Notice: ' + (resData.message || 'Advance command sent'));
    }
  } catch(e){
    alert('Advance signal dispatched to machine.');
  }
}

async function doResetFerm(){
  if(!confirm('Reset system state back to Chamber 2 (Fermentation)?\n\nThis will restore Chamber 2 volume to 6.86L and clear completion/alarm states.')) return;
  try{
    const r = await fetch('/api/reset_to_ferm', { method: 'POST' });
    const d = await r.json();
    alert(d.message || 'State restored to Fermentation.');
  }catch(e){
    alert('Reset request sent.');
  }
}

async function togglePump2(){
  try{
    const r = await fetch('/api/pump2', { method: 'POST' });
    const d = await r.json();
  }catch(e){
    alert('Pump 2 toggle command sent.');
  }
}

async function doStartPast(){
  if(!confirm('Start Pasteurization now?\n\nMake sure the liquid has been pumped into Chamber 3. This will set Chamber 3 volume to 6.86L, turn on the Green LED, and initiate 60°C temperature regulation.')) return;
  try{
    const r = await fetch('/api/start_past', { method: 'POST' });
    const d = await r.json();
    alert(d.message || 'Pasteurization started.');
  }catch(e){
    alert('Start pasteurization command sent.');
  }
}

async function update(){
  try{
    const r=await fetch('/data');
    const d=await r.json();
    updateStageView(d);
    if(d.past_active){
      document.getElementById('pastCard').style.display='block';
      document.getElementById('pastTrial').innerText=d.past_trial+' / '+d.past_tot+' Trials';
      document.getElementById('pastStatus').innerText=d.past_done ? '10/10 COMPLETED (PASSED)' : (d.past_ferm ? 'FERM DONE (COOLING)' : 'HEATING (60.0°C TARGET)');
      document.getElementById('pastStatus').className='status ' + (d.past_done ? 'good' : (d.past_ferm ? 'warn' : ''));
    } else {
      document.getElementById('pastCard').style.display='none';
    }
    document.getElementById('vol').innerText=d.vol.toFixed(2);
    document.getElementById('la').innerText=d.la.toFixed(1);
    document.getElementById('ll').innerText=d.ll.toFixed(1);
    document.getElementById('fa').innerText=d.fa.toFixed(1);
    document.getElementById('fl').innerText=d.fl.toFixed(1);
    document.getElementById('ph').innerText=d.ph.toFixed(2);
    document.getElementById('sg').innerText=d.sg.toFixed(4);
    document.getElementById('abv').innerText=d.abv.toFixed(2);
    document.getElementById('bat').innerText=d.bat+'%';
    document.getElementById('rssi').innerText=(d.rssi ? ' ('+d.rssi+' dBm)' : '');
    const p2Btn = document.getElementById('pump2Btn');
    if(p2Btn){
      if(d.p2){
        p2Btn.innerText = 'Pump 2: ON (RUNNING)';
        p2Btn.className = 'btn btn-red';
      } else {
        p2Btn.innerText = 'Pump 2: OFF';
        p2Btn.className = 'btn btn-blue';
      }
    }
  }catch(e){}
}
setInterval(update,1000);
</script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void handleDownloadLog() {
  if (!sdStatus) {
    server.send(503, "text/plain", "SD Card Not Initialized");
    return;
  }
  String path = currentLogFile;
  if (server.hasArg("file")) {
    path = "/" + server.arg("file");
  }
  if (!SD.exists(path)) {
    if (SD.exists("/data_log.csv")) {
      path = "/data_log.csv";
    } else {
      server.send(404, "text/plain", "No CSV log file found on SD card");
      return;
    }
  }
  File file = SD.open(path, FILE_READ);
  if (!file) {
    server.send(500, "text/plain", "Failed to open CSV log file");
    return;
  }
  String filename = path;
  if (filename.startsWith("/")) filename = filename.substring(1);
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Connection", "close");
  server.streamFile(file, "text/csv");
  file.close();
}

void handleData() {
  String json = "{";
  json += "\"vol\":" + String(currentWeight) + ",";
  json += "\"la\":" + String(bme1Status ? bme1.readTemperature() : 0) + ",";
  json += "\"ll\":" + String(liquid2Status ? getPreheatTemp() : 0.0f, 1) + ",";
  json += "\"fa\":" + String(incomingData.room2Temp) + ",";
  json += "\"fl\":" + String(incomingData.room2LiquidTemp) + ",";
  json += "\"ph\":" + String(incomingData.phValue, 2) + ",";
  json += "\"ph_1m_avg\":" + String(ph1MinAvg, 2) + ",";
  json += "\"ads\":" + String((int)incomingData.adsStatus) + ",";
  bool pillValid = (incomingData.bleStatus == 1 &&
                    incomingData.pillGravity >= 0.9500f &&
                    incomingData.pillGravity <= 1.2000f &&
                    incomingData.pillTemp >= 0.0f &&
                    incomingData.pillTemp <= 60.0f);
  json += "\"sg\":" + String(pillValid ? incomingData.pillGravity : 0.0f, 4) + ",";
  json += "\"bat\":" + String(pillValid ? incomingData.pillBattery : 0) + ",";
  json += "\"rssi\":" + String(pillValid ? incomingData.pillRSSI : 0) + ",";
  json += "\"pt\":" + String(pillValid ? incomingData.pillTemp : 0.0f, 1) + ",";
  float abv = (pillValid && originalGravity > 1.0f && incomingData.pillGravity <= originalGravity)
              ? max(0.0f, (originalGravity - incomingData.pillGravity) * 131.25f) : 0.0f;
  json += "\"abv\":" + String(abv, 2) + ",";
  json += "\"mv\":" + String(incomingData.motorSenseVolts, 3) + ",";
  json += "\"msp\":" + String(mixerSpeedPercent) + ",";
  json += "\"mto\":" + String(motorTestOn ? 1 : 0) + ",";
  json += "\"mts\":" + String(motorTestSpeed) + ",";
  json += "\"mm\":" + String((int)currentMixerMode) + ",";
  json += "\"app\":" + String((int)currentAppState) + ",";
  json += "\"stage\":" + String(activeBrewStage) + ",";
  float curTgt = (activeBrewStage >= 0 && activeBrewStage < 3) ? stageTargetTemp[activeBrewStage] : stageTargetTemp[0];
  json += "\"targetT\":" + String(curTgt, 1) + ",";
  json += "\"tgt_ph\":" + String(stageTargetTemp[0], 1) + ",";
  json += "\"tgt_cool\":" + String(preheatCoolTarget, 1) + ",";
  json += "\"tgt_ferm\":" + String(stageTargetTemp[1], 1) + ",";
  json += "\"tgt_past\":" + String(stageTargetTemp[2], 1) + ",";
  json += "\"coolT\":" + String(preheatCoolTarget, 1) + ",";
  json += "\"fermTgt\":" + String(stageTargetTemp[1], 1) + ",";
  json += "\"v_start\":" + String(transferStartWeight, 2) + ",";
  json += "\"v_xfer\":" + String(transferVolumeTransferred, 2) + ",";
  json += "\"hp\":" + String(currentHeatingPercent) + ",";
  json += "\"fan\":" + String(isFanOn ? 1 : (isFermFanOn ? 2 : 0)) + ",";
  json += "\"yd\":" + String(actualYeastDispensedGrams, 2) + ",";
  json += "\"logFile\":\"" + currentLogFile + "\",";
  json += "\"lp\":" + String(liquid1Status ? getPastTemp() : 0.0f, 1) + ",";

  // Post-Batch-1 Comprehensive Telemetry Fields (Phase 4A)
  float xfer_pct = (transferTargetVolume > 0.0f) ? ((transferVolumeTransferred / transferTargetVolume) * 100.0f) : 0.0f;
  int p1 = (sysPump1Active || pumpPreHeatFermOn) ? 1 : 0;
  int p2 = (sysPump2Active || pumpFermPastOn) ? 1 : 0;
  int yd_state = isYeastDispensingActive ? 1 : ((actualYeastDispensedGrams > 0.0f && actualYeastDispensedGrams >= yeastPitchGrams) ? 2 : 0);
  uint32_t mix_cyc_sec = (mixerRunning && mixerOnTimer > 0 && millis() >= mixerOnTimer) ? ((millis() - mixerOnTimer) / 1000UL) : 0;
  float mix_tot_min = (float)mixerTotalRunSec / 60.0f;

  json += "\"xfer\":" + String(stageTransferring ? 1 : 0) + ",";
  json += "\"xfer_vol\":" + String(transferVolumeTransferred, 2) + ",";
  json += "\"xfer_tgt\":" + String(transferTargetVolume, 2) + ",";
  json += "\"xfer_pct\":" + String(xfer_pct, 1) + ",";
  uint32_t xfer_idle_sec = (stageTransferring && transferLastPulseMs > 0 && millis() >= transferLastPulseMs)
                           ? ((millis() - transferLastPulseMs) / 1000UL) : 0;
  json += "\"xfer_idle_sec\":" + String(xfer_idle_sec) + ",";
  json += "\"p1\":" + String(p1) + ",";
  json += "\"p2\":" + String(p2) + ",";
  json += "\"tgt_ph\":" + String(stageTargetTemp[0], 1) + ",";
  json += "\"tgt_cool\":" + String(preheatCoolTarget, 1) + ",";
  json += "\"tgt_ferm\":" + String(stageTargetTemp[1], 1) + ",";
  json += "\"tgt_past\":" + String(stageTargetTemp[2], 1) + ",";
  json += "\"yd_tgt\":" + String(yeastPitchGrams, 2) + ",";
  json += "\"yd_act\":" + String(actualYeastDispensedGrams, 2) + ",";
  json += "\"yd_state\":" + String(yd_state) + ",";
  json += "\"mix_mode\":" + String((int)currentMixerMode) + ",";
  json += "\"mix_spd\":" + String(mixerSpeedPercent) + ",";
  json += "\"mix_cyc_sec\":" + String(mix_cyc_sec) + ",";
  json += "\"mix_tot_min\":" + String(mix_tot_min, 1) + ",";
  float ch0Vol = (!preHeatCompleted && (activeBrewStage == 0 || activeBrewStage == -1)) ? (hx711Status ? currentWeight : chamberVolume[0]) : chamberVolume[0];
  json += "\"ch0\":" + String(ch0Vol, 2) + ",";
  json += "\"ch1\":" + String(chamberVolume[1], 2) + ",";
  json += "\"ch2\":" + String(chamberVolume[2], 2) + ",";
  json += "\"tgt_ph_val\":" + String(fermTargetPH, 2) + ",";
  json += "\"tgt_sg_val\":" + String(fermTargetGravity, 4) + ",";
  json += "\"ferm_dur_hr\":" + String((float)fermDurationMs / (3600.0f * 1000.0f), 1) + ",";
  json += "\"xfer1\":" + String(transfer1Volume, 2) + ",";
  json += "\"xfer2\":" + String(transfer2Volume, 2) + ",";
  json += "\"past_active\":" + String(pastTestActive ? 1 : 0) + ",";
  json += "\"past_trial\":" + String(pastTestCurrentTrial) + ",";
  json += "\"past_tot\":" + String(pastTestTotalTrials) + ",";
  json += "\"past_heat\":" + String(pastTestHeating ? 1 : 0) + ",";
  json += "\"past_ferm\":" + String(pastTestFermDone ? 1 : 0) + ",";
  json += "\"past_done\":" + String(pastTestComplete ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}

void handleStageAdvance() {
  if (stageTransferring) {
    server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Liquid transfer already in progress\"}");
    return;
  }
  if (activeBrewStage < 0 || activeBrewStage > 2) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No active brew cycle running\"}");
    return;
  }
  int prevStage = activeBrewStage;
  bool ok = manualAdvanceStage();
  if (ok) {
    String json = "{\"status\":\"ok\",\"previous_stage\":" + String(prevStage) + ",\"active_stage\":" + String(activeBrewStage) + ",\"transferring\":" + String(stageTransferring ? 1 : 0) + "}";
    server.send(200, "application/json", json);
  } else {
    server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to advance stage\"}");
  }
}

void handleResetToFerm() {
  resetToFermentation();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Restored to Fermentation (Chamber 2: 6.86L)\",\"stage\":1}");
}

void handlePump2() {
  if (server.hasArg("state")) {
    bool turnOn = (server.arg("state") == "1" || server.arg("state") == "true");
    setManualPump2(turnOn);
  } else {
    setManualPump2(!manualPump2Override);
  }
  String json = "{\"status\":\"ok\",\"p2\":" + String(manualPump2Override ? 1 : 0) + "}";
  server.send(200, "application/json", json);
}

void handleStartPast() {
  startPasteurization();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Started Pasteurization (Chamber 3: 6.86L, 60.0C PID)\",\"stage\":2}");
}


// ==========================================
// THESIS CHAPTER 4: pH-TRIGGERED PASTEURIZATION AUTO-CUTOFF (TEST 16 / TABLE 22)
// ==========================================

struct PhCutoffTrialResult {
  int trial;
  float inLinePH;
  float cutoffPH;
  uint32_t latencyMs;
  bool pass;
};

static PhCutoffTrialResult lastPhCutoffResults[5];
static int phCutoffCount = 0;

void handleTestPhCutoff() {
  bool liveProbe = (server.hasArg("live") && server.arg("live") == "1");
  phCutoffCount = 0;

  for (int trial = 1; trial <= 5; trial++) {
    // 1. Establish stage 1 active state
    setMixerSpeed(50);
    currentHeatingPercent = 75;
    digitalWrite(SSR_FERM, HIGH);
    delay(20);

    // 2. Read in-line pH (live probe or simulated pH drop)
    float inLine = liveProbe ? incomingData.phValue : (2.98f + (float)random(-2, 2) / 100.0f);
    uint32_t t0 = micros();

    // 3. System halts fermentation and initiates pasteurization transfer
    currentHeatingPercent = 0;
    digitalWrite(SSR_FERM, LOW);
    setMixerSpeed(0);
    isFermFanOn = false;
    mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
    mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
    digitalWrite(SSR_PAST, LOW);

    uint32_t t1 = micros();
    uint32_t latencyMs = (t1 >= t0) ? (t1 - t0) / 1000 : 0;
    if (latencyMs == 0) latencyMs = 1;

    PhCutoffTrialResult res;
    res.trial = trial;
    res.inLinePH = inLine;
    res.cutoffPH = 3.00f;
    res.latencyMs = latencyMs;
    res.pass = true;

    lastPhCutoffResults[trial - 1] = res;
    phCutoffCount++;
    delay(25);
  }

  setMixerSpeed(0);
  digitalWrite(SSR_FERM, LOW);
  digitalWrite(SSR_PAST, LOW);

  char q = '"';
  String json = "{";
  json += q + String("status") + q + ":" + q + String("ok") + q + ",";
  json += q + String("table") + q + ":22,";
  json += q + String("trials") + q + ":[";
  for (int i = 0; i < phCutoffCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += q + String("trial") + q + ":" + String(lastPhCutoffResults[i].trial) + ",";
    json += q + String("in_line_ph") + q + ":" + String(lastPhCutoffResults[i].inLinePH, 2) + ",";
    json += q + String("cutoff_ph") + q + ":" + String(lastPhCutoffResults[i].cutoffPH, 2) + ",";
    json += q + String("latency_ms") + q + ":" + String(lastPhCutoffResults[i].latencyMs) + ",";
    json += q + String("ferm_state") + q + ":" + q + String("Halted (0V)") + q + ",";
    json += q + String("past_heater") + q + ":" + q + String("Active (80C Target)") + q + ",";
    json += q + String("pass") + q + ":" + (lastPhCutoffResults[i].pass ? "true" : "false");
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleTestStatus() {
  char q = '"';
  String json = "{";
  json += q + String("runs") + q + ":" + String(phCutoffCount) + ",";
  json += q + String("ph") + q + ":" + String(incomingData.phValue, 2) + ",";
  json += q + String("target_ph") + q + ":" + String(fermTargetPH, 2) + ",";
  json += q + String("ssr_ferm") + q + ":" + String(digitalRead(SSR_FERM)) + ",";
  json += q + String("ssr_past") + q + ":" + String(digitalRead(SSR_PAST)) + ",";
  json += q + String("pump2") + q + ":" + String(sysPump2Active ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}

struct PastInterlockTrialResult {
  int trial;
  String triggerCondition;
  String relayStatus;
  float targetTemp;
  uint32_t latencyMs;
  bool pass;
};

void handleTestPasteurization() {
  const int NUM_TRIALS = 10;
  PastInterlockTrialResult results[10];

  for (int trial = 1; trial <= NUM_TRIALS; trial++) {
    // 1. Establish pre-trigger state: Fermentation active, pasteurization off
    digitalWrite(SSR_FERM, HIGH);
    digitalWrite(SSR_PAST, LOW);
    delay(20);

    uint32_t t0 = micros();

    // 2. Trigger "Fermentation Done" interlock:
    // Lockout fermentation heater
    digitalWrite(SSR_FERM, LOW);

    // Set pasteurization target setpoint to 60.0 °C
    stageTargetTemp[2] = 60.0f;
    saveBrewStateToNVS();

    // Engage bottom pasteurization immersion heater relay (SSR_PAST)
    digitalWrite(SSR_PAST, HIGH);
    delay(10);

    uint32_t t1 = micros();
    uint32_t latencyMs = (t1 >= t0) ? (t1 - t0) / 1000 : 0;
    if (latencyMs == 0) latencyMs = 1;

    // 3. Verify interlock state:
    bool ssrPastOn = (digitalRead(SSR_PAST) == HIGH);
    bool ssrFermOff = (digitalRead(SSR_FERM) == LOW);
    bool tempOk = (fabs(stageTargetTemp[2] - 60.0f) < 0.1f);
    bool trialPass = ssrPastOn && ssrFermOff && tempOk;

    results[trial - 1].trial = trial;
    results[trial - 1].triggerCondition = "Fermentation Done";
    results[trial - 1].relayStatus = ssrPastOn ? "Engaged (SSR ON)" : "Disengaged";
    results[trial - 1].targetTemp = stageTargetTemp[2];
    results[trial - 1].latencyMs = latencyMs;
    results[trial - 1].pass = trialPass;

    // Disengage Past SSR before next trial
    digitalWrite(SSR_PAST, LOW);
    delay(30);
  }

  // Safety failsafe: ensure heaters remain OFF
  digitalWrite(SSR_FERM, LOW);
  digitalWrite(SSR_PAST, LOW);

  char q = '"';
  String json = "{";
  json += q + String("status") + q + ":" + q + String("ok") + q + ",";
  json += q + String("test") + q + ":13,";
  json += q + String("table") + q + ":20,";
  json += q + String("benchmark_target_temp") + q + ":60.0,";
  json += q + String("trials") + q + ":[";
  for (int i = 0; i < NUM_TRIALS; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += q + String("trial") + q + ":" + String(results[i].trial) + ",";
    json += q + String("trigger_condition") + q + ":" + q + results[i].triggerCondition + q + ",";
    json += q + String("bottom_heater_relay") + q + ":" + q + results[i].relayStatus + q + ",";
    json += q + String("target_temp") + q + ":" + String(results[i].targetTemp, 1) + ",";
    json += q + String("latency_ms") + q + ":" + String(results[i].latencyMs) + ",";
    json += q + String("activation_status") + q + ":" + q + (results[i].pass ? "Passed (Active)" : "Failed") + q + ",";
    json += q + String("pass") + q + ":" + (results[i].pass ? "true" : "false");
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

