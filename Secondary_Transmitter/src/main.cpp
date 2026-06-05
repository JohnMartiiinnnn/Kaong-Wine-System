#include <Adafruit_ADS1X15.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Arduino.h>
#include <DallasTemperature.h>
#include <NimBLEDevice.h>
#include <OneWire.h>
#include <Wire.h>

/*
 * WIRING FOR SECONDARY ESP32:
 * 6-Pin Sensor: VCC->3.3V, GND->GND, SDA->21, SCL->22, CSB->3.3V, SDO->3.3V
 * (Address 0x77) ADS1115: VCC->3.3V, GND->GND, SDA->21, SCL->22 PH4502C: Po ->
 * ADS1115 A0 DS18B20 (Liquid Temp): Data -> Pin 4 (Use 4.7k pullup to 3.3V),
 * VCC->3.3V, GND->GND UART to Main: TX(17) -> Main RX(16), RX(16) -> Main
 * TX(17), GND -> GND
 */

const int ONE_WIRE_BUS = 13;

// Structure to send data (Updated to include framing and checksum)
typedef struct __attribute__((packed)) {
  uint32_t signature; // 0xDEADBEEF for synchronization
  float pillTemp;
  float pillGravity;
  float room2Temp;
  float room2Pres;
  float phValue;
  float room2LiquidTemp;
  uint8_t sensor2Status; // 0: Not Found, 1: BME280, 2: BMP280
  uint8_t adsStatus;     // 0: Not Found, 1: Connected
  uint8_t ds18Status;    // 0: Not Found, 1: Connected
  uint8_t bleStatus;     // 0: Failed, 1: Initialized
  uint8_t pillBattery;   // 0-100%
  int16_t pillRSSI;      // Signal Strength
  uint8_t checksum;      // For data integrity
} struct_message;

struct_message txData;
static portMUX_TYPE txDataMux = portMUX_INITIALIZER_UNLOCKED;

// Simple XOR checksum calculation
uint8_t calculateChecksum(const struct_message &msg) {
  uint8_t checksum = 0;
  const uint8_t *ptr = (const uint8_t *)&msg;
  for (size_t i = 0; i < sizeof(struct_message) - 1; i++) {
    checksum ^= ptr[i];
  }
  return checksum;
}

Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
Adafruit_ADS1115 ads;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

NimBLEScan *pBLEScan;

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    if (advertisedDevice->haveManufacturerData()) {
      std::string strData = advertisedDevice->getManufacturerData();
      uint8_t *payload = (uint8_t *)strData.data();
      int payloadLength = strData.length();

      // Check for RAPT Pill beacon (0x52, 0x41, 0x50, 0x54)
      if (payloadLength >= 23 && payload[0] == 0x52 && payload[1] == 0x41 &&
          payload[2] == 0x50 && payload[3] == 0x54) {

        // Debug: Print raw payload ONLY for RAPT
        Serial.print("RAPT Raw Payload: ");
        for (int i = 0; i < payloadLength; i++) {
          if (payload[i] < 0x10)
            Serial.print("0");
          Serial.print(payload[i], HEX);
          Serial.print(" ");
        }
        Serial.println();

        // --- 1. BATTERY ---
        uint8_t bat = (payloadLength > 23) ? payload[23] : payload[10];

        // --- 2. TEMPERATURE ---
        uint16_t tempRaw = (payload[11] << 8) | payload[12];
        float pillTemp = (tempRaw / 128.0) - 273.15;

        // --- 3. SPECIFIC GRAVITY ---
        // Based on your raw data, 1.296 (0x44A2003A) starts at byte index 13
        uint32_t gravInt = (payload[13] << 24) | (payload[14] << 16) |
                           (payload[15] << 8) | payload[16];
        float sgFloat;
        memcpy(&sgFloat, &gravInt, sizeof(float));
        // For this firmware version, SG is sent as density (1296.0)
        float pillGravity = (sgFloat > 200.0) ? sgFloat / 1000.0 : sgFloat;

        // --- 4. RSSI ---
        int16_t rssi = advertisedDevice->getRSSI();

        // Write decoded fields atomically — callback runs in NimBLE RTOS task
        portENTER_CRITICAL(&txDataMux);
        txData.pillBattery = bat;
        txData.pillTemp = pillTemp;
        txData.pillGravity = pillGravity;
        txData.pillRSSI = rssi;
        portEXIT_CRITICAL(&txDataMux);

        Serial.print("Decoded -> Bat: ");
        Serial.print(txData.pillBattery);
        Serial.print("%, Temp: ");
        Serial.print(txData.pillTemp);
        Serial.print("C, Grav: ");
        Serial.println(txData.pillGravity, 4);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  Wire.begin();

  Serial.println("Initializing components…");

  // 1. Pre-heat sensors (Not present on Secondary)
  Serial.print("Pre-heat. ambient temp. sensor: ");
  Serial.println("not found");
  Serial.print("Pre-heat. liquid temp. sensor: ");
  Serial.println("not found");

  // 2. Ferm. ambient temp. sensor (Address 0x77)
  Serial.print("Ferm. ambient temp. sensor: ");
  if (bme.begin(0x77)) {
    txData.sensor2Status = 1;
    Serial.println("good");
  } else if (bmp.begin(0x77)) {
    txData.sensor2Status = 2;
    Serial.println("good");
  } else {
    txData.sensor2Status = 0;
    Serial.println("not found");
  }

  // 3. Ferm. liquid temp. sensor
  Serial.print("Ferm. liquid temp. sensor: ");
  sensors.begin();
  sensors.setWaitForConversion(
      false); // Non-blocking; loop() requests conversion, reads result next
              // iteration
  if (sensors.getDeviceCount() > 0) {
    txData.ds18Status = 1;
    sensors.requestTemperatures(); // Prime first conversion
    Serial.println("good");
  } else {
    txData.ds18Status = 0;
    Serial.println("not found");
  }

  // 4. pH sensor
  Serial.print("pH sensor: ");
  if (ads.begin()) {
    txData.adsStatus = 1;
    Serial.println("good");
  } else {
    txData.adsStatus = 0;
    Serial.println("not found");
  }

  // 5. BLE Setup
  Serial.print("BLE decoder: ");
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(),
                                         false);
  pBLEScan->setActiveScan(false);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setDuplicateFilter(false);

  if (pBLEScan->start(0, nullptr, false)) {
    txData.bleStatus = 1;
    Serial.println("initialized");
  } else {
    txData.bleStatus = 0;
    Serial.println("failed to initialize");
  }

  // Send initial status immediately
  txData.signature = 0xDEADBEEF;
  txData.checksum = calculateChecksum(txData);
  Serial2.write((uint8_t *)&txData, sizeof(txData));
}

void loop() {
  // 1. Dynamic Sensor Status Check (In case they weren't ready at boot)
  if (txData.sensor2Status == 0) {
    if (bme.begin(0x77))
      txData.sensor2Status = 1;
    else if (bmp.begin(0x77))
      txData.sensor2Status = 2;
  }

  if (txData.ds18Status == 0) {
    sensors.begin();
    if (sensors.getDeviceCount() > 0)
      txData.ds18Status = 1;
  }

  if (txData.adsStatus == 0) {
    if (ads.begin())
      txData.adsStatus = 1;
  }

  // 2. Update ambient sensor data
  if (txData.sensor2Status == 1) {
    float t = bme.readTemperature();
    if (!isnan(t)) {
      txData.room2Temp = t;
      txData.room2Pres = bme.readPressure() / 100.0F;
    } else {
      txData.sensor2Status = 0; // Mark as lost if reading fails
    }
  } else if (txData.sensor2Status == 2) {
    float t = bmp.readTemperature();
    if (!isnan(t)) {
      txData.room2Temp = t;
      txData.room2Pres = bmp.readPressure() / 100.0F;
    } else {
      txData.sensor2Status = 0;
    }
  }

  // 3. Update Liquid Temp
  // Read result of the PREVIOUS requestTemperatures(), then request the next
  // one. With setWaitForConversion(false), the 1000ms delay() below covers the
  // 750ms conversion.
  if (txData.ds18Status == 1) {
    float t = sensors.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C) {
      txData.room2LiquidTemp = t;
    } else {
      txData.ds18Status = 0; // Sensor lost
    }
    sensors.requestTemperatures(); // Start next conversion (completes during
                                   // delay(1000))
  } else {
    txData.room2LiquidTemp = -127.0;
  }

  // 4. Update pH data
  if (txData.adsStatus == 1) {
    int16_t results = ads.readADC_SingleEnded(0);
    float voltage = ads.computeVolts(results);

    // Temperature Compensation (Nernst-based)
    float tempC = (txData.ds18Status == 1) ? txData.room2LiquidTemp : 25.0;
    float slope_V_pH = 0.17126 * (tempC + 273.15) / 298.15;
    txData.phValue = 7.0 - (voltage - 2.555) / slope_V_pH;
  }

  // 5. UART Transmission — copy under lock so BLE callback can't tear a field
  // mid-send
  portENTER_CRITICAL(&txDataMux);
  struct_message snapshot = txData;
  portEXIT_CRITICAL(&txDataMux);
  snapshot.signature = 0xDEADBEEF;
  snapshot.checksum = calculateChecksum(snapshot);
  Serial2.write((uint8_t *)&snapshot, sizeof(snapshot));

  // BLE scanning happens in the background via callbacks
  if (!pBLEScan->isScanning()) {
    pBLEScan->start(0, nullptr, false);
  }
  delay(1000); // Send data every second
}
