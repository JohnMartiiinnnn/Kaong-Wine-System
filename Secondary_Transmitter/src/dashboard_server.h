#pragma once

#include <Arduino.h>

// Structure to send data (40 bytes with signature and checksum)
typedef struct __attribute__((packed)) {
  uint32_t signature; // 0xDEADBEEF for synchronization
  float pillTemp;
  float pillGravity;
  float room2Temp;
  float room2Pres;
  float phValue;
  float room2LiquidTemp;
  float motorSenseVolts; // Volts on BTS7960 current sense
  uint8_t sensor2Status; // 0: Not Found, 1: BME280, 2: BMP280
  uint8_t adsStatus;     // 0: Not Found, 1: Connected
  uint8_t ds18Status;    // 0: Not Found, 1: Connected
  uint8_t bleStatus;     // 0: Failed, 1: Initialized
  uint8_t pillBattery;   // 0-100%
  int16_t pillRSSI;      // Signal Strength
  uint8_t checksum;      // For data integrity
} struct_message;

static_assert(sizeof(struct_message) == 40, "struct_message size must be 40 bytes");

// Global data references
extern struct_message txData;
extern portMUX_TYPE txDataMux;
extern bool isUartConnected;
extern uint32_t lastUartRxMs;

// Checksum calculation
uint8_t calculateChecksum(const struct_message &msg);

// Server & Captive Portal Lifecycle
void initDashboardServer();
void handleDashboardServer();
void startSecondaryAP();
void stopSecondaryAP();
bool isSecondaryAPRunning();
int getSecondaryAPStationNum();
