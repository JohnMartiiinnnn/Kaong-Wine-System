#!/usr/bin/env python3
"""
WineBrew System - Autonomous Wi-Fi Data Logger (Auto-Batch Rotating)
Continuously streams /data from the WineBrew ESP32 controller.
Automatically rotates CSV files on new batch start (transition from IDLE or new SD logFile).
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
    ["# Volume_L                    : Vat / Sap Volume in Liters (HX711 Load Cell)"],
    ["# Ch0_Preheat_L              : Preheat Chamber / Load Cell Volume in Liters"],
    ["# Ch1_Ferm_L                 : Fermentation Chamber Liquid Load in Liters"],
    ["# Ch2_Past_L                 : Pasteurization Chamber Liquid Load in Liters"],
    ["# Transfer1_PreheatToFerm_L   : Transferred Volume from Preheat to Fermentation via Flow Sensor 1 (Liters)"],
    ["# Transfer2_FermToPast_L      : Transferred Volume from Fermentation to Pasteurization via Flow Sensor 2 (Liters)"],
    ["# LocalAmbient_C              : Preheat Chamber Ambient Temperature (BME280, Celsius)"],
    ["# PreheatLiquid_C             : Preheat Chamber Liquid Temperature (DS18B20 Probe 2, Celsius)"],
    ["# PastLiquid_C                : Pasteurization Liquid Temperature (DS18B20 Probe 1, Celsius)"],
    ["# FermAmbient_C               : Fermentation Enclosure Ambient Air Temperature (BME280 Remote, Celsius)"],
    ["# FermLiquid_C                : Fermentation Vessel Liquid Temperature (DS18B20 Remote, Celsius)"],
    ["# pH                          : Liquid Acidity / pH Level (ADS1115 Dual-Slope Calibrated Probe)"],
    ["# Gravity                     : Specific Gravity (RAPT Pill BLE Digital Hydrometer, calibrated to 1.000)"],
    ["# ABV_pct                     : Estimated Alcohol by Volume Percentage ((OG - FG) * 131.25)"],
    ["# TargetTemp_C                : Closed-Loop Active Stage Target Temperature Setpoint (Celsius)"],
    ["# Setpoint_Preheat_Temp_C    : Preheat Chamber Target Temperature Setpoint (Celsius)"],
    ["# Setpoint_Preheat_Cool_C    : Preheat Chamber Cooling Target Setpoint (Celsius)"],
    ["# Setpoint_Ferm_Temp_C       : Fermentation Chamber Target Temperature Setpoint (Celsius)"],
    ["# Setpoint_Past_Temp_C       : Pasteurization Chamber Target Temperature Setpoint (Celsius)"],
    ["# Setpoint_pH                : Target Terminal Fermentation pH Setpoint"],
    ["# Setpoint_Gravity           : Target Terminal Fermentation Specific Gravity Setpoint"],
    ["# Setpoint_Yeast_g           : Target Yeast Pitch Mass Setpoint (Grams)"],
    ["# Heater_pct                  : SSR Heating Element Output Duty Cycle (0 - 100%)"],
    ["# Fan_State                   : Chamber Ventilation Fan (PREHEAT_FAN, FERM_FAN, OFF)"],
    ["# Mixer_Mode                  : Mixing Impeller Automation Mode (OFF, MANUAL, AUTO)"],
    ["# MixerSpeed_pct              : JGB37 Mixing Motor Speed (0 - 100%)"],
    ["# YeastDispensed_g            : Yeast Mass Dispensed into Fermentation Chamber (Grams)"],
    ["# Pill_Battery_pct            : RAPT Pill Battery Level (0 - 100%)"],
    ["# Pill_RSSI_dBm               : RAPT Pill Bluetooth Signal Strength (dBm)"],
    ["# Motor_Sense_V               : Mixing Motor BTS7960 Current Sense Feedback Voltage (Volts)"],
    ["# Active_Log_File             : Primary Controller Active SD Batch File Reference"],
    ["#"]
]

CSV_HEADERS = [
    "Date",
    "Time",
    "Stage",
    "Volume_L",
    "Ch0_Preheat_L",
    "Ch1_Ferm_L",
    "Ch2_Past_L",
    "Transfer1_PreheatToFerm_L",
    "Transfer2_FermToPast_L",
    "LocalAmbient_C",
    "PreheatLiquid_C",
    "PastLiquid_C",
    "FermAmbient_C",
    "FermLiquid_C",
    "pH",
    "Gravity",
    "ABV_pct",
    "TargetTemp_C",
    "",
    "Setpoint_Preheat_Temp_C",
    "Setpoint_Preheat_Cool_C",
    "Setpoint_Ferm_Temp_C",
    "Setpoint_Past_Temp_C",
    "Setpoint_pH",
    "Setpoint_Gravity",
    "Setpoint_Yeast_g",
    "",
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

def write_header_if_needed(csv_path):
    if not os.path.exists(csv_path) or os.path.getsize(csv_path) == 0:
        with open(csv_path, mode="a", newline="", buffering=1) as f:
            writer = csv.writer(f)
            for leg in CSV_LEGEND:
                writer.writerow(leg)
            writer.writerow(CSV_HEADERS)
            f.flush()

def main():
    parser = argparse.ArgumentParser(description="Autonomous WineBrew Wi-Fi CSV Logger")
    parser.add_argument("--ip", default=DEFAULT_IP, help="ESP32 IP address or hostname")
    parser.add_argument("--interval", type=float, default=DEFAULT_INTERVAL, help="Polling interval in seconds")
    parser.add_argument("--outdir", default=os.path.expanduser("~/winebrew-logs"), help="Directory to save CSV logs")
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    print(f"==================================================")
    print(f"WineBrew Auto-Rotating Wi-Fi Logger Started")
    print(f"Target: http://{args.ip}/data")
    print(f"Output Directory: {args.outdir}")
    print(f"Interval: {args.interval}s")
    print(f"==================================================")

    current_batch_sd_file = None
    current_csv_path = None
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
            sd_logfile = data.get("logFile", "")

            # Check if an active batch is running
            is_active_brew = (stage_code >= 0) or (sd_logfile.startswith("/brew_") and stage_code != -1)

            if is_active_brew:
                # If this is a new batch file or we transitioned from IDLE, create a new CSV
                if current_batch_sd_file != sd_logfile or current_csv_path is None:
                    current_batch_sd_file = sd_logfile
                    start_ts = t_now.strftime("%Y%m%d_%H%M%S")
                    current_csv_path = os.path.join(args.outdir, f"brew_wifi_{start_ts}.csv")
                    write_header_if_needed(current_csv_path)
                    print(f"[{t_now.strftime('%H:%M:%S')}] *** NEW BATCH DETECTED *** Created log: {current_csv_path} (SD: {sd_logfile})", flush=True)

                target_file = current_csv_path
            else:
                # In IDLE: close active batch file and log idle heartbeat to idle_telemetry.csv
                if current_csv_path is not None:
                    print(f"[{t_now.strftime('%H:%M:%S')}] *** BATCH FINALIZED *** Returned to IDLE.", flush=True)
                    current_batch_sd_file = None
                    current_csv_path = None

                target_file = os.path.join(args.outdir, "idle_telemetry.csv")
                write_header_if_needed(target_file)

            ch0 = data.get("ch0", data.get("vol", 0.0))
            ch1 = data.get("ch1", 0.0)
            ch2 = data.get("ch2", 0.0)
            xfer1 = data.get("xfer1", data.get("xfer_vol", 0.0) if stage_code == 0 else 0.0)
            xfer2 = data.get("xfer2", data.get("xfer_vol", 0.0) if stage_code == 1 else 0.0)

            row = [
                t_now.strftime("%Y-%m-%d"),
                t_now.strftime("%H:%M:%S"),
                stage_str,
                f"{data.get('vol', 0.0):.2f}",
                f"{float(ch0):.2f}",
                f"{float(ch1):.2f}",
                f"{float(ch2):.2f}",
                f"{float(xfer1):.2f}",
                f"{float(xfer2):.2f}",
                f"{data.get('la', 0.0):.2f}",
                f"{data.get('ll', 0.0):.2f}",
                f"{data.get('lp', 0.0):.2f}",
                f"{data.get('fa', 0.0):.2f}",
                f"{data.get('fl', 0.0):.2f}",
                f"{data.get('ph', 0.0):.2f}",
                f"{data.get('sg', 0.0):.4f}",
                f"{data.get('abv', 0.0):.2f}",
                f"{data.get('targetT', 0.0):.1f}",
                "",
                f"{data.get('tgt_ph', 40.0):.1f}",
                f"{data.get('tgt_cool', 38.0):.1f}",
                f"{data.get('tgt_ferm', 30.0):.1f}",
                f"{data.get('tgt_past', 72.0):.1f}",
                f"{data.get('tgt_ph_val', 4.0):.2f}",
                f"{data.get('tgt_sg_val', 1.000):.4f}",
                f"{data.get('yd_tgt', 5.0):.2f}",
                "",
                data.get("hp", 0),
                fan_str,
                mixer_str,
                data.get("msp", 0),
                f"{data.get('yd', 0.0):.2f}",
                data.get("bat", 0),
                data.get("rssi", 0),
                f"{data.get('mv', 0.0):.3f}",
                sd_logfile
            ]

            with open(target_file, mode="a", newline="", buffering=1) as f:
                writer = csv.writer(f)
                writer.writerow(row)
                f.flush()

            if is_active_brew:
                print(f"[{t_now.strftime('%H:%M:%S')}] [{os.path.basename(target_file)}] Stage: {stage_str:12} | Vol: {data.get('vol',0.0):4.1f}L | Ferm: {data.get('fl',0.0):4.1f}C | pH: {data.get('ph',0.0):4.2f} | SG: {data.get('sg',0.0):.4f}", flush=True)
        else:
            consecutive_errors += 1
            if consecutive_errors % 12 == 1:
                print(f"[{t_now.strftime('%H:%M:%S')}] Waiting for ESP32 at {args.ip}...", flush=True)

        time.sleep(args.interval)

if __name__ == "__main__":
    main()
