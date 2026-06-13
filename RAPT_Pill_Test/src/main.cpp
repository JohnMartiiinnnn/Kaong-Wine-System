#include <Arduino.h>
#include <NimBLEDevice.h>

#define GRAVITY_OFFSET 0.009f

NimBLEScan *pBLEScan;

struct PillData {
  float sg;
  float temp;
  uint8_t battery;
  int16_t rssi;
  bool fresh;
};

static volatile PillData latest = {0, 0, 0, 0, false};
static portMUX_TYPE pillMux = portMUX_INITIALIZER_UNLOCKED;

class RAPTPillCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *device) {
    if (!device->haveManufacturerData()) return;

    std::string data = device->getManufacturerData();
    uint8_t *p = (uint8_t *)data.data();
    int len = data.length();

    if (len < 23 || p[0] != 0x52 || p[1] != 0x41 || p[2] != 0x50 || p[3] != 0x54)
      return;

    uint8_t battery = (len > 23) ? p[23] : p[10];
    uint16_t tempRaw = (p[11] << 8) | p[12];
    float temp = (tempRaw / 128.0f) - 273.15f;
    uint32_t gravRaw = ((uint32_t)p[13] << 24) | ((uint32_t)p[14] << 16) |
                       ((uint32_t)p[15] << 8)  |  (uint32_t)p[16];
    float sg;
    memcpy(&sg, &gravRaw, sizeof(float));
    if (sg > 200.0f) sg /= 1000.0f;
    sg += GRAVITY_OFFSET;

    portENTER_CRITICAL_ISR(&pillMux);
    latest.sg      = sg;
    latest.temp    = temp;
    latest.battery = battery;
    latest.rssi    = device->getRSSI();
    latest.fresh   = true;
    portEXIT_CRITICAL_ISR(&pillMux);
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("\nRAPT Pill BLE Test");
  Serial.println("Scanning for RAPT Pill...");

  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new RAPTPillCallbacks(), false);
  pBLEScan->setActiveScan(false);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setDuplicateFilter(false);
  pBLEScan->start(0, nullptr, false);
}

void loop() {
  static float lastSG = -1.0f;

  PillData d;
  portENTER_CRITICAL(&pillMux);
  d.sg      = latest.sg;
  d.temp    = latest.temp;
  d.battery = latest.battery;
  d.rssi    = latest.rssi;
  d.fresh   = latest.fresh;
  latest.fresh = false;
  portEXIT_CRITICAL(&pillMux);

  if (d.fresh && d.sg != lastSG) {
    lastSG = d.sg;
    Serial.printf("SG: %.4f  Temp: %.2fC  Batt: %d%%  RSSI: %d dBm\n",
                  d.sg, d.temp, d.battery, d.rssi);
  }

  if (!pBLEScan->isScanning())
    pBLEScan->start(0, nullptr, false);

  delay(500);
}
