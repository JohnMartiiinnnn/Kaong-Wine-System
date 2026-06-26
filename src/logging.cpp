#include "logging.h"

uint8_t calculateChecksum(const struct_message &msg) {
  uint8_t        checksum = 0;
  const uint8_t *ptr      = (const uint8_t *)&msg;
  for (size_t i = 0; i < sizeof(struct_message) - 1; i++)
    checksum ^= ptr[i];
  return checksum;
}

void logDataToSD() {
  if (!sdStatus || !rtcStatus)
    return;
  DateTime now       = rtc.now();
  bool     fileExists = SD.exists(currentLogFile);
  File     dataFile  = SD.open(currentLogFile, FILE_APPEND);
  if (!dataFile)
    return;
  if (!fileExists)
    dataFile.println("Date,Time,LocalTemp,LocalLiquid1,LocalLiquid2,RemoteTemp,RemoteLiquid,Gravity,pH,ABV");
  sprintf(lastLogTime, "%02d:%02d", now.hour(), now.minute());
  dataFile.print(now.year(), DEC);   dataFile.print('/');
  dataFile.print(now.month(), DEC);  dataFile.print('/');
  dataFile.print(now.day(), DEC);    dataFile.print(',');
  dataFile.print(now.hour(), DEC);   dataFile.print(':');
  dataFile.print(now.minute(), DEC); dataFile.print(':');
  dataFile.print(now.second(), DEC); dataFile.print(',');
  dataFile.print(bme1Status   ? bme1.readTemperature()                   : 0.0); dataFile.print(',');
  dataFile.print(liquid2Status ? sharedLiquidSensors.getTempCByIndex(1)  : 0.0); dataFile.print(',');
  dataFile.print(liquid1Status ? sharedLiquidSensors.getTempCByIndex(0)  : 0.0); dataFile.print(',');
  dataFile.print(incomingData.room2Temp); dataFile.print(',');
  dataFile.print(incomingData.ds18Status == 1 ? incomingData.room2LiquidTemp : 0.0); dataFile.print(',');
  dataFile.print(incomingData.pillGravity, 4); dataFile.print(',');
  dataFile.print(incomingData.phValue, 2);     dataFile.print(',');
  float abv = (originalGravity > 0 && incomingData.pillGravity > 0 && incomingData.pillGravity < 10.0)
              ? max(0.0f, (originalGravity - incomingData.pillGravity) * 131.25f) : 0.0f;
  dataFile.print(abv, 2);
  dataFile.println();
  dataFile.close();
}
