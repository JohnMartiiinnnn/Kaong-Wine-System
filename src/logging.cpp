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
  DateTime now = rtc.now();

  // If currentLogFile is default and a batch is active, set a batch-specific log filename
  if (currentLogFile == "/data_log.csv" && activeBrewStage >= 0) {
    char batchName[32];
    sprintf(batchName, "/brew_%04d%02d%02d_%02d%02d.csv", now.year(), now.month(), now.day(), now.hour(), now.minute());
    currentLogFile = String(batchName);
    saveBrewStateToNVS();
  }

  bool fileExists = SD.exists(currentLogFile);
  File dataFile  = SD.open(currentLogFile, FILE_APPEND);
  if (!dataFile)
    return;
  if (!fileExists) {
    dataFile.println("# WineBrew Automated Wine Brewing System - Experimental Telemetry Log");
    dataFile.println("# System Architecture: Primary Controller (Master) + Secondary Node + RAPT Pill Hydrometer");
    dataFile.println("# Legend: Date,Time,Stage,Volume_L,LocalAmbient_C,PreheatLiquid_C,PastLiquid_C,FermAmbient_C,FermLiquid_C,pH,Gravity,ABV_pct,Heater_pct,Fan_State,MixerSpeed_pct,YeastDispensed_g");
    dataFile.println("Date,Time,Stage,Volume_L,LocalAmbient_C,PreheatLiquid_C,PastLiquid_C,FermAmbient_C,FermLiquid_C,pH,Gravity,ABV_pct,Heater_pct,Fan_State,MixerSpeed_pct,YeastDispensed_g");
  }

  int h12 = now.hour() % 12;
  if (h12 == 0) h12 = 12;
  const char* ampm = (now.hour() >= 12) ? "PM" : "AM";
  sprintf(lastLogTime, "%d:%02d%s", h12, now.minute(), ampm);

  const char* stageStr = (activeBrewStage == 0) ? "PREHEAT" : (activeBrewStage == 1 ? "FERMENTATION" : (activeBrewStage == 2 ? "PASTEURIZATION" : "IDLE"));
  const char* fanStr = isFanOn ? "PREHEAT_FAN" : (isFermFanOn ? "FERM_FAN" : "OFF");

  char lineBuf[256];
  float curPreheat = liquid2Status ? sharedLiquidSensors.getTempCByIndex(1) : 0.0f;
  float curPast = liquid1Status ? sharedLiquidSensors.getTempCByIndex(0) : 0.0f;
  float curFermLiq = (incomingData.ds18Status == 1) ? incomingData.room2LiquidTemp : 0.0f;
  float ambLocal = bme1Status ? bme1.readTemperature() : 0.0f;
  float abv = (originalGravity > 0.0f && incomingData.pillGravity > 0.0f && incomingData.pillGravity < 10.0f)
              ? max(0.0f, (originalGravity - incomingData.pillGravity) * 131.25f) : 0.0f;

  sprintf(lineBuf, "%04d/%02d/%02d,%02d:%02d:%02d,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%d,%s,%d,%.2f",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second(),
          stageStr,
          currentWeight,
          ambLocal,
          curPreheat,
          curPast,
          incomingData.room2Temp,
          curFermLiq,
          incomingData.phValue,
          incomingData.pillGravity,
          abv,
          currentHeatingPercent,
          fanStr,
          mixerSpeedPercent,
          actualYeastDispensedGrams);

  dataFile.println(lineBuf);
  dataFile.close();
}
