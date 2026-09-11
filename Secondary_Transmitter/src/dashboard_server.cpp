#include "dashboard_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "YeastDispenser.h"

// Access Point Settings
static const char *AP_SSID = "Secondary uController";
static const char *AP_PASS = ""; // Open network for effortless captive portal onboarding
static const byte DNS_PORT = 53;

static IPAddress apIP(192, 168, 4, 1);
static IPAddress netMask(255, 255, 255, 0);

static WebServer server(80);
static DNSServer dnsServer;
static bool apActive = false;

// HTML Dashboard content stored in flash memory
static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Secondary Sensor Dashboard</title>
  <style>
    :root {
      --bg: #0b1120;
      --card-bg: rgba(30, 41, 59, 0.7);
      --card-border: rgba(255, 255, 255, 0.08);
      --card-hover: rgba(51, 65, 85, 0.8);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --cyan: #06b6d4;
      --emerald: #10b981;
      --amber: #f59e0b;
      --rose: #f43f5e;
      --purple: #a855f7;
      --blue: #3b82f6;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }
    body {
      background: radial-gradient(circle at top right, #1e1b4b, #0b1120 70%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .container {
      width: 100%;
      max-width: 900px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    header {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 16px 20px;
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.4);
    }
    .header-title {
      display: flex;
      flex-direction: column;
      gap: 4px;
    }
    .header-title h1 {
      font-size: 1.25rem;
      font-weight: 700;
      letter-spacing: -0.02em;
      background: linear-gradient(135deg, #38bdf8, #818cf8);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }
    .header-title .subtitle {
      font-size: 0.8rem;
      color: var(--text-muted);
    }
    .status-badge {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 6px 14px;
      border-radius: 9999px;
      font-size: 0.8rem;
      font-weight: 600;
      background: rgba(16, 185, 129, 0.15);
      color: #34d399;
      border: 1px solid rgba(16, 185, 129, 0.3);
    }
    .pulse-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background-color: currentColor;
      box-shadow: 0 0 8px currentColor;
      animation: pulse 1.8s infinite ease-in-out;
    }
    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.4; transform: scale(0.85); }
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 16px;
    }
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 18px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      gap: 12px;
      box-shadow: 0 4px 20px rgba(0, 0, 0, 0.2);
      transition: transform 0.2s ease, border-color 0.2s ease;
    }
    .card:hover {
      transform: translateY(-2px);
      border-color: rgba(255, 255, 255, 0.2);
    }
    .card-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .card-label {
      font-size: 0.8rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      color: var(--text-muted);
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .card-tag {
      font-size: 0.7rem;
      padding: 3px 8px;
      border-radius: 6px;
      font-weight: 600;
    }
    .tag-ok { background: rgba(16, 185, 129, 0.2); color: #34d399; }
    .tag-warn { background: rgba(245, 158, 11, 0.2); color: #fbbf24; }
    .tag-err { background: rgba(244, 63, 94, 0.2); color: #fb7185; }

    .main-val {
      font-size: 2.2rem;
      font-weight: 700;
      letter-spacing: -0.03em;
      line-height: 1.1;
    }
    .unit {
      font-size: 1rem;
      font-weight: 500;
      color: var(--text-muted);
      margin-left: 4px;
    }
    .card-meta {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      padding-top: 10px;
      border-top: 1px solid var(--card-border);
      font-size: 0.75rem;
      color: var(--text-muted);
    }
    .meta-item {
      display: flex;
      align-items: center;
      gap: 4px;
    }
    .meta-item strong {
      color: var(--text-main);
    }

    .val-cyan { color: #38bdf8; }
    .val-emerald { color: #34d399; }
    .val-amber { color: #fbbf24; }
    .val-purple { color: #c084fc; }
    .val-rose { color: #fb7185; }
    .val-blue { color: #60a5fa; }

    footer {
      text-align: center;
      font-size: 0.75rem;
      color: var(--text-muted);
      padding: 12px 0;
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 8px;
    }
    .unit-btn {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      color: var(--text-main);
      padding: 4px 10px;
      border-radius: 8px;
      font-size: 0.75rem;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="header-title">
        <h1>Secondary uController</h1>
        <div class="subtitle" id="nodeStatus">Standalone Access Point &bull; Real-Time Sensor Node</div>
      </div>
      <div class="status-badge" id="uartBadge">
        <span class="pulse-dot"></span>
        <span id="uartStatusText">AP Active</span>
      </div>
    </header>

    <div class="grid">
      <!-- RAPT Pill SG -->
      <div class="card">
        <div class="card-head">
          <span class="card-label">🍺 Specific Gravity</span>
          <span class="card-tag tag-ok" id="pillTag">RAPT Pill</span>
        </div>
        <div class="main-val val-cyan" id="pillSG">---</div>
        <div class="card-meta">
          <div class="meta-item">Plato: <strong id="pillPlato">---</strong></div>
          <div class="meta-item">Temp: <strong id="pillTemp">---</strong></div>
          <div class="meta-item">Bat: <strong id="pillBat">---</strong></div>
          <div class="meta-item">RSSI: <strong id="pillRSSI">---</strong></div>
        </div>
      </div>

      <!-- Liquid Temp -->
      <div class="card">
        <div class="card-head">
          <span class="card-label">🌡️ Liquid Temp</span>
          <span class="card-tag tag-ok" id="liquidTag">DS18B20</span>
        </div>
        <div class="main-val val-emerald" id="liquidTemp">---</div>
        <div class="card-meta">
          <div class="meta-item">Chamber: <strong>Fermentation</strong></div>
          <div class="meta-item">Status: <strong id="liquidStatus">Connected</strong></div>
        </div>
      </div>

      <!-- Ambient Temp & Pressure -->
      <div class="card">
        <div class="card-head">
          <span class="card-label">🌤️ Ambient Chamber</span>
          <span class="card-tag tag-ok" id="ambientTag">BME/BMP</span>
        </div>
        <div class="main-val val-amber" id="ambientTemp">---</div>
        <div class="card-meta">
          <div class="meta-item">Pressure: <strong id="ambientPres">---</strong></div>
          <div class="meta-item">Sensor: <strong id="ambientType">BME280</strong></div>
        </div>
      </div>

      <!-- pH Sensor -->
      <div class="card">
        <div class="card-head">
          <span class="card-label">🧪 pH Level</span>
          <span class="card-tag tag-ok" id="phTag">PH4502C</span>
        </div>
        <div class="main-val val-purple" id="phValue">---</div>
        <div class="card-meta">
          <div class="meta-item">Temp-Comp: <strong>Active (Nernst)</strong></div>
          <div class="meta-item">ADC: <strong id="adsStatus">Connected</strong></div>
        </div>
      </div>

      <!-- Motor & Current Sense -->
      <div class="card">
        <div class="card-head">
          <span class="card-label">⚙️ Mixer & Current</span>
          <span class="card-tag tag-ok" id="motorTag">BTS7960</span>
        </div>
        <div class="main-val val-blue" id="motorRpm">---</div>
        <div class="card-meta">
          <div class="meta-item">Sense: <strong id="motorVolts">---</strong></div>
          <div class="meta-item">Status: <strong id="motorStatus">Normal</strong></div>
        </div>
      </div>

      <!-- Dispenser & Link Status -->
      <div class="card">
        <div class="card-head">
          <span class="card-label">📡 System Diagnostics</span>
          <span class="card-tag tag-ok" id="sysTag">ESP32</span>
        </div>
        <div class="main-val" style="font-size: 1.4rem; color: #e2e8f0; line-height: 1.5;" id="dispenserState">Yeast: Ready</div>
        <div class="card-meta">
          <div class="meta-item">UART Link: <strong id="uartLinkText">Disconnected</strong></div>
          <div class="meta-item">Uptime: <strong id="sysUptime">---</strong></div>
          <div class="meta-item">Clients: <strong id="apClients">---</strong></div>
        </div>
      </div>
    </div>

    <footer>
      <div>Auto-refreshes every 1.0s &bull; Direct Captive Portal</div>
      <button class="unit-btn" id="toggleUnit" onclick="toggleTempUnit()">Switch to &deg;F</button>
    </footer>
  </div>

  <script>
    let useFahrenheit = false;

    function formatTemp(c) {
      if (c === null || c === undefined || c < -50) return '---';
      if (useFahrenheit) {
        return ((c * 9/5) + 32).toFixed(1) + '<span class="unit">&deg;F</span>';
      }
      return c.toFixed(1) + '<span class="unit">&deg;C</span>';
    }

    function toggleTempUnit() {
      useFahrenheit = !useFahrenheit;
      document.getElementById('toggleUnit').innerText = useFahrenheit ? 'Switch to °C' : 'Switch to °F';
      fetchData();
    }

    function calculatePlato(sg) {
      if (!sg || sg <= 0.5) return '---';
      let p = (-1 * 616.868) + (1111.14 * sg) - (630.272 * Math.pow(sg, 2)) + (135.997 * Math.pow(sg, 3));
      return (p > 0 ? p.toFixed(1) : '0.0') + ' &deg;P';
    }

    async function fetchData() {
      try {
        const res = await fetch('/api/data');
        if (!res.ok) return;
        const d = await res.json();

        // 1. RAPT Pill
        if (d.pill && d.pill.gravity > 0.5) {
          document.getElementById('pillSG').innerHTML = d.pill.gravity.toFixed(4);
          document.getElementById('pillPlato').innerHTML = calculatePlato(d.pill.gravity);
          document.getElementById('pillTemp').innerHTML = (d.pill.temp > -50) ? (useFahrenheit ? ((d.pill.temp * 9/5)+32).toFixed(1)+'°F' : d.pill.temp.toFixed(1)+'°C') : '---';
          document.getElementById('pillBat').innerText = d.pill.battery + '%';
          document.getElementById('pillRSSI').innerText = d.pill.rssi + ' dBm';
          document.getElementById('pillTag').className = 'card-tag tag-ok';
          document.getElementById('pillTag').innerText = 'Connected';
        } else {
          document.getElementById('pillSG').innerText = '---';
          document.getElementById('pillPlato').innerText = '---';
          document.getElementById('pillTemp').innerText = '---';
          document.getElementById('pillBat').innerText = '---';
          document.getElementById('pillRSSI').innerText = '---';
          document.getElementById('pillTag').className = 'card-tag tag-warn';
          document.getElementById('pillTag').innerText = 'Scanning...';
        }

        // 2. Liquid Temp
        if (d.liquid && d.liquid.status === 1 && d.liquid.temp > -50) {
          document.getElementById('liquidTemp').innerHTML = formatTemp(d.liquid.temp);
          document.getElementById('liquidStatus').innerText = 'Connected';
          document.getElementById('liquidTag').className = 'card-tag tag-ok';
        } else {
          document.getElementById('liquidTemp').innerText = '---';
          document.getElementById('liquidStatus').innerText = 'Disconnected';
          document.getElementById('liquidTag').className = 'card-tag tag-err';
        }

        // 3. Ambient
        if (d.ambient && d.ambient.sensor !== 'None') {
          document.getElementById('ambientTemp').innerHTML = formatTemp(d.ambient.temp);
          document.getElementById('ambientPres').innerText = d.ambient.pressure.toFixed(1) + ' hPa';
          document.getElementById('ambientType').innerText = d.ambient.sensor;
          document.getElementById('ambientTag').className = 'card-tag tag-ok';
        } else {
          document.getElementById('ambientTemp').innerText = '---';
          document.getElementById('ambientPres').innerText = '---';
          document.getElementById('ambientType').innerText = 'None';
          document.getElementById('ambientTag').className = 'card-tag tag-err';
        }

        // 4. pH
        if (d.ph && d.ph.status === 1) {
          document.getElementById('phValue').innerText = d.ph.value.toFixed(2);
          document.getElementById('adsStatus').innerText = 'ADS1115 OK';
          document.getElementById('phTag').className = 'card-tag tag-ok';
        } else {
          document.getElementById('phValue').innerText = '---';
          document.getElementById('adsStatus').innerText = 'Disconnected';
          document.getElementById('phTag').className = 'card-tag tag-err';
        }

        // 5. Motor
        if (d.motor) {
          let rpm = d.motor.rpm || 0;
          document.getElementById('motorRpm').innerHTML = (rpm > 0.1 ? rpm.toFixed(1) : '0.0') + '<span class="unit">RPM</span>';
          document.getElementById('motorVolts').innerText = d.motor.senseVolts.toFixed(3) + ' V';
          document.getElementById('motorStatus').innerText = (d.motor.senseVolts > 0.353) ? 'Stall Warning' : 'Normal';
        }

        // 6. System & Diagnostics
        if (d.system) {
          document.getElementById('uartLinkText').innerText = d.system.uartConnected ? 'Active' : 'Disconnected';
          document.getElementById('uartBadge').className = d.system.uartConnected ? 'status-badge' : 'status-badge';
          document.getElementById('uartStatusText').innerText = d.system.uartConnected ? 'UART Connected' : 'AP Standalone';
          let m = Math.floor(d.system.uptimeSec / 60);
          let s = d.system.uptimeSec % 60;
          document.getElementById('sysUptime').innerText = `${m}m ${s}s`;
          document.getElementById('apClients').innerText = d.system.apClients;
        }

        if (d.dispenser) {
          document.getElementById('dispenserState').innerText = d.dispenser.dispensing ? 'Yeast: Dispensing...' : 'Yeast: Ready';
        }
      } catch (err) {
        console.error('Fetch error:', err);
      }
    }

    fetchData();
    setInterval(fetchData, 1000);
  </script>
</body>
</html>
)rawliteral";

// Captive Portal Redirect Handler
static void handleCaptivePortalRedirect() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

// Data API Handler
static void handleApiData() {
  portENTER_CRITICAL(&txDataMux);
  struct_message snap = txData;
  portEXIT_CRITICAL(&txDataMux);

  float motorRpm = 0.0f;
  if (snap.motorSenseVolts > 0.035f) {
    motorRpm = 66.0f - 160.19f * (snap.motorSenseVolts - 0.035f);
    if (motorRpm < 0.0f) motorRpm = 0.0f;
    if (motorRpm > 66.0f) motorRpm = 66.0f;
  }

  const char *sensorName = "None";
  if (snap.sensor2Status == 1) sensorName = "BME280";
  else if (snap.sensor2Status == 2) sensorName = "BMP280";

  uint32_t uptime = millis() / 1000;
  int clients = WiFi.softAPgetStationNum();

  char json[512];
  snprintf(json, sizeof(json),
    "{"
      "\"pill\":{\"gravity\":%.4f,\"temp\":%.2f,\"battery\":%u,\"rssi\":%d,\"status\":%u},"
      "\"ambient\":{\"temp\":%.2f,\"pressure\":%.2f,\"sensor\":\"%s\"},"
      "\"liquid\":{\"temp\":%.2f,\"status\":%u},"
      "\"ph\":{\"value\":%.2f,\"status\":%u},"
      "\"motor\":{\"senseVolts\":%.3f,\"rpm\":%.1f},"
      "\"dispenser\":{\"dispensing\":%s,\"msPerGram\":%.1f},"
      "\"system\":{\"uartConnected\":%s,\"lastUartRxSecAgo\":%lu,\"apClients\":%d,\"uptimeSec\":%lu,\"freeHeap\":%u}"
    "}",
    snap.pillGravity, snap.pillTemp, snap.pillBattery, snap.pillRSSI, snap.bleStatus,
    snap.room2Temp, snap.room2Pres, sensorName,
    snap.room2LiquidTemp, snap.ds18Status,
    snap.phValue, snap.adsStatus,
    snap.motorSenseVolts, motorRpm,
    isYeastDispensing ? "true" : "false", msPerGramYeast,
    isUartConnected ? "true" : "false", (millis() - lastUartRxMs) / 1000, clients, (unsigned long)uptime, (unsigned int)ESP.getFreeHeap()
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// Root page
static void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send_P(200, "text/html", INDEX_HTML);
}

void initDashboardServer() {
  // Setup routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);

  // Android captive portal probes
  server.on("/generate_204", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/gen_204", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/connectivitycheck.gstatic.com/generate_204", HTTP_GET, handleCaptivePortalRedirect);

  // iOS / macOS captive portal probes
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/library/test/success.html", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/success.txt", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/canonical.html", HTTP_GET, handleCaptivePortalRedirect);

  // Windows captive portal probes
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/redirect", HTTP_GET, handleCaptivePortalRedirect);

  // Catch-all for any other captive portal redirect
  server.onNotFound([]() {
    handleCaptivePortalRedirect();
  });
}

void startSecondaryAP() {
  if (apActive) return;

  Serial.println("[AP] Starting Access Point 'Secondary uController'...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMask);
  bool res = WiFi.softAP(AP_SSID, AP_PASS);
  if (res) {
    apActive = true;
    Serial.print("[AP] AP started! IP: ");
    Serial.println(WiFi.softAPIP());

    // Start Captive Portal DNS on port 53 — resolve all domains '*' to AP IP
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println("[AP] Captive Portal DNS Server started on port 53.");

    server.begin();
    Serial.println("[AP] Web server started on port 80.");
  } else {
    Serial.println("[AP] Failed to start SoftAP.");
  }
}

void stopSecondaryAP() {
  if (!apActive) return;
  Serial.println("[AP] Stopping Access Point...");
  dnsServer.stop();
  server.stop();
  WiFi.softAPdisconnect(true);
  apActive = false;
  Serial.println("[AP] Access Point stopped.");
}

bool isSecondaryAPRunning() {
  return apActive;
}

int getSecondaryAPStationNum() {
  return apActive ? WiFi.softAPgetStationNum() : 0;
}

void handleDashboardServer() {
  if (apActive) {
    dnsServer.processNextRequest();
    server.handleClient();
  }
}
