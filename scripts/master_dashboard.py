#!/usr/bin/env python3
"""
WineBrew Master Web Dashboard & Telemetry Server
Hosts a comprehensive, real-time brewing monitor and batch CSV manager.
Port: 3080
"""

import os
import sys
import time
import json
import glob
import urllib.request
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

ESP32_IP = "192.168.1.137"
PORT = 3080
LOG_DIR = os.path.expanduser("~/winebrew-logs")

HTML_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WineBrew System Master Dashboard</title>
<style>
  :root {
    --bg: #090d16;
    --card-bg: #131b2e;
    --card-border: #1e293b;
    --text-main: #f8fafc;
    --text-muted: #94a3b8;
    --accent: #38bdf8;
    --green: #10b981;
    --yellow: #f59e0b;
    --red: #ef4444;
    --blue: #3b82f6;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
  body { background: var(--bg); color: var(--text-main); padding: 1.5rem 1rem; max-width: 1120px; margin: 0 auto; line-height: 1.4; }
  
  /* Header */
  .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.25rem; flex-wrap: wrap; gap: 1rem; }
  .title-group h1 { font-size: 1.55rem; color: #fff; font-weight: 800; letter-spacing: -0.02em; }
  .title-group p { font-size: 0.85rem; color: var(--text-muted); margin-top: 0.2rem; }
  .live-badge { display: inline-flex; align-items: center; gap: 0.45rem; background: rgba(16, 185, 129, 0.15); color: var(--green); padding: 0.4rem 0.85rem; border-radius: 9999px; font-size: 0.75rem; font-weight: 700; border: 1px solid rgba(16, 185, 129, 0.3); }
  .live-dot { width: 8px; height: 8px; background: var(--green); border-radius: 50%; animation: pulse 2s infinite; }
  
  @keyframes pulse {
    0% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7); }
    70% { transform: scale(1); box-shadow: 0 0 0 6px rgba(16, 185, 129, 0); }
    100% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
  }

  /* Stage Banner */
  .stage-banner { background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%); border: 1px solid #334155; border-radius: 14px; padding: 1.15rem 1.4rem; margin-bottom: 1.25rem; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 1rem; box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3); }
  .stage-info { display: flex; align-items: center; gap: 1.25rem; }
  .stage-pill { padding: 0.45rem 1.1rem; border-radius: 8px; font-weight: 800; font-size: 1rem; letter-spacing: 0.04em; text-transform: uppercase; }
  .stage-preheat { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.4); }
  .stage-ferm { background: rgba(245, 158, 11, 0.2); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.4); }
  .stage-past { background: rgba(16, 185, 129, 0.2); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.4); }
  .stage-idle { background: rgba(148, 163, 184, 0.2); color: #cbd5e1; border: 1px solid rgba(148, 163, 184, 0.4); }
  
  /* Target Pills Grid */
  .targets-grid { display: flex; flex-wrap: wrap; gap: 0.5rem; align-items: center; }
  .target-pill { display: flex; flex-direction: column; background: rgba(15, 23, 42, 0.7); border: 1px solid rgba(255, 255, 255, 0.08); padding: 0.35rem 0.65rem; border-radius: 8px; min-width: 80px; text-align: center; }
  .target-pill-label { font-size: 0.65rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.04em; font-weight: 600; }
  .target-pill-val { font-size: 0.95rem; font-weight: 800; color: #fff; margin-top: 0.1rem; }
  .target-pill.active { background: rgba(16, 185, 129, 0.15); border-color: rgba(16, 185, 129, 0.4); }
  .target-pill.active .target-pill-label { color: #34d399; }
  .target-pill.active .target-pill-val { color: #10b981; }

  /* Grid Layout (Uniform 2x2) */
  .dashboard-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 1.25rem; margin-bottom: 1.25rem; }
  @media (max-width: 768px) {
    .dashboard-grid { grid-template-columns: 1fr; }
  }
  
  /* Cards */
  .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 1.25rem; display: flex; flex-direction: column; justify-content: space-between; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.2); min-height: 250px; }
  .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.9rem; padding-bottom: 0.5rem; border-bottom: 1px solid rgba(255, 255, 255, 0.06); }
  .card-title { font-size: 0.75rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.06em; }
  
  .metric-row { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 0.65rem; }
  .metric-row:last-child { margin-bottom: 0; }
  .metric-label { font-size: 0.85rem; color: var(--text-muted); }
  .metric-val { font-size: 1.25rem; font-weight: 700; color: #fff; }
  .metric-val.large { font-size: 1.8rem; color: #34d399; }
  .metric-unit { font-size: 0.75rem; color: var(--text-muted); margin-left: 0.2rem; font-weight: 400; }

  /* Progress Bar */
  .bar-container { width: 100%; height: 6px; background: #1e293b; border-radius: 9999px; overflow: hidden; margin-top: 0.35rem; }
  .bar-fill { height: 100%; background: var(--accent); border-radius: 9999px; transition: width 0.4s ease; }
  .bar-fill.heat { background: linear-gradient(90deg, #f59e0b, #ef4444); }

  /* Action & Export Bar */
  .export-bar { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 1.15rem 1.4rem; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 1rem; margin-bottom: 1.25rem; }
  .btn { display: inline-flex; align-items: center; gap: 0.5rem; background: #0284c7; color: #fff; padding: 0.6rem 1.2rem; border-radius: 8px; text-decoration: none; font-weight: 700; font-size: 0.85rem; border: none; cursor: pointer; transition: background 0.2s; }
  .btn:hover { background: #0369a1; }
  .btn-outline { background: transparent; border: 1px solid #3f3f46; color: var(--text-muted); }
  .btn-outline:hover { background: #27272a; color: #fff; }

  /* Historical Files Table */
  .history-card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 1.25rem; }
  .history-list { list-style: none; margin-top: 0.75rem; }
  .history-item { display: flex; justify-content: space-between; align-items: center; padding: 0.55rem 0; border-bottom: 1px solid rgba(255,255,255,0.04); font-size: 0.85rem; }
  .history-item:last-child { border-bottom: none; }
</style>
</head>
<body>

  <div class="header">
    <div class="title-group">
      <h1>WineBrew Master Dashboard</h1>
      <p>Automated Wine Brewing & Distillation Research System</p>
    </div>
    <div class="live-badge">
      <div class="live-dot"></div>
      <span id="conn-status">ESP32 ONLINE (192.168.1.137)</span>
    </div>
  </div>

  <!-- Stage Status Banner -->
  <div class="stage-banner">
    <div class="stage-info">
      <div id="stage-badge" class="stage-pill stage-preheat">STANDBY</div>
      <div>
        <div style="font-size: 0.7rem; color: var(--text-muted); text-transform: uppercase; font-weight: 600; letter-spacing: 0.05em;">Current Batch Volume</div>
        <div style="font-size: 1.4rem; font-weight: 800; color: #fff;" id="top-vol">0.00 L</div>
      </div>
    </div>
    <div class="targets-grid">
      <div class="target-pill">
        <span class="target-pill-label">Preheat</span>
        <span class="target-pill-val" id="tgt-ph">40.0°C</span>
      </div>
      <div class="target-pill">
        <span class="target-pill-label">Cooling</span>
        <span class="target-pill-val" id="tgt-cool">38.0°C</span>
      </div>
      <div class="target-pill">
        <span class="target-pill-label">Ferm</span>
        <span class="target-pill-val" id="tgt-ferm">30.0°C</span>
      </div>
      <div class="target-pill">
        <span class="target-pill-label">Past</span>
        <span class="target-pill-val" id="tgt-past">72.0°C</span>
      </div>
      <div class="target-pill active">
        <span class="target-pill-label">Active Target</span>
        <span class="target-pill-val" id="tgt-active">40.0°C</span>
      </div>
    </div>
  </div>

  <div class="dashboard-grid">
    
    <!-- 1. Chamber Temperatures -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">Chamber Thermal Matrix</span>
        <span style="font-size: 0.75rem; color: var(--text-muted);">DS18B20 & BME280</span>
      </div>
      <div>
        <div class="metric-row">
          <span class="metric-label">Pre-Heat Liquid (Vat)</span>
          <span class="metric-val" id="ll">--<span class="metric-unit">°C</span></span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Pre-Heat Ambient Air</span>
          <span class="metric-val" id="la">--<span class="metric-unit">°C</span></span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Fermentation Liquid</span>
          <span class="metric-val" id="fl">--<span class="metric-unit">°C</span></span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Fermentation Ambient</span>
          <span class="metric-val" id="fa">--<span class="metric-unit">°C</span></span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Pasteurization Liquid</span>
          <span class="metric-val" id="lp">--<span class="metric-unit">°C</span></span>
        </div>
      </div>
    </div>

    <!-- 2. Fermentation Kinetics -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">Fermentation Kinetics</span>
        <span style="font-size: 0.75rem; color: var(--text-muted);">RAPT Pill & pH Probe</span>
      </div>
      <div>
        <div class="metric-row">
          <span class="metric-label">pH Level</span>
          <span class="metric-val large" id="ph">--</span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Specific Gravity (RAPT)</span>
          <span class="metric-val" id="sg">--</span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Estimated ABV</span>
          <span class="metric-val" id="abv">--<span class="metric-unit">%</span></span>
        </div>
        <div class="metric-row">
          <span class="metric-label">RAPT Pill Battery / RSSI</span>
          <span class="metric-val" id="pill-info" style="font-size: 0.95rem; color: var(--text-muted);">--</span>
        </div>
      </div>
    </div>

    <!-- 3. Actuators & Automation -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">Actuators & State Machine</span>
        <span style="font-size: 0.75rem; color: var(--text-muted);">Relays & PWM</span>
      </div>
      <div>
        <div class="metric-row" style="margin-bottom: 0.25rem;">
          <span class="metric-label">SSR Heater Output</span>
          <span class="metric-val" id="hp">0<span class="metric-unit">%</span></span>
        </div>
        <div class="bar-container" style="margin-bottom: 0.75rem;">
          <div class="bar-fill heat" id="hp-bar" style="width: 0%;"></div>
        </div>
        <div class="metric-row">
          <span class="metric-label">Ventilation Fan</span>
          <span class="metric-val" id="fan-state" style="font-size: 0.95rem;">OFF</span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Mixing Impeller</span>
          <span class="metric-val" id="mixer-info" style="font-size: 0.95rem;">OFF (0%)</span>
        </div>
        <div class="metric-row">
          <span class="metric-label">Yeast Dispensed</span>
          <span class="metric-val" id="yeast-disp">0.0<span class="metric-unit">g</span></span>
        </div>
      </div>
    </div>

    <!-- 4. Chamber Volume Distribution -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">Chamber Volume Distribution</span>
        <span style="font-size: 0.75rem; color: var(--text-muted);">HX711 & Flow Sensors</span>
      </div>
      <div>
        <div style="margin-bottom: 0.65rem;">
          <div class="metric-row" style="margin-bottom: 0.2rem;">
            <span class="metric-label">1. Pre-Heat Vat (Scale)</span>
            <span class="metric-val" id="vol-preheat">0.00<span class="metric-unit">L</span></span>
          </div>
          <div class="bar-container">
            <div class="bar-fill" id="bar-preheat" style="width: 0%; background: #f87171;"></div>
          </div>
        </div>

        <div style="margin-bottom: 0.65rem;">
          <div class="metric-row" style="margin-bottom: 0.2rem;">
            <span class="metric-label">2. Fermentation Vessel</span>
            <span class="metric-val" id="vol-ferm">0.00<span class="metric-unit">L</span></span>
          </div>
          <div class="bar-container">
            <div class="bar-fill" id="bar-ferm" style="width: 0%; background: #fbbf24;"></div>
          </div>
        </div>

        <div>
          <div class="metric-row" style="margin-bottom: 0.2rem;">
            <span class="metric-label">3. Pasteurization Tank</span>
            <span class="metric-val" id="vol-past">0.00<span class="metric-unit">L</span></span>
          </div>
          <div class="bar-container">
            <div class="bar-fill" id="bar-past" style="width: 0%; background: #34d399;"></div>
          </div>
        </div>
      </div>
    </div>

  </div>

  <!-- Export & Batch Logger Status -->
  <div class="export-bar">
    <div>
      <div style="font-weight: 700; font-size: 0.95rem;">Autonomous 24/7 Telemetry Logging Active</div>
      <div style="font-size: 0.8rem; color: var(--text-muted); margin-top: 0.2rem;">Telemetry is logged to Beelink every 5 seconds with second-precision timestamps.</div>
    </div>
    <div style="display: flex; gap: 0.75rem;">
      <a href="/download/latest" class="btn">📥 Download Active Batch CSV</a>
    </div>
  </div>

  <!-- Batch Files Explorer -->
  <div class="history-card">
    <div class="card-title" style="margin-bottom: 0.5rem;">Recorded Batch CSV Logs on Server</div>
    <div id="history-container">
      <div style="font-size: 0.85rem; color: var(--text-muted); padding: 0.5rem 0;">Loading recorded batch files...</div>
    </div>
  </div>

<script>
const STAGES = {
  "-1": { name: "STANDBY / COMPLETE", cls: "stage-idle" },
  "0": { name: "PRE-HEATING", cls: "stage-preheat" },
  "1": { name: "FERMENTATION", cls: "stage-ferm" },
  "2": { name: "PASTEURIZATION", cls: "stage-past" }
};

async function updateData() {
  try {
    const r = await fetch('/api/data');
    if (!r.ok) throw new Error();
    const d = await r.json();

    document.getElementById('conn-status').innerText = 'ESP32 ONLINE (192.168.1.137)';
    document.getElementById('conn-status').parentElement.style.color = 'var(--green)';

    // Stage
    const st = STAGES[d.stage] || { name: 'STANDBY', cls: 'stage-idle' };
    const sb = document.getElementById('stage-badge');
    sb.innerText = st.name;
    sb.className = 'stage-pill ' + st.cls;

    // Targets
    document.getElementById('tgt-ph').innerText = (d.tgt_ph !== undefined ? d.tgt_ph : 40.0).toFixed(1) + '°C';
    document.getElementById('tgt-cool').innerText = (d.tgt_cool !== undefined ? d.tgt_cool : (d.coolT || 38.0)).toFixed(1) + '°C';
    document.getElementById('tgt-ferm').innerText = (d.tgt_ferm !== undefined ? d.tgt_ferm : (d.fermTgt || 30.0)).toFixed(1) + '°C';
    document.getElementById('tgt-past').innerText = (d.tgt_past !== undefined ? d.tgt_past : 72.0).toFixed(1) + '°C';
    document.getElementById('tgt-active').innerText = (d.targetT || 40.0).toFixed(1) + '°C';

    // Temps
    document.getElementById('ll').innerHTML = (d.ll ? d.ll.toFixed(1) : '--') + '<span class="metric-unit">°C</span>';
    document.getElementById('la').innerHTML = (d.la ? d.la.toFixed(1) : '--') + '<span class="metric-unit">°C</span>';
    document.getElementById('fl').innerHTML = (d.fl ? d.fl.toFixed(1) : '--') + '<span class="metric-unit">°C</span>';
    document.getElementById('fa').innerHTML = (d.fa ? d.fa.toFixed(1) : '--') + '<span class="metric-unit">°C</span>';
    document.getElementById('lp').innerHTML = (d.lp ? d.lp.toFixed(1) : '--') + '<span class="metric-unit">°C</span>';

    // Kinetics
    document.getElementById('ph').innerText = d.ph ? d.ph.toFixed(2) : '--';
    document.getElementById('sg').innerText = (d.sg && d.sg < 10.0) ? d.sg.toFixed(4) : '--';
    document.getElementById('abv').innerHTML = (d.abv !== undefined ? d.abv.toFixed(2) : '0.00') + '<span class="metric-unit">%</span>';
    document.getElementById('pill-info').innerText = (d.bat || 0) + '% (' + (d.rssi || 0) + ' dBm)';

    // Actuators
    const hp = d.hp || 0;
    document.getElementById('hp').innerHTML = hp + '<span class="metric-unit">%</span>';
    document.getElementById('hp-bar').style.width = hp + '%';

    const fanStates = { 0: 'OFF', 1: 'PREHEAT FAN (ON)', 2: 'FERM FAN (ON)' };
    document.getElementById('fan-state').innerText = fanStates[d.fan] || 'OFF';

    const mixerModes = { 0: 'OFF', 1: 'MANUAL', 2: 'AUTO' };
    document.getElementById('mixer-info').innerText = (mixerModes[d.mm] || 'OFF') + ' (' + (d.msp || 0) + '%)';

    document.getElementById('yeast-disp').innerHTML = (d.yd !== undefined ? d.yd.toFixed(1) : '0.0') + '<span class="metric-unit">g</span>';

    // 3-Chamber Volume Distribution & Baseline Management
    let rawScale = (d.vol !== undefined && d.vol > 0.0) ? d.vol : 0.0;
    let savedTotal = parseFloat(localStorage.getItem('winebrew_batch_volume')) || 12.14;

    if (d.stage === 0 && rawScale > 5.0) {
      savedTotal = rawScale;
      localStorage.setItem('winebrew_batch_volume', savedTotal.toString());
    }

    let vPre = 0.0;
    let vFerm = 0.0;
    let vPast = 0.0;

    if (d.stage === 0) {
      vPre = rawScale;
      vFerm = 0.0;
      vPast = 0.0;
    } else if (d.stage === 1) {
      vPre = rawScale;
      vFerm = (d.v_xfer !== undefined && d.v_xfer > 0.1) ? d.v_xfer : Math.max(0.0, savedTotal - rawScale);
      vPast = 0.0;
    } else if (d.stage === 2) {
      vPre = rawScale;
      vFerm = 0.0;
      vPast = (d.v_xfer !== undefined && d.v_xfer > 0.1) ? d.v_xfer : Math.max(0.0, savedTotal - rawScale);
    } else {
      // Stage -1 (Standby / Finished)
      vPre = rawScale;
      if (rawScale < 1.0) {
        vPast = savedTotal;
        vFerm = 0.0;
      } else {
        vPast = Math.max(0.0, savedTotal - rawScale);
      }
    }

    let activeDisplayVol = (d.stage === 0) ? vPre : (d.stage === 1 ? vFerm : (d.stage === 2 ? vPast : (vPast > 0 ? vPast : vPre)));
    document.getElementById('top-vol').innerText = activeDisplayVol.toFixed(2) + ' L';

    document.getElementById('vol-preheat').innerHTML = vPre.toFixed(2) + '<span class="metric-unit">L</span>';
    document.getElementById('bar-preheat').style.width = Math.min(100, (vPre / 15.0) * 100) + '%';

    document.getElementById('vol-ferm').innerHTML = vFerm.toFixed(2) + '<span class="metric-unit">L</span>';
    document.getElementById('bar-ferm').style.width = Math.min(100, (vFerm / 15.0) * 100) + '%';

    document.getElementById('vol-past').innerHTML = vPast.toFixed(2) + '<span class="metric-unit">L</span>';
    document.getElementById('bar-past').style.width = Math.min(100, (vPast / 15.0) * 100) + '%';

  } catch(e) {
    document.getElementById('conn-status').innerText = 'ESP32 RECONNECTING...';
    document.getElementById('conn-status').parentElement.style.color = 'var(--yellow)';
  }
}

async function loadHistory() {
  try {
    const r = await fetch('/api/logs');
    const files = await r.json();
    const c = document.getElementById('history-container');
    if (!files || files.length === 0) {
      c.innerHTML = '<div style="font-size:0.85rem;color:var(--text-muted);padding:0.5rem 0;">No batch logs recorded yet.</div>';
      return;
    }
    let html = '<ul class="history-list">';
    files.forEach(f => {
      html += `
        <li class="history-item">
          <div>
            <strong>📄 ${f.name}</strong>
            <span style="color:var(--text-muted);margin-left:0.5rem;font-size:0.75rem;">(${f.size})</span>
          </div>
          <a href="/download/${f.name}" class="btn btn-outline" style="padding:0.3rem 0.75rem;font-size:0.75rem;">Download</a>
        </li>
      `;
    });
    html += '</ul>';
    c.innerHTML = html;
  } catch(e) {
    document.getElementById('history-container').innerHTML = '<div style="font-size:0.85rem;color:var(--red);">Failed to load batch files list.</div>';
  }
}

setInterval(updateData, 1000);
updateData();
loadHistory();
setInterval(loadHistory, 30000);
</script>
</body>
</html>
"""

class DashboardHandler(BaseHTTPRequestHandler):
  def do_GET(self):
    parsed = urlparse(self.path)
    
    if parsed.path == "/" or parsed.path == "/index.html":
      self.send_response(200)
      self.send_header("Content-Type", "text/html; charset=utf-8")
      self.end_headers()
      self.wfile.write(HTML_PAGE.encode("utf-8"))
      
    elif parsed.path == "/api/data":
      try:
        req = urllib.request.Request(f"http://{ESP32_IP}/data", headers={"User-Agent": "WineBrewMaster/1.0"})
        with urllib.request.urlopen(req, timeout=2.0) as res:
          data = res.read()
          self.send_response(200)
          self.send_header("Content-Type", "application/json")
          self.send_header("Access-Control-Allow-Origin", "*")
          self.end_headers()
          self.wfile.write(data)
      except Exception as e:
        self.send_response(502)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps({"error": str(e)}).encode("utf-8"))
        
    elif parsed.path == "/api/logs":
      logs = []
      os.makedirs(LOG_DIR, exist_ok=True)
      for p in sorted(glob.glob(os.path.join(LOG_DIR, "*.csv")), key=os.path.getmtime, reverse=True):
        size_bytes = os.path.getsize(p)
        size_str = f"{size_bytes / 1024:.1f} KB" if size_bytes < 1024*1024 else f"{size_bytes / (1024*1024):.2f} MB"
        logs.append({
          "name": os.path.basename(p),
          "size": size_str,
          "mtime": os.path.getmtime(p)
        })
      self.send_response(200)
      self.send_header("Content-Type", "application/json")
      self.end_headers()
      self.wfile.write(json.dumps(logs).encode("utf-8"))
      
    elif parsed.path.startswith("/download/"):
      fname = parsed.path[len("/download/"):]
      os.makedirs(LOG_DIR, exist_ok=True)
      if fname == "latest":
        files = sorted(glob.glob(os.path.join(LOG_DIR, "*.csv")), key=os.path.getmtime, reverse=True)
        target_path = files[0] if files else None
      else:
        target_path = os.path.join(LOG_DIR, fname)
        
      if target_path and os.path.exists(target_path):
        self.send_response(200)
        self.send_header("Content-Type", "text/csv")
        self.send_header("Content-Disposition", f'attachment; filename="{os.path.basename(target_path)}"')
        self.end_headers()
        with open(target_path, "rb") as f:
          self.wfile.write(f.read())
      else:
        self.send_response(404)
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(b"Log file not found")
    else:
      self.send_response(404)
      self.end_headers()

  def log_message(self, format, *args):
    pass

def run():
  server = HTTPServer(("0.0.0.0", PORT), DashboardHandler)
  print(f"[WineBrew Dashboard] Listening on http://0.0.0.0:{PORT} (Proxying {ESP32_IP})")
  server.serve_forever()

if __name__ == "__main__":
  run()
