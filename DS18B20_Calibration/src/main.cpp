#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 26
#define RX2_PIN 16
#define TX2_PIN 17

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// UART Telemetry Package (from Secondary)
typedef struct __attribute__((packed)) {
  uint32_t signature;
  float pillTemp;
  float pillGravity;
  float room2Temp;
  float room2Pres;
  float phValue;
  float room2LiquidTemp; // Fermentation liquid temp
  uint8_t sensor2Status;
  uint8_t adsStatus;
  uint8_t ds18Status;
  uint8_t bleStatus;
  uint8_t pillBattery;
  int16_t pillRSSI;
  uint8_t checksum;
} struct_message;

struct_message rxData;
bool secondaryActive = false;
uint32_t lastUARTPacket = 0;

uint8_t calculateChecksum(const struct_message &msg) {
  uint8_t checksum = 0;
  const uint8_t *ptr = (const uint8_t *)&msg;
  for (size_t i = 0; i < sizeof(struct_message) - 1; i++) {
    checksum ^= ptr[i];
  }
  return checksum;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  delay(1000);
  Serial.println("\n=============================================");
  Serial.println("  STANDALONE 3-WAY DS18B20 TEMP CALIBRATION");
  Serial.println("=============================================");
  Serial.println("  Put all 3 probes in the same reference water bath.");
  Serial.println("  Compare the readings to calculate your offsets.");
  Serial.println("  Offset = Reference_Temp - Raw_Temp\n");

  sensors.begin();
  int count = sensors.getDeviceCount();
  Serial.printf("Local DS18B20 Probes Found on Pin %d: %d\n", ONE_WIRE_BUS, count);
}

void loop() {
  // 1. Read UART packets from Secondary (to get Fermentation temp)
  while (Serial2.available() > 0) {
    if (Serial2.peek() != 0xEF) {
      Serial2.read();
      continue;
    }
    if (Serial2.available() < (int)sizeof(struct_message)) {
      break;
    }
    struct_message t;
    Serial2.readBytes((uint8_t *)&t, sizeof(t));
    if (t.signature == 0xDEADBEEF && calculateChecksum(t) == t.checksum) {
      rxData = t;
      secondaryActive = true;
      lastUARTPacket = millis();
    }
  }

  if (secondaryActive && (millis() - lastUARTPacket > 3000)) {
    secondaryActive = false;
  }

  // 2. Read local temperatures and print all three
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    sensors.requestTemperatures();
    
    float pastRaw = sensors.getTempCByIndex(0);
    float preheatRaw = sensors.getTempCByIndex(1);
    float fermRaw = (secondaryActive && rxData.ds18Status == 1) ? rxData.room2LiquidTemp : -127.0f;

    Serial.println("--- Current Live Readings (Raw) ---");
    if (pastRaw != DEVICE_DISCONNECTED_C) {
      Serial.printf("  1. Pasteurization (Local Index 0): %.2f C\n", pastRaw);
    } else {
      Serial.println("  1. Pasteurization (Local Index 0): [DISCONNECTED]");
    }

    if (preheatRaw != DEVICE_DISCONNECTED_C) {
      Serial.printf("  2. Pre-heat (Local Index 1)      : %.2f C\n", preheatRaw);
    } else {
      Serial.println("  2. Pre-heat (Local Index 1)      : [DISCONNECTED]");
    }

    if (fermRaw > -100.0f) {
      Serial.printf("  3. Fermentation (Remote UART)     : %.2f C\n", fermRaw);
    } else {
      Serial.println("  3. Fermentation (Remote UART)     : [DISCONNECTED / NO UART DATA]");
    }
    Serial.println("-----------------------------------");
  }
  delay(10);
}
