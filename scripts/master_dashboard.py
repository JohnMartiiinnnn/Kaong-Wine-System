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
  body { background: var(--bg); color: var(--text-main); padding: 1.5rem 1rem; max-width: 1100px; margin: 0 auto; }
  
  /* Header */
  .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.5rem; flex-wrap: wrap; gap: 1rem; }
  .title-group h1 { font-size: 1.6rem; color: #fff; font-weight: 800; letter-spacing: -0.02em; }
  .title-group p { font-size: 0.85rem; color: var(--text-muted); margin-top: 0.2rem; }
  .live-badge { display: inline-flex; align-items: center; gap: 0.4rem; background: rgba(16, 185, 129, 0.15); color: var(--green); padding: 0.35rem 0.8rem; border-radius: 9999px; font-size: 0.75rem; font-weight: 700; border: 1px solid rgba(16, 185, 129, 0.3); }
  .live-dot { width: 8px; height: 8px; background: var(--green); border-radius: 50%; animation: pulse 2s infinite; }
  
  @keyframes pulse {
    0% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7); }
    70% { transform: scale(1); box-shadow: 0 0 0 6px rgba(16, 185, 129, 0); }
    100% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
  }

  /* Stage Banner */
  .stage-banner { background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%); border: 1px solid #334155; border-radius: 16px; padding: 1.25rem 1.5rem; margin-bottom: 1.5rem; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 1rem; box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.3); }
  .stage-info { display: flex; align-items: center; gap: 1rem; }
  .stage-pill { padding: 0.5rem 1.2rem; border-radius: 8px; font-weight: 800; font-size: 1.1rem; letter-spacing: 0.05em; text-transform: uppercase; }
  .stage-preheat { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.4); }
  .stage-ferm { background: rgba(245, 158, 11, 0.2); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.4); }
  .stage-past { background: rgba(16, 185, 129, 0.2); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.4); }
  .stage-idle { background: rgba(148, 163, 184, 0.2); color: #cbd5e1; border: 1px solid rgba(148, 163, 184, 0.4); }
  .targets { display: flex; gap: 1.5rem; font-size: 0.85rem; color: var(--text-muted); }
  .targets span strong { color: #fff; font-size: 1rem; }

  /* Grid Layout */
  .dashboard-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 1.25rem; margin-bottom: 1.5rem; }
  
  /* Cards */
  .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 1.25rem; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.2); }
  .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; padding-bottom: 0.5rem; border-bottom: 1px solid rgba(255, 255, 255, 0.05); }
  .card-title { font-size: 0.8rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.05em; }
  
  .metric-row { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 0.75rem; }
  .metric-label { font-size: 0.85rem; color: var(--text-muted); }
  .metric-val { font-size: 1.35rem; font-weight: 700; color: #fff; }
  .metric-val.large { font-size: 2rem; color: var(--accent); }
  .metric-unit { font-size: 0.8rem; color: var(--text-muted); margin-left: 0.2rem; font-weight: 400; }

  /* Progress Bar */
  .bar-container { width: 100%; height: 8px; background: #1e293b; border-radius: 9999px; overflow: hidden; margin-top: 0.4rem; }
  .bar-fill { height: 100%; background: var(--accent); border-radius: 9999px; transition: width 0.4s ease; }
  .bar-fill.heat { background: linear-gradient(90deg, #f59e0b, #ef4444); }

  /* Action Buttons */
  .export-bar { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 1.25rem; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 1rem; margin-bottom: 1.5rem; }
  .btn { display: inline-flex; align-items: center; gap: 0.5rem; background: #0284c7; color: #fff; padding: 0.65rem 1.25rem; border-radius: 8px; text-decoration: none; font-weight: 700; font-size: 0.85rem; border: none; cursor: pointer; transition: background 0.2s; }
  .btn:hover { background: #0369a1; }
  .btn-outline { background: transparent; border: 1px solid #334155; color: var(--text-muted); }
  .btn-outline:hover { background: #1e293b; color: #fff; }

  /* Historical Files Table */
  .history-card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 1.25rem; }
  .history-list { list-style: none; margin-top: 0.75rem; }
  .history-item { display: flex; justify-content: space-between; align-items: center; padding: 0.6rem 0; border-bottom: 1px solid rgba(255,255,255,0.05); font-size: 0.85rem; }
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
      <div id="stage-badge" class="stage-pill stage-preheat">PRE-HEATING</div>
      <div>
        <div style="font-size: 0.75rem; color: var(--text-muted); text-transform: uppercase;">Active Volume</div>
        <div style="font-size: 1.5rem; font-weight: 800; color: #fff;" id="top-vol">-- L</div>
      </div>
    </div>
    <div class="targets">
      <div>Preheat Target: <strong id="tgt-ph">40.0°C</strong></div>
      <div>Cooling Target: <strong id="tgt-cool">38.0°C</strong></div>
      <div>Ferm Target: <strong id="tgt-ferm">30.0°C</strong></div>
    </div>
  </div>

  <div class="dashboard-grid">
    
    <!-- Chamber Temperatures -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">Chamber Thermal Matrix</span>
      </div>
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

    <!-- Fermentation Kinetics -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">Fermentation Kinetics</span>
      </div>
      <div class="metric-row">
        <span class="metric-label">pH Level</span>
        <span class="metric-val large" id="ph" style="color: #34d399;">--</span>
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
        <span class="metric-label">RAPT Pill Signal / Battery</span>
        <span class="metric-val" id="pill-info" style="font-size: 1rem; color: var(--text-muted);">--</span>
      </div>
    </div>

    <!-- Actuators & Automation -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">Actuators & Automation</span>
      </div>
      <div class="metric-row">
        <span class="metric-label">SSR Heater Output</span>
        <span class="metric-val" id="hp">0<span class="metric-unit">%</span></span>
      </div>
      <div class="bar-container" style="margin-bottom: 1rem;">
        <div class="bar-fill heat" id="hp-bar" style="width: 0%;"></div>
      </div>
      <div class="metric-row">
        <span class="metric-label">Ventilation Fan</span>
        <span class="metric-val" id="fan-state" style="font-size: 1rem;">OFF</span>
      </div>
      <div class="metric-row">
        <span class="metric-label">Mixing Impeller</span>
        <span class="metric-val" id="mixer-info" style="font-size: 1rem;">OFF (0%)</span>
      </div>
      <div class="metric-row">
        <span class="metric-label">Yeast Dispensed</span>
        <span class="metric-val" id="yeast-disp">0.0<span class="metric-unit">g</span></span>
      </div>
    </div>

  </div>

  <!-- Export & Batch Logger Status -->
  <div class="export-bar">
    <div>
      <div style="font-weight: 700; font-size: 1rem;">Autonomous 24/7 Telemetry Logging Active</div>
      <div style="font-size: 0.8rem; color: var(--text-muted); margin-top: 0.2rem;">Data is automatically logged to Beelink server and ready for export.</div>
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
  "-1": { name: "IDLE / STANDBY", cls: "stage-idle" },
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
    const st = STAGES[d.stage] || { name: 'UNKNOWN', cls: 'stage-idle' };
    const sb = document.getElementById('stage-badge');
    sb.innerText = st.name;
    sb.className = 'stage-pill ' + st.cls;

    // Top volume
    document.getElementById('top-vol').innerText = (d.vol !== undefined ? d.vol.toFixed(2) : '--') + ' L';

    // Targets
    document.getElementById('tgt-ph').innerText = (d.targetT || 40.0).toFixed(1) + '°C';
    document.getElementById('tgt-cool').innerText = (d.coolT || 38.0).toFixed(1) + '°C';
    document.getElementById('tgt-ferm').innerText = (d.fermTgt || 30.0).toFixed(1) + '°C';

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

  } catch(e) {
    document.getElementById('conn-status').innerText = 'ESP32 RECONNECTING...';
    document.getElementById('conn-status').parentElement.style.color = 'var(--yellow)';
  }
}

async function loadHistory() {
  try {
    const r = await fetch('/api/logs');
    const files = await r.json();
    const cont = document.getElementById('history-container');
    if (files.length === 0) {
      cont.innerHTML = '<div style="font-size: 0.85rem; color: var(--text-muted); padding: 0.5rem 0;">No saved CSV batch files yet.</div>';
      return;
    }
    let html = '<ul class="history-list">';
    files.forEach(f => {
      html += `<li class="history-item">
        <span>📄 <strong>${f.name}</strong> <span style="color:var(--text-muted);margin-left:0.5rem">(${f.size})</span></span>
        <a href="/download/${f.name}" class="btn btn-outline" style="padding:0.3rem 0.75rem;font-size:0.75rem">Download CSV</a>
      </li>`;
    });
    html += '</ul>';
    cont.innerHTML = html;
  } catch(e) {}
}

setInterval(updateData, 1000);
updateData();
loadHistory();
setInterval(loadHistory, 10000);
</script>
</body>
</html>
"""

class DashboardHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/" or path == "/index.html":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))

        elif path == "/api/data":
            try:
                req = urllib.request.Request(f"http://{ESP32_IP}/data", headers={"User-Agent": "WineBrewDashboard/1.0"})
                with urllib.request.urlopen(req, timeout=2.5) as resp:
                    raw = resp.read()
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.send_header("Access-Control-Allow-Origin", "*")
                    self.end_headers()
                    self.wfile.write(raw)
            except Exception:
                self.send_response(502)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(b'{"error":"ESP32 unreachable"}')

        elif path == "/api/logs":
            files = []
            if os.path.exists(LOG_DIR):
                for f in sorted(glob.glob(os.path.join(LOG_DIR, "*.csv")), reverse=True):
                    sz_bytes = os.path.getsize(f)
                    sz_str = f"{sz_bytes / 1024:.1f} KB" if sz_bytes < 1024*1024 else f"{sz_bytes / (1024*1024):.1f} MB"
                    files.append({"name": os.path.basename(f), "size": sz_str})
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(files).encode("utf-8"))

        elif path.startswith("/download/"):
            fname = path.replace("/download/", "")
            if fname == "latest":
                flist = sorted(glob.glob(os.path.join(LOG_DIR, "*.csv")), reverse=True)
                target = flist[0] if flist else None
            else:
                target = os.path.join(LOG_DIR, fname)

            if target and os.path.exists(target):
                self.send_response(200)
                self.send_header("Content-Type", "text/csv")
                self.send_header("Content-Disposition", f'attachment; filename="{os.path.basename(target)}"')
                self.end_headers()
                with open(target, "rb") as f:
                    self.wfile.write(f.read())
            else:
                self.send_response(404)
                self.send_header("Content-Type", "text/plain")
                self.end_headers()
                self.wfile.write(b"File not found")
        else:
            self.send_response(404)
            self.end_headers()

def run_server():
    server = HTTPServer(("0.0.0.0", PORT), DashboardHandler)
    print(f"WineBrew Master Dashboard running on http://0.0.0.0:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    server.server_close()

if __name__ == "__main__":
    run_server()
