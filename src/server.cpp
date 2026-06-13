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
  }catch(e){}
}
setInterval(update,1000);
</script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void handleData() {
  String json = "{";
  json += "\"vol\":" + String(currentWeight) + ",";
  json += "\"la\":" + String(bme1Status ? bme1.readTemperature() : 0) + ",";
  json += "\"ll\":" + String(liquid1Status ? sharedLiquidSensors.getTempCByIndex(0) : 0) + ",";
  json += "\"fa\":" + String(incomingData.room2Temp) + ",";
  json += "\"fl\":" + String(incomingData.room2LiquidTemp) + ",";
  json += "\"ph\":" + String(incomingData.phValue) + ",";
  json += "\"sg\":" + String(incomingData.pillGravity) + ",";
  float abv = (originalGravity > 0 && incomingData.pillGravity > 0 && incomingData.pillGravity < 10.0)
              ? max(0.0f, (originalGravity - incomingData.pillGravity) * 131.25f) : 0.0f;
  json += "\"abv\":" + String(abv);
  json += "}";
  server.send(200, "application/json", json);
}
