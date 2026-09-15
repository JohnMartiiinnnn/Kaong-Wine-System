#!/usr/bin/env python3
"""
WineBrew System - Autonomous Wi-Fi Data Logger
Continuously streams /data from the WineBrew ESP32 controller and writes all parameters into a batch CSV file.
Uses standard library only (urllib.request) - no external dependencies required.
"""

import os
import sys
import time
import json
import csv
import argparse
import urllib.request
from datetime import datetime

DEFAULT_IP = "192.168.1.137"
DEFAULT_INTERVAL = 5.0 # seconds

CSV_LEGEND = [
    ["# WineBrew Automated Wine Brewing System - Experimental Telemetry Log"],
    ["# System Architecture: Primary ESP32 (Master) + Secondary ESP32 (Sensor Node) + RAPT Pill Hydrometer"],
    ["#"],
    ["# PARAMETER LEGEND & SENSOR SPECIFICATIONS:"],
    ["# Date               : Sample Date (YYYY-MM-DD)"],
    ["# Time               : Sample Timestamp with Second Precision (HH:MM:SS)"],
    ["# Stage              : Active Stage (PREHEAT, FERMENTATION, PASTEURIZATION, IDLE)"],
    ["# Volume_L           : Vat / Sap Volume in Liters (HX711 Load Cell)"],
    ["# LocalAmbient_C     : Preheat Chamber Ambient Temperature (BME280, Celsius)"],
    ["# PreheatLiquid_C    : Preheat Chamber Liquid Temperature (DS18B20 Probe 2, Celsius)"],
    ["# PastLiquid_C       : Pasteurization Liquid Temperature (DS18B20 Probe 1, Celsius)"],
    ["# FermAmbient_C      : Fermentation Enclosure Ambient Air Temperature (BME280 Remote, Celsius)"],
    ["# FermLiquid_C       : Fermentation Vessel Liquid Temperature (DS18B20 Remote, Celsius)"],
    ["# pH                 : Liquid Acidity / pH Level (ADS1115 Dual-Slope Calibrated Probe)"],
    ["# Gravity            : Specific Gravity (RAPT Pill BLE Digital Hydrometer, calibrated to 1.000)"],
    ["# ABV_pct            : Estimated Alcohol by Volume Percentage ((OG - FG) * 131.25)"],
    ["# TargetTemp_C       : Closed-Loop Stage Target Temperature Setpoint (Celsius)"],
    ["# Heater_pct         : SSR Heating Element Output Duty Cycle (0 - 100%)"],
    ["# Fan_State          : Chamber Ventilation Fan (PREHEAT_FAN, FERM_FAN, OFF)"],
    ["# Mixer_Mode         : Mixing Impeller Automation Mode (OFF, MANUAL, AUTO)"],
    ["# MixerSpeed_pct     : JGB37 Mixing Motor Speed (0 - 100%)"],
    ["# YeastDispensed_g   : Yeast Mass Dispensed into Fermentation Chamber (Grams)"],
    ["# Pill_Battery_pct   : RAPT Pill Battery Level (0 - 100%)"],
    ["# Pill_RSSI_dBm      : RAPT Pill Bluetooth Signal Strength (dBm)"],
    ["# Motor_Sense_V      : Mixing Motor BTS7960 Current Sense Feedback Voltage (Volts)"],
    ["# Active_Log_File    : Primary Controller Active SD Batch File Reference"],
    ["#"]
]

CSV_HEADERS = [
    "Date",
    "Time",
    "Stage",
    "Volume_L",
    "LocalAmbient_C",
    "PreheatLiquid_C",
    "PastLiquid_C",
    "FermAmbient_C",
    "FermLiquid_C",
    "pH",
    "Gravity",
    "ABV_pct",
    "TargetTemp_C",
    "Heater_pct",
    "Fan_State",
    "Mixer_Mode",
    "MixerSpeed_pct",
    "YeastDispensed_g",
    "Pill_Battery_pct",
    "Pill_RSSI_dBm",
    "Motor_Sense_V",
    "Active_Log_File"
]

STAGE_NAMES = {
    -1: "IDLE",
    0: "PREHEAT",
    1: "FERMENTATION",
    2: "PASTEURIZATION"
}

FAN_NAMES = {
    0: "OFF",
    1: "PREHEAT_FAN",
    2: "FERM_FAN"
}

MIXER_MODES = {
    0: "OFF",
    1: "MANUAL",
    2: "AUTO"
}

def get_live_data(ip):
    url = f"http://{ip}/data"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "WineBrewLogger/1.0"})
        with urllib.request.urlopen(req, timeout=3.0) as resp:
            if resp.status == 200:
                raw = resp.read().decode("utf-8")
                return json.loads(raw)
    except Exception:
        return None
    return None

def main():
    parser = argparse.ArgumentParser(description="Autonomous WineBrew Wi-Fi CSV Logger")
    parser.add_argument("--ip", default=DEFAULT_IP, help="ESP32 IP address or hostname")
    parser.add_argument("--interval", type=float, default=DEFAULT_INTERVAL, help="Polling interval in seconds")
    parser.add_argument("--outdir", default="logs", help="Directory to save CSV logs")
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)
    start_ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = os.path.join(args.outdir, f"brew_wifi_{start_ts}.csv")

    print(f"==================================================")
    print(f"WineBrew Autonomous Wi-Fi Logger Started")
    print(f"Target: http://{args.ip}/data")
    print(f"Output File: {csv_path}")
    print(f"Interval: {args.interval}s")
    print(f"==================================================")

    file_exists = os.path.exists(csv_path)
    with open(csv_path, mode="a", newline="", buffering=1) as f:
        writer = csv.writer(f)
        if not file_exists:
            for leg in CSV_LEGEND:
                writer.writerow(leg)
            writer.writerow(CSV_HEADERS)
            f.flush()

        consecutive_errors = 0
        while True:
            t_now = datetime.now()
            data = get_live_data(args.ip)

            if data:
                consecutive_errors = 0
                stage_code = data.get("stage", -1)
                stage_str = STAGE_NAMES.get(stage_code, str(stage_code))
                fan_code = data.get("fan", 0)
                fan_str = FAN_NAMES.get(fan_code, str(fan_code))
                mixer_mode = data.get("mm", 0)
                mixer_str = MIXER_MODES.get(mixer_mode, str(mixer_mode))

                row = [
                    t_now.strftime("%Y-%m-%d"),
                    t_now.strftime("%H:%M:%S"),
                    stage_str,
                    f"{data.get('vol', 0.0):.2f}",
                    f"{data.get('la', 0.0):.2f}",
                    f"{data.get('ll', 0.0):.2f}",
                    f"{data.get('lp', 0.0):.2f}",
                    f"{data.get('fa', 0.0):.2f}",
                    f"{data.get('fl', 0.0):.2f}",
                    f"{data.get('ph', 0.0):.2f}",
                    f"{data.get('sg', 0.0):.4f}",
                    f"{data.get('abv', 0.0):.2f}",
                    f"{data.get('targetT', 0.0):.1f}",
                    data.get("hp", 0),
                    fan_str,
                    mixer_str,
                    data.get("msp", 0),
                    f"{data.get('yd', 0.0):.2f}",
                    data.get("bat", 0),
                    data.get("rssi", 0),
                    f"{data.get('mv', 0.0):.3f}",
                    data.get("logFile", "")
                ]

                writer.writerow(row)
                f.flush()

                print(f"[{t_now.strftime('%H:%M:%S')}] Stage: {stage_str:12} | Vol: {data.get('vol',0.0):4.1f}L | Liquid: {data.get('ll',0.0):4.1f}C | Ferm: {data.get('fl',0.0):4.1f}C | pH: {data.get('ph',0.0):4.2f} | SG: {data.get('sg',0.0):.4f} | Yeast: {data.get('yd',0.0):.1f}g", flush=True)
            else:
                consecutive_errors += 1
                if consecutive_errors % 5 == 1:
                    print(f"[{t_now.strftime('%H:%M:%S')}] Waiting for ESP32 at {args.ip}...", flush=True)

            time.sleep(args.interval)

if __name__ == "__main__":
    main()
