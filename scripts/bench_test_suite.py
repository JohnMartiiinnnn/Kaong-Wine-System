#!/usr/bin/env python3
"""
WineBrew Automated Bench Test Runner & Chapter 4 CSV Table Generator
--------------------------------------------------------------------
Drives the WineBrew Primary ESP32 over HTTP (/data, /test/*, /api/*),
captures ground-truth sensor & millisecond-precision timing telemetry,
prompts only for physical measurements that have no onboard sensor
(0.1 g yeast scale, 250 mL residual cylinder, external clamp meter,
and visual drip check), and writes individual-trial CSV deliverables.

Zero hardware changes required. Uses Python standard library only.
"""

import argparse
import csv
import glob
import json
import os
import statistics
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime


DEFAULT_ESP32_IP = "192.168.1.137"
DEFAULT_OUT_DIR = os.path.expanduser("~/thesis/results/bench_runs")
DEFAULT_LOG_DIR = os.path.expanduser("~/winebrew-logs")


def http_get_json(ip: str, path: str, timeout: float = 8.0) -> tuple[dict, float]:
    """Perform GET request and return (parsed_json, rtt_ms)."""
    url = f"http://{ip}{path}"
    req = urllib.request.Request(url, headers={"User-Agent": "WineBrew-BenchSuite/1.0"})
    t0 = time.perf_counter()
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        raw = resp.read().decode("utf-8", errors="replace")
    rtt_ms = (time.perf_counter() - t0) * 1000.0
    return json.loads(raw), round(rtt_ms, 2)


def ensure_out_dir(out_dir: str) -> str:
    os.makedirs(out_dir, exist_ok=True)
    return out_dir


def append_csv_row(filepath: str, headers: list[str], row: dict) -> None:
    exists = os.path.exists(filepath) and os.path.getsize(filepath) > 0
    with open(filepath, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=headers)
        if not exists:
            writer.writeheader()
        writer.writerow(row)


def ask_float(prompt: str, default: float | None = None) -> float:
    while True:
        raw = input(prompt).strip()
        if not raw and default is not None:
            return default
        try:
            return float(raw)
        except ValueError:
            print("  Invalid number, try again.")


def ask_yes_no(prompt: str, default_yes: bool = True) -> bool:
    raw = input(prompt).strip().lower()
    if not raw:
        return default_yes
    return raw in ("y", "yes", "1", "true", "pass", "ok")


# ==============================================================================
# TABLE 29 (OFFLINE + OPTIONAL LIVE RTT): CONTINUOUS DATA LOGGING INTEGRITY
# ==============================================================================
def run_offline_table29(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    csv_path = args.csv
    if not csv_path:
        bch1 = os.path.join(DEFAULT_LOG_DIR, "brew_wifi_20261021_192709.csv")
        csv_path = bch1 if os.path.exists(bch1) else sorted(glob.glob(os.path.join(DEFAULT_LOG_DIR, "brew_wifi_*.csv")))[-1]

    print(f"[Table 29] Parsing telemetry log: {csv_path}")
    timestamps = []
    stages = []
    with open(csv_path, encoding="utf-8", errors="replace") as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#") or s.startswith("Date,"):
                continue
            parts = s.split(",")
            if len(parts) < 3:
                continue
            dt_str = f"{parts[0]} {parts[1]}"
            for fmt in ("%Y-%m-%d %H:%M:%S", "%Y/%m/%d %H:%M:%S"):
                try:
                    timestamps.append(datetime.strptime(dt_str, fmt))
                    stages.append(parts[2])
                    break
                except ValueError:
                    continue

    if len(timestamps) < 10:
        print("Not enough valid rows in CSV.", file=sys.stderr)
        sys.exit(1)

    deltas = [(timestamps[i] - timestamps[i - 1]).total_seconds() for i in range(1, len(timestamps)) if timestamps[i] >= timestamps[i - 1]]
    median_dt = statistics.median(deltas) if deltas else 5.0
    total_sec = (timestamps[-1] - timestamps[0]).total_seconds()
    total_hrs = total_sec / 3600.0
    expected_pkts = max(1, int(total_sec / max(1.0, median_dt)))
    gap_threshold = max(15.0, median_dt * 3.0)
    gaps = [d for d in deltas if d > gap_threshold]

    # Measure live HTTP /data RTT latency if ESP32 reachable
    rtt_samples = []
    if args.ping_live:
        print(f"Probing live /data RTT latency x10 on {args.ip}...")
        for _ in range(10):
            try:
                _, rtt = http_get_json(args.ip, "/data", timeout=3.0)
                rtt_samples.append(rtt)
                time.sleep(0.25)
            except Exception:
                break

    avg_rtt = round(statistics.mean(rtt_samples), 2) if rtt_samples else "NOT_CONNECTED"

    # Split log cleanly into 5 equal time-window intervals for Chapter 4 Table 29
    out_csv = os.path.join(out_dir, "table29_continuity.csv")
    if os.path.exists(out_csv):
        os.remove(out_csv)

    headers = [
        "Interval", "Window_Start", "Window_End", "Duration_Hours",
        "Packets_Logged", "Median_Period_s", "Gaps_Over_15s",
        "Max_Gap_s", "Uptime_Pct", "Live_HTTP_RTT_ms", "Status"
    ]
    n = len(timestamps)
    chunk = n // 5
    for idx in range(5):
        start_i = idx * chunk
        end_i = n if idx == 4 else (idx + 1) * chunk
        sub_ts = timestamps[start_i:end_i]
        sub_dt = [(sub_ts[j] - sub_ts[j - 1]).total_seconds() for j in range(1, len(sub_ts)) if sub_ts[j] >= sub_ts[j - 1]]
        win_sec = max(1.0, (sub_ts[-1] - sub_ts[0]).total_seconds())
        sub_gaps = [d for d in sub_dt if d > gap_threshold]
        dropped_sec = sum(max(0.0, d - median_dt) for d in sub_gaps)
        uptime_pct = max(0.0, min(100.0, ((win_sec - dropped_sec) / win_sec) * 100.0))
        row = {
            "Interval": idx + 1,
            "Window_Start": sub_ts[0].strftime("%Y-%m-%d %H:%M:%S"),
            "Window_End": sub_ts[-1].strftime("%Y-%m-%d %H:%M:%S"),
            "Duration_Hours": round(win_sec / 3600.0, 2),
            "Packets_Logged": len(sub_ts),
            "Median_Period_s": round(statistics.median(sub_dt), 2) if sub_dt else median_dt,
            "Gaps_Over_15s": len(sub_gaps),
            "Max_Gap_s": round(max(sub_gaps), 1) if sub_gaps else round(max(sub_dt), 1),
            "Uptime_Pct": round(uptime_pct, 2),
            "Live_HTTP_RTT_ms": avg_rtt,
            "Status": "Passed" if uptime_pct >= 95.0 else "Check Gaps"
        }
        append_csv_row(out_csv, headers, row)
        print(f"  Interval {idx + 1}: {row['Duration_Hours']}h | {row['Packets_Logged']} pkts | gaps>15s={row['Gaps_Over_15s']} | uptime={row['Uptime_Pct']}%")

    print(f"\nOverall: {total_hrs:.2f} hrs, {len(timestamps)} packets, {len(gaps)} gaps > {gap_threshold:.0f}s (expected ~{expected_pkts} pkts).")
    print(f"Saved -> {out_csv}")


# ==============================================================================
# TABLE 16: DRY-FIRE SAFETY LOCKOUT (5 TRIALS)
# ==============================================================================
def run_table16(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    out_csv = os.path.join(out_dir, "table16_dry_fire.csv")
    headers = [
        "Trial", "Timestamp", "HX711_Vol_L", "Min_Safe_L", "SSR_Preheat_State",
        "Heater_Pct", "Dry_Alarm", "LCD_Warning_Confirmed", "Lockout_Latency_ms",
        "HTTP_RTT_ms", "Status"
    ]
    print(f"[Table 16] Dry-Fire Safety Lockout ({args.trials} trials) on {args.ip}")
    print("Ensure Tank 1 is EMPTY or < 5.0 L before pressing Enter.")
    for t in range(1, args.trials + 1):
        input(f"\nTrial {t}/{args.trials} - Press Enter to query /api/test/dry_fire...")
        data, rtt = http_get_json(args.ip, "/api/test/dry_fire")
        print(f"  ESP32: vol={data['vol_l']} L | SSR_PRE={data['ssr_preheat']} | Heater={data['heater_pct']}% | dry_alarm={data['dry_alarm']} | fw_ms={data['latency_ms']}")
        lcd_ok = ask_yes_no("  LCD shows 'ERROR: MIN 5L REQUIRED' / Dry Alarm lockout? [Y/n]: ", True)
        passed = bool(data.get("pass")) and lcd_ok
        append_csv_row(out_csv, headers, {
            "Trial": t,
            "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "HX711_Vol_L": data["vol_l"],
            "Min_Safe_L": data["min_safe_l"],
            "SSR_Preheat_State": data["ssr_preheat"],
            "Heater_Pct": data["heater_pct"],
            "Dry_Alarm": data["dry_alarm"],
            "LCD_Warning_Confirmed": "Yes" if lcd_ok else "No",
            "Lockout_Latency_ms": data["latency_ms"],
            "HTTP_RTT_ms": rtt,
            "Status": "Passed" if passed else "Failed"
        })
    print(f"Saved -> {out_csv}")


# ==============================================================================
# TABLE 20: PASTEURIZATION TRIGGER INTERLOCK (10 TRIALS)
# ==============================================================================
def run_table20(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    out_csv = os.path.join(out_dir, "table20_past_interlock.csv")
    headers = [
        "Trial", "Trigger_Condition", "Bottom_Heater_Relay",
        "Target_Temp_C", "Latency_ms", "Activation_Status"
    ]
    data, rtt = http_get_json(args.ip, "/test/pasteurization", timeout=15.0)
    for row in data.get("trials", []):
        append_csv_row(out_csv, headers, {
            "Trial": row["trial"],
            "Trigger_Condition": row["trigger_condition"],
            "Bottom_Heater_Relay": row["bottom_heater_relay"],
            "Target_Temp_C": row["target_temp"],
            "Latency_ms": row["latency_ms"],
            "Activation_Status": row["activation_status"]
        })
        print(f"  Trial {row['trial']}: {row['bottom_heater_relay']} | {row['target_temp']} C | {row['latency_ms']} ms | {row['activation_status']}")
    print(f"HTTP RTT={rtt} ms | Saved -> {out_csv}")


# ==============================================================================
# TABLE 22 (AND TABLE 19): YEAST DISPENSING + AGITATION (5 TRIALS)
# ==============================================================================
def run_table22(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    out_csv = os.path.join(out_dir, "table22_yeast_dispensing.csv")
    headers = [
        "Trial", "Timestamp", "Target_g", "Reported_Dispensed_g",
        "Measured_Scale_g", "Abs_Error_g", "Mixer_Speed_pct",
        "Mixer_Duration_s", "Motor_Sense_V", "Dispatch_Latency_ms",
        "Impeller_Visual_OK", "Status"
    ]
    print(f"[Table 19/22] Yeast Dispensing & Agitation ({args.trials} trials, target={args.grams} g)")
    for t in range(1, args.trials + 1):
        input(f"\nTrial {t}/{args.trials} - Tare 0.1g scale under yeast chute, press Enter to dispense {args.grams} g + mix {args.mix_sec}s...")
        qs = urllib.parse.urlencode({"grams": args.grams, "mix_sec": args.mix_sec, "mix_spd": args.mix_spd})
        res, rtt = http_get_json(args.ip, f"/api/test/yeast?{qs}")
        est_s = max(2.0, res["est_duration_ms"] / 1000.0)
        wait_s = max(est_s, args.mix_sec) + 2.0
        print(f"  Dispatched in {res['dispatch_ms']} ms (RTT={rtt} ms). Waiting {wait_s:.1f}s for dispense + mix...")

        max_mv = res.get("motor_sense_v", 0.0)
        last_d = {}
        end_t = time.time() + wait_s
        while time.time() < end_t:
            try:
                last_d, _ = http_get_json(args.ip, "/data", timeout=3.0)
                max_mv = max(max_mv, float(last_d.get("mv", 0.0)))
            except Exception:
                pass
            time.sleep(1.5)

        measured_g = ask_float(f"  Enter yeast mass weighed on 0.1g scale (g) [target {args.grams}]: ")
        impeller_ok = ask_yes_no("  Did impeller spin smoothly for agitation? [Y/n]: ", True)
        err_g = round(abs(measured_g - args.grams), 2)
        passed = err_g <= max(0.5, args.grams * 0.15) and impeller_ok

        append_csv_row(out_csv, headers, {
            "Trial": t,
            "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "Target_g": args.grams,
            "Reported_Dispensed_g": last_d.get("yd_act", args.grams),
            "Measured_Scale_g": round(measured_g, 2),
            "Abs_Error_g": err_g,
            "Mixer_Speed_pct": args.mix_spd,
            "Mixer_Duration_s": args.mix_sec,
            "Motor_Sense_V": round(max_mv, 3),
            "Dispatch_Latency_ms": res["dispatch_ms"],
            "Impeller_Visual_OK": "Yes" if impeller_ok else "No",
            "Status": "Passed" if passed else "Check Calibration"
        })
    print(f"Saved -> {out_csv}")


# ==============================================================================
# TABLE 23: pH-TRIGGERED PASTEURIZATION AUTO-CUTOFF (5 LIVE PROBE TRIALS)
# ==============================================================================
def run_table23(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    out_csv = os.path.join(out_dir, "table23_ph_cutoff.csv")
    headers = [
        "Batch_Run", "Trial", "Timestamp", "Live_Probe_pH",
        "Cutoff_Threshold_pH", "Ferm_State", "Past_Heater",
        "Latency_ms", "HTTP_RTT_ms", "Status"
    ]
    print("[Table 23] Live pH-Triggered Auto-Cutoff Test (5 cups / probe immersions)")
    for t in range(1, args.trials + 1):
        input(f"\nBatch Run {t}/{args.trials} - Submerge pH probe in < 3.05 pH acid/vinegar cup, wait for reading to settle, press Enter...")
        live, _ = http_get_json(args.ip, "/data")
        print(f"  Current live ADS1115 pH = {live.get('ph')} (ads={live.get('ads')})")
        res, rtt = http_get_json(args.ip, "/test/ph_cutoff?live=1")
        row0 = res["trials"][0]
        print(f"  Triggered: in_line_ph={row0['in_line_ph']} | cutoff={row0['cutoff_ph']} | latency={row0['latency_ms']} ms | {row0['ferm_state']}")
        append_csv_row(out_csv, headers, {
            "Batch_Run": t,
            "Trial": row0["trial"],
            "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "Live_Probe_pH": row0["in_line_ph"],
            "Cutoff_Threshold_pH": row0["cutoff_ph"],
            "Ferm_State": row0["ferm_state"],
            "Past_Heater": row0["past_heater"],
            "Latency_ms": row0["latency_ms"],
            "HTTP_RTT_ms": rtt,
            "Status": "Passed" if row0.get("pass") else "Failed"
        })
    print(f"Saved -> {out_csv}")


# ==============================================================================
# TABLE 25: STAGE TRANSITION DATA LOGGING INTEGRITY (5 TRIALS)
# ==============================================================================
def run_table25(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    out_csv = os.path.join(out_dir, "table25_stage_logging.csv")
    headers = [
        "Trial", "Timestamp", "Stage_Path", "ESP32_Transition_ms",
        "SD_Write_Latency_ms", "WiFi_HTTP_RTT_ms", "Stage_Seq_Before",
        "Stage_Seq_After", "Reported_Stage", "Status"
    ]
    print(f"[Table 25] Stage Transition Data Logging Integrity ({args.trials} trials)")
    for t in range(1, args.trials + 1):
        http_get_json(args.ip, "/api/brew/start_water?skip_heat=1")
        time.sleep(0.5)
        before, _ = http_get_json(args.ip, "/data")
        seq0 = before.get("stage_seq", 0)

        res, rtt = http_get_json(args.ip, "/api/stage/advance")
        time.sleep(0.4)
        after, _ = http_get_json(args.ip, "/data")
        seq1 = after.get("stage_seq", 0)

        passed = (seq1 > seq0) and (res.get("status") == "ok")
        row = {
            "Trial": t,
            "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "Stage_Path": f"Stage {res.get('previous_stage')} -> Stage {res.get('active_stage')} (xfer={res.get('transferring')})",
            "ESP32_Transition_ms": after.get("trans_ms", 1),
            "SD_Write_Latency_ms": after.get("sd_ms", 0),
            "WiFi_HTTP_RTT_ms": rtt,
            "Stage_Seq_Before": seq0,
            "Stage_Seq_After": seq1,
            "Reported_Stage": after.get("stage"),
            "Status": "Passed" if passed else "Failed"
        }
        append_csv_row(out_csv, headers, row)
        print(f"  Trial {t}: seq {seq0}->{seq1} | fw_trans={row['ESP32_Transition_ms']} ms | sd={row['SD_Write_Latency_ms']} ms | rtt={rtt} ms")
        http_get_json(args.ip, "/api/brew/start_water?stop=1")
        time.sleep(0.4)
    print(f"Saved -> {out_csv}")


# ==============================================================================
# TABLE 30: MAX ELECTRICAL POWER & THERMAL STABILITY (5 TRIALS)
# ==============================================================================
def run_table30(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    out_csv = os.path.join(out_dir, "table30_power_thermal.csv")
    headers = [
        "Trial", "Timestamp", "Duration_s", "Heaters_On", "Pumps_On",
        "Measured_Volts_V", "Measured_Amps_A", "Apparent_Power_VA",
        "SSR_Enclosure_Temp_C", "Breaker_Tripped", "Status"
    ]
    print(f"[Table 30] Max Electrical Power & Thermal Stability ({args.trials} trials)")
    print("Note: If testing immersion heater, ensure >= 5.5 L water is inside Tank 1.")
    for t in range(1, args.trials + 1):
        input(f"\nTrial {t}/{args.trials} - Clamp meter ready on AC main? Press Enter to start {args.duration}s full-load test...")
        qs = urllib.parse.urlencode({
            "action": "start",
            "sec": args.duration,
            "heaters": 1 if args.heaters else 0,
            "pumps": 1 if args.pumps else 0
        })
        start_res, _ = http_get_json(args.ip, f"/api/test/full_load?{qs}")
        print(f"  Full load active for {start_res['duration_sec']}s (heaters_enabled={start_res['heaters_enabled']}, vol={start_res['vol_l']} L)")
        for sec_left in range(start_res["duration_sec"], 0, -5):
            print(f"    {sec_left}s remaining... watch peak clamp meter A/V", end="\r", flush=True)
            time.sleep(min(5, sec_left))
        http_get_json(args.ip, "/api/test/full_load?action=stop")
        print("\n  Full load stopped safely.")

        volts = ask_float("  Measured AC Line Voltage (V) [220.0]: ", 220.0)
        amps = ask_float("  Peak Measured Current (A): ")
        ssr_c = ask_float("  Peak SSR / Enclosure Temp (C): ")
        tripped = ask_yes_no("  Did circuit breaker / thermal cutoff trip? [y/N]: ", False)
        va = round(volts * amps, 1)

        append_csv_row(out_csv, headers, {
            "Trial": t,
            "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "Duration_s": start_res["duration_sec"],
            "Heaters_On": start_res["heaters_enabled"],
            "Pumps_On": start_res["pumps_enabled"],
            "Measured_Volts_V": volts,
            "Measured_Amps_A": amps,
            "Apparent_Power_VA": va,
            "SSR_Enclosure_Temp_C": ssr_c,
            "Breaker_Tripped": "Yes" if tripped else "No",
            "Status": "Passed" if not tripped else "Failed"
        })
    print(f"Saved -> {out_csv}")


# ==============================================================================
# COMBINED WATER COMMISSIONING RUN (TABLES 17, 18, 21, 24, 27, 28, 31)
# ==============================================================================
def run_water_commissioning(args) -> None:
    out_dir = ensure_out_dir(args.out_dir)
    raw_dir = os.path.join(out_dir, "raw_water_runs")
    os.makedirs(raw_dir, exist_ok=True)

    print(f"[Water Commissioning Suite] Trials={args.trials}, PreHeat={args.pre_c} C, Cool={args.cool_c} C, Past={args.past_c} C")
    print("Each water pour automatically logs telemetry for Tables 17, 18, 21, 24, 27, 28, and 31.")

    for t in range(args.start_trial, args.start_trial + args.trials):
        start_vol_l = ask_float(f"\n--- Trial {t} ---\nPour water into Tank 1 and enter measured fill volume (L, e.g. 6.0): ")
        qs = urllib.parse.urlencode({
            "pre_c": args.pre_c,
            "cool_c": args.cool_c,
            "ferm_c": args.ferm_c,
            "past_c": args.past_c,
            "skip_heat": 0
        })
        start_res, _ = http_get_json(args.ip, f"/api/brew/start_water?{qs}")
        print(f"  Cycle started: pre_c={start_res['pre_c']} C, cool_c={start_res['cool_c']} C, hx711={start_res['vol_l']} L")

        raw_csv = os.path.join(raw_dir, f"water_trial_{t}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")
        raw_headers = [
            "Elapsed_s", "Stage", "Vol_L", "Ch0_L", "Ch1_L", "Ch2_L",
            "Preheat_C", "Past_C", "Ferm_C", "Heater_pct", "Fan_State",
            "Pump1", "Pump2", "Xfer_Vol_L", "Xfer1_L", "Xfer2_L",
            "Flow_P1", "Flow_P2", "Pre_Sterilized", "Pre_Cooled", "RTT_ms"
        ]

        t_start = time.time()
        temps_pre = []
        max_pre_c = -999.0
        fan_trigger_c = None
        pump1_seen = False
        max_xfer1 = 0.0
        max_pulse1 = 0
        reached_ferm = False

        print("  Auto-logging every 2s. Press Ctrl+C ONLY once Stage 1->2 transfer completes (or let cycle advance automatically)...")
        try:
            while True:
                d, rtt = http_get_json(args.ip, "/data", timeout=5.0)
                el = round(time.time() - t_start, 1)
                pre_c = float(d.get("ll", 0.0))
                if pre_c > 5.0:
                    temps_pre.append((el, pre_c))
                    max_pre_c = max(max_pre_c, pre_c)
                if fan_trigger_c is None and int(d.get("fan", 0)) == 1:
                    fan_trigger_c = pre_c
                if int(d.get("p1", 0)) == 1:
                    pump1_seen = True
                max_xfer1 = max(max_xfer1, float(d.get("xfer1", 0.0)), float(d.get("xfer_vol", 0.0)))
                max_pulse1 = max(max_pulse1, int(d.get("flow_p1", 0)))

                append_csv_row(raw_csv, raw_headers, {
                    "Elapsed_s": el,
                    "Stage": d.get("stage"),
                    "Vol_L": d.get("vol"),
                    "Ch0_L": d.get("ch0"),
                    "Ch1_L": d.get("ch1"),
                    "Ch2_L": d.get("ch2"),
                    "Preheat_C": pre_c,
                    "Past_C": d.get("lp"),
                    "Ferm_C": d.get("fl"),
                    "Heater_pct": d.get("hp"),
                    "Fan_State": d.get("fan"),
                    "Pump1": d.get("p1"),
                    "Pump2": d.get("p2"),
                    "Xfer_Vol_L": d.get("xfer_vol"),
                    "Xfer1_L": d.get("xfer1"),
                    "Xfer2_L": d.get("xfer2"),
                    "Flow_P1": d.get("flow_p1"),
                    "Flow_P2": d.get("flow_p2"),
                    "Pre_Sterilized": d.get("pre_sterilized"),
                    "Pre_Cooled": d.get("pre_cooled"),
                    "RTT_ms": rtt
                })

                sys.stdout.write(
                    f"\r  t={el:6.1f}s | stg={d.get('stage')} | Pre={pre_c:5.1f}C | Htr={d.get('hp'):3}% | Fan={d.get('fan')} | P1={d.get('p1')} | Xfer1={d.get('xfer1')}L ({d.get('flow_p1')}p)   "
                )
                sys.stdout.flush()

                if int(d.get("stage", -1)) == 1 and int(d.get("xfer", 0)) == 0:
                    reached_ferm = True
                    print("\n  Stage 1->2 transfer completed cleanly!")
                    break
                time.sleep(2.0)
        except KeyboardInterrupt:
            print("\n  Run stopped by user.")

        http_get_json(args.ip, "/api/brew/start_water?stop=1")

        # Derived metrics from telemetry
        start_c = temps_pre[0][1] if temps_pre else 0.0
        peak_idx = max(range(len(temps_pre)), key=lambda i: temps_pre[i][1]) if temps_pre else 0
        time_to_peak_min = max(0.01, temps_pre[peak_idx][0] / 60.0) if temps_pre else 0.0
        ramp_c_per_min = round((max_pre_c - start_c) / time_to_peak_min, 2) if temps_pre else 0.0
        overshoot_c = round(max(0.0, max_pre_c - args.pre_c), 2)

        leaks_ok = ask_yes_no("  [Table 27] Zero drips/leaks observed on bulkheads & hoses? [Y/n]: ", True)
        residual_ml = ask_float("  [Table 31] Drained residual water in 250 mL cylinder (mL): ")
        transferred_l = round(max(0.0, start_vol_l - (residual_ml / 1000.0)), 3)
        efficiency_pct = round((transferred_l / start_vol_l) * 100.0, 2) if start_vol_l > 0 else 0.0

        # Save derived Table rows
        append_csv_row(
            os.path.join(out_dir, "table28_thermal_ramp.csv"),
            ["Trial", "Start_Temp_C", "Target_Temp_C", "Peak_Temp_C", "Time_to_Peak_min", "Ramp_Rate_C_per_min", "Overshoot_C"],
            {
                "Trial": t,
                "Start_Temp_C": round(start_c, 1),
                "Target_Temp_C": args.pre_c,
                "Peak_Temp_C": round(max_pre_c, 1),
                "Time_to_Peak_min": round(time_to_peak_min, 2),
                "Ramp_Rate_C_per_min": ramp_c_per_min,
                "Overshoot_C": overshoot_c
            }
        )
        append_csv_row(
            os.path.join(out_dir, "table27_leak_integrity.csv"),
            ["Trial", "Volume_L", "Max_Temp_C", "Leaks_Observed", "Status"],
            {
                "Trial": t,
                "Volume_L": start_vol_l,
                "Max_Temp_C": round(max_pre_c, 1),
                "Leaks_Observed": "0 drips" if leaks_ok else "Leak detected",
                "Status": "Passed" if leaks_ok else "Failed"
            }
        )
        append_csv_row(
            os.path.join(out_dir, "table31_transfer_efficiency.csv"),
            ["Trial", "Start_Vol_L", "FlowMeter_Xfer_L", "Flow_Pulses", "Residual_Drained_mL", "True_Transferred_L", "Efficiency_Pct", "Status"],
            {
                "Trial": t,
                "Start_Vol_L": start_vol_l,
                "FlowMeter_Xfer_L": round(max_xfer1, 2),
                "Flow_Pulses": max_pulse1,
                "Residual_Drained_mL": round(residual_ml, 1),
                "True_Transferred_L": transferred_l,
                "Efficiency_Pct": efficiency_pct,
                "Status": "Passed" if efficiency_pct >= 90.0 else "Check Residual"
            }
        )
        append_csv_row(
            os.path.join(out_dir, "table21_24_commissioning_summary.csv"),
            ["Trial", "Fan_Trigger_Temp_C", "Pump1_Activated", "Reached_Ferm_Stage", "Raw_Log_CSV"],
            {
                "Trial": t,
                "Fan_Trigger_Temp_C": round(fan_trigger_c, 1) if fan_trigger_c is not None else "NotObserved",
                "Pump1_Activated": "Yes" if pump1_seen else "No",
                "Reached_Ferm_Stage": "Yes" if reached_ferm else "StoppedEarly",
                "Raw_Log_CSV": raw_csv
            }
        )
        print(f"  Trial {t} saved: Ramp={ramp_c_per_min} C/min | Efficiency={efficiency_pct}% | Raw={raw_csv}")


# ==============================================================================
# ECONOMIC FEASIBILITY CALCULATOR (FROM MEASURED LOGS)
# ==============================================================================
def run_econ(args) -> None:
    csv_path = args.csv or os.path.join(DEFAULT_LOG_DIR, "brew_wifi_20261021_192709.csv")
    rows = []
    with open(csv_path, encoding="utf-8", errors="replace") as f:
        header = None
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            parts = s.split(",")
            if parts[0] == "Date":
                header = parts
                continue
            if header and len(parts) >= 22:
                rows.append(parts)

    if len(rows) < 2:
        print("Insufficient CSV rows for economic calculation.", file=sys.stderr)
        sys.exit(1)

    def parse_ts(p):
        for fmt in ("%Y-%m-%d %H:%M:%S", "%Y/%m/%d %H:%M:%S"):
            try:
                return datetime.strptime(f"{p[0]} {p[1]}", fmt)
            except ValueError:
                continue
        return None

    stage_kwh = {"PREHEAT": 0.0, "FERMENTATION": 0.0, "PASTEURIZATION": 0.0}
    heater_kw = {"PREHEAT": args.pre_kw, "FERMENTATION": args.ferm_kw, "PASTEURIZATION": args.past_kw}
    base_kw = args.base_w / 1000.0

    total_hrs = 0.0
    for i in range(1, len(rows)):
        t0, t1 = parse_ts(rows[i - 1]), parse_ts(rows[i])
        if not t0 or not t1 or t1 < t0:
            continue
        dt_h = min(60.0, (t1 - t0).total_seconds()) / 3600.0
        total_hrs += dt_h
        stg = rows[i][2].strip()
        try:
            hp = float(rows[i][21]) / 100.0
        except (ValueError, IndexError):
            hp = 0.0
        if stg in stage_kwh:
            stage_kwh[stg] += (heater_kw[stg] * hp + base_kw) * dt_h

    total_kwh = sum(stage_kwh.values())
    total_cost_php = total_kwh * args.rate
    per_liter_php = total_cost_php / args.batch_liters if args.batch_liters > 0 else 0.0

    print(f"[Economic Feasibility Summary] Log: {os.path.basename(csv_path)}")
    print(f"  Logged Duration: {total_hrs:.2f} hrs | Electricity Rate: {args.rate:.2f} PHP/kWh | Batch: {args.batch_liters:.2f} L")
    for k, v in stage_kwh.items():
        print(f"  {k:<15}: {v:.3f} kWh ({v * args.rate:.2f} PHP)")
    print(f"  TOTAL ENERGY   : {total_kwh:.3f} kWh")
    print(f"  TOTAL ELEC COST: {total_cost_php:.2f} PHP ({per_liter_php:.2f} PHP/L)")


def main() -> None:
    parser = argparse.ArgumentParser(description="WineBrew Bench Test Suite & Table CSV Generator")
    parser.add_argument("--ip", default=DEFAULT_ESP32_IP, help="Primary ESP32 IP")
    parser.add_argument("--out-dir", default=DEFAULT_OUT_DIR, help="Output CSV directory")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p29 = sub.add_parser("offline-table29", help="Analyze BCH-01 CSV continuity for Table 29")
    p29.add_argument("--csv", default="", help="Path to CSV telemetry log")
    p29.add_argument("--ping-live", action="store_true", help="Also measure live /data RTT latency x10")

    p16 = sub.add_parser("table16", help="Run Table 16 Dry-Fire Safety Lockout (5 trials)")
    p16.add_argument("--trials", type=int, default=5)

    sub.add_parser("table20", help="Run Table 20 Pasteurization Interlock (10 trials)")

    p22 = sub.add_parser("table22", help="Run Table 19/22 Yeast Dispensing & Mixing (5 trials)")
    p22.add_argument("--trials", type=int, default=5)
    p22.add_argument("--grams", type=float, default=3.5)
    p22.add_argument("--mix-sec", type=int, default=30)
    p22.add_argument("--mix-spd", type=int, default=100)

    p23 = sub.add_parser("table23", help="Run Table 23 Live pH Cutoff (5 trials)")
    p23.add_argument("--trials", type=int, default=5)

    p25 = sub.add_parser("table25", help="Run Table 25 Stage Logging & ms Latency (5 trials)")
    p25.add_argument("--trials", type=int, default=5)

    p30 = sub.add_parser("table30", help="Run Table 30 Full Load Power & Thermal Test (5 trials)")
    p30.add_argument("--trials", type=int, default=5)
    p30.add_argument("--duration", type=int, default=45)
    p30.add_argument("--heaters", action="store_true", help="Enable immersion SSR (requires >= 5L water)")
    p30.add_argument("--pumps", action="store_true", help="Enable pumps (requires water in tanks)")

    pw = sub.add_parser("water-run", help="Run Water Commissioning Suite (Tables 17, 18, 21, 24, 27, 28, 31)")
    pw.add_argument("--trials", type=int, default=5)
    pw.add_argument("--start-trial", type=int, default=1)
    pw.add_argument("--pre-c", type=float, default=60.0)
    pw.add_argument("--cool-c", type=float, default=30.0)
    pw.add_argument("--ferm-c", type=float, default=30.0)
    pw.add_argument("--past-c", type=float, default=60.0)

    pe = sub.add_parser("econ", help="Compute electrical kWh & PHP cost from logged CSV")
    pe.add_argument("--csv", default="")
    pe.add_argument("--rate", type=float, default=12.50, help="PHP per kWh")
    pe.add_argument("--batch-liters", type=float, default=6.86)
    pe.add_argument("--pre-kw", type=float, default=1.5)
    pe.add_argument("--ferm-kw", type=float, default=1.9)
    pe.add_argument("--past-kw", type=float, default=1.5)
    pe.add_argument("--base-w", type=float, default=25.0, help="Controllers + fans idle load in Watts")

    args = parser.parse_args()
    dispatch = {
        "offline-table29": run_offline_table29,
        "table16": run_table16,
        "table20": run_table20,
        "table22": run_table22,
        "table23": run_table23,
        "table25": run_table25,
        "table30": run_table30,
        "water-run": run_water_commissioning,
        "econ": run_econ,
    }
    dispatch[args.cmd](args)


if __name__ == "__main__":
    main()
