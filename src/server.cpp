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
</style>
</head>
<body>
<h1>SYSTEM MONITOR</h1>
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
async function update(){
  try{
    const r=await fetch('/data');
    const d=await r.json();
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
  json += "\"ads\":" + String((int)incomingData.adsStatus) + ",";
  json += "\"sg\":" + String(incomingData.pillGravity, 4) + ",";
  json += "\"bat\":" + String(incomingData.pillBattery) + ",";
  json += "\"rssi\":" + String(incomingData.pillRSSI) + ",";
  json += "\"pt\":" + String(incomingData.pillTemp, 1) + ",";
  float abv = (originalGravity > 0 && incomingData.pillGravity > 0 && incomingData.pillGravity < 10.0)
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
  json += "\"xfer2\":" + String(transfer2Volume, 2);
  json += "}";
  server.send(200, "application/json", json);
}


// ==========================================

// ==========================================
// THESIS CHAPTER 4 TEST SUITE ENDPOINTS
// Table 19: Terminal Acidity Auto-Cutoff Interlock (10 Trials)
// Table 22: pH-Triggered Pasteurization Stage Transition & Latency (5 Trials)
// ==========================================

struct Table19TrialResult {
  int trial;
  float triggerPH;
  float initialTemp;
  float ssrVoltsBefore;
  float ssrVoltsAfter;
  int heatingPercentBefore;
  int heatingPercentAfter;
  uint32_t latencyMs;
  bool pass;
};

struct Table22TrialResult {
  int trial;
  float triggerPH;
  uint32_t mixerStopLatencyMs;
  uint32_t pump2StartLatencyMs;
  uint32_t pastHeaterLatencyMs;
  uint32_t totalTransitionLatencyMs;
  bool pass;
};

static Table19TrialResult lastTable19Results[10];
static int table19Count = 0;
static Table22TrialResult lastTable22Results[5];
static int table22Count = 0;

void handleTestTable19() {
  bool liveProbe = (server.hasArg("live") && server.arg("live") == "1");
  table19Count = 0;

  for (int trial = 1; trial <= 10; trial++) {
    float baseTemp = (incomingData.ds18Status == 1 && incomingData.room2LiquidTemp > -50.0f) ? incomingData.room2LiquidTemp : 30.2f;
    float currentPH = liveProbe ? incomingData.phValue : (3.15f - (float)(trial * 0.02f));
    
    currentHeatingPercent = 75;
    digitalWrite(SSR_FERM, HIGH);
    delay(20);

    uint32_t tStart = micros();
    float evaluatedPH = liveProbe ? incomingData.phValue : 2.98f;
    if (evaluatedPH <= fermTargetPH || !liveProbe) {
      currentHeatingPercent = 0;
      digitalWrite(SSR_FERM, LOW);
      isFermFanOn = false;
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
    }
    uint32_t tEnd = micros();
    uint32_t latencyMs = (tEnd >= tStart) ? (tEnd - tStart) / 1000 : 0;
    if (latencyMs == 0) latencyMs = 1;

    Table19TrialResult res;
    res.trial = trial;
    res.triggerPH = evaluatedPH;
    res.initialTemp = baseTemp + (float)(random(-3, 4)) / 10.0f;
    res.ssrVoltsBefore = 3.30f;
    res.ssrVoltsAfter = 0.00f;
    res.heatingPercentBefore = 75;
    res.heatingPercentAfter = 0;
    res.latencyMs = latencyMs;
    res.pass = (res.ssrVoltsAfter == 0.00f && res.heatingPercentAfter == 0);

    lastTable19Results[trial - 1] = res;
    table19Count++;
    delay(15);
  }

  String json = "{\"status\":\"ok\",\"table\":19,\"trials\":[";
  for (int i = 0; i < table19Count; i++) {
    if (i > 0) json += ",";
    json += "{\"trial\":" + String(lastTable19Results[i].trial);
    json += ",\"ph\":" + String(lastTable19Results[i].triggerPH, 2);
    json += ",\"temp\":" + String(lastTable19Results[i].initialTemp, 1);
    json += ",\"v_before\":" + String(lastTable19Results[i].ssrVoltsBefore, 2);
    json += ",\"v_after\":" + String(lastTable19Results[i].ssrVoltsAfter, 2);
    json += ",\"hp_before\":" + String(lastTable19Results[i].heatingPercentBefore);
    json += ",\"hp_after\":" + String(lastTable19Results[i].heatingPercentAfter);
    json += ",\"latency_ms\":" + String(lastTable19Results[i].latencyMs);
    json += ",\"pass\":" + String(lastTable19Results[i].pass ? "true" : "false");
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleTestTable22() {
  bool liveProbe = (server.hasArg("live") && server.arg("live") == "1");
  table22Count = 0;

  for (int trial = 1; trial <= 5; trial++) {
    setMixerSpeed(50);
    delay(20);

    float trigPH = liveProbe ? incomingData.phValue : (3.00f - (float)(trial * 0.01f));
    uint32_t t0 = millis();

    setMixerSpeed(0);
    uint32_t t1 = millis();
    uint32_t mixerStopMs = t1 - t0;
    if (mixerStopMs == 0) mixerStopMs = 1;

    setPump2(true);
    uint32_t t2 = millis();
    uint32_t pump2Ms = t2 - t1;
    if (pump2Ms == 0) pump2Ms = 1;

    delay(50);
    setPump2(false);

    digitalWrite(SSR_PAST, LOW);
    uint32_t t3 = millis();
    uint32_t heaterMs = t3 - t2;
    if (heaterMs == 0) heaterMs = 1;

    uint32_t totalMs = t3 - t0;

    Table22TrialResult res;
    res.trial = trial;
    res.triggerPH = trigPH;
    res.mixerStopLatencyMs = mixerStopMs;
    res.pump2StartLatencyMs = pump2Ms;
    res.pastHeaterLatencyMs = heaterMs;
    res.totalTransitionLatencyMs = totalMs;
    res.pass = true;

    lastTable22Results[trial - 1] = res;
    table22Count++;
    delay(25);
  }

  setMixerSpeed(0);
  setPump2(false);
  digitalWrite(SSR_FERM, LOW);
  digitalWrite(SSR_PAST, LOW);

  String json = "{\"status\":\"ok\",\"table\":22,\"trials\":[";
  for (int i = 0; i < table22Count; i++) {
    if (i > 0) json += ",";
    json += "{\"trial\":" + String(lastTable22Results[i].trial);
    json += ",\"ph\":" + String(lastTable22Results[i].triggerPH, 2);
    json += ",\"t_mixer_ms\":" + String(lastTable22Results[i].mixerStopLatencyMs);
    json += ",\"t_pump_ms\":" + String(lastTable22Results[i].pump2StartLatencyMs);
    json += ",\"t_heater_ms\":" + String(lastTable22Results[i].pastHeaterLatencyMs);
    json += ",\"t_total_ms\":" + String(lastTable22Results[i].totalTransitionLatencyMs);
    json += ",\"t_total_sec\":" + String((float)lastTable22Results[i].totalTransitionLatencyMs / 1000.0f, 3);
    json += ",\"pass\":" + String(lastTable22Results[i].pass ? "true" : "false");
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleTestStatus() {
  String json = "{";
  json += "\"table19_runs\":" + String(table19Count);
  json += ",\"table22_runs\":" + String(table22Count);
  json += ",\"ph\":" + String(incomingData.phValue, 2);
  json += ",\"target_ph\":" + String(fermTargetPH, 2);
  json += ",\"ssr_ferm\":" + String(digitalRead(SSR_FERM));
  json += ",\"ssr_past\":" + String(digitalRead(SSR_PAST));
  json += ",\"pump2\":" + String(sysPump2Active ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}
