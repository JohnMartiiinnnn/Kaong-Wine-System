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
  json += "\"ll\":" + String(liquid2Status ? sharedLiquidSensors.getTempCByIndex(1) : 0) + ",";
  json += "\"fa\":" + String(incomingData.room2Temp) + ",";
  json += "\"fl\":" + String(incomingData.room2LiquidTemp) + ",";
  json += "\"ph\":" + String(incomingData.phValue) + ",";
  json += "\"sg\":" + String(incomingData.pillGravity) + ",";
  json += "\"bat\":" + String(incomingData.pillBattery) + ",";
  json += "\"rssi\":" + String(incomingData.pillRSSI) + ",";
  json += "\"pt\":" + String(incomingData.pillTemp, 1) + ",";
  float abv = (originalGravity > 0 && incomingData.pillGravity > 0 && incomingData.pillGravity < 10.0)
              ? max(0.0f, (originalGravity - incomingData.pillGravity) * 131.25f) : 0.0f;
  json += "\"abv\":" + String(abv) + ",";
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
  json += "\"lp\":" + String(liquid1Status ? sharedLiquidSensors.getTempCByIndex(0) : 0.0f, 1);
  json += "}";
  server.send(200, "application/json", json);
}
