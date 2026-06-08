#!/usr/bin/env python3
"""
Load Cell Live Monitor
Run: python3 monitor.py

Keys (press anytime, no Enter needed):
  t  — tare
  +  — cal factor +100
  -  — cal factor -100
  [  — cal factor +10
  ]  — cal factor -10
  c  — calibrate with known weight (wizard)
  r  — print raw ADC value
  q  — quit
"""

import serial, time, sys, os, subprocess, termios, tty, threading, select

PORT  = "/dev/cu.usbserial-0001"
BAUD  = 115200

# ── kill any stale process holding the port ──────────────────────────────────
def free_port():
    result = subprocess.run(["lsof", "-t", PORT], capture_output=True, text=True)
    pids = result.stdout.strip().split()
    for pid in pids:
        try:
            os.kill(int(pid), 9)
        except Exception:
            pass
    if pids:
        print(f"  [INFO] Freed port from PID(s): {' '.join(pids)}")
        time.sleep(0.5)

# ── non-blocking single keypress ─────────────────────────────────────────────
def get_key_nonblocking():
    if select.select([sys.stdin], [], [], 0)[0]:
        return sys.stdin.read(1)
    return None

# ── send a command, return lines that come back ───────────────────────────────
def send_cmd(ser, cmd, wait=2.5):
    ser.reset_input_buffer()
    ser.write((cmd + "\n").encode())
    time.sleep(wait)
    raw = ser.read(ser.in_waiting or 512)
    return raw.decode("utf-8", errors="replace").strip()

# ── calibration wizard (blocking, pauses stream) ─────────────────────────────
def calibrate_wizard(ser):
    # restore normal terminal for input
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, OLD_SETTINGS)

    print("\n")
    print("  ╔══════════════════════════════════╗")
    print("  ║    CALIBRATION WIZARD            ║")
    print("  ╚══════════════════════════════════╝")
    print("  Step 1: Remove everything from the scale.")
    input("          Press Enter when ready... ")
    print("  >> Taring (taking 10 samples)...")
    resp = send_cmd(ser, "t", wait=3.0)
    print(f"  << {resp}")

    print("\n  Step 2: Place a container with a KNOWN volume of liquid.")
    known = input("          Enter the known weight in Liters (e.g. 10): ").strip()
    if not known:
        print("  Cancelled.")
    else:
        print(f"  >> Calibrating with {known} L...")
        resp = send_cmd(ser, f"k{known}", wait=4.0)
        print(f"  << {resp}")
        for line in resp.splitlines():
            if "factor" in line.lower():
                print(f"\n  ★ {line.strip()}")
        print("\n  Done! Readings will now use the new factor.")
        print("  If it looks correct, copy the factor to config.h.")

    input("\n  Press Enter to return to live stream... ")
    # re-enable raw mode
    tty.setraw(sys.stdin.fileno())

# ─────────────────────────────────────────────────────────────────────────────

def main():
    global OLD_SETTINGS

    free_port()

    print(f"\n  Connecting to {PORT} @ {BAUD}...")
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
    except Exception as e:
        print(f"\n  ERROR: {e}")
        sys.exit(1)

    time.sleep(2)
    ser.reset_input_buffer()
    print("  Connected. Readings start in 1s.\n")
    print("  Keys: [t] tare  [+/-] cal ±100  [[ /]] cal ±10  [c] calibrate  [r] raw  [q] quit\n")
    print("  " + "─"*55)

    # save terminal state and go raw (single keypress, no Enter)
    OLD_SETTINGS = termios.tcgetattr(sys.stdin)
    tty.setraw(sys.stdin.fileno())

    try:
        last_read = 0
        READ_INTERVAL = 1.2  # seconds between auto-reads

        while True:
            # ── auto stream reading ──────────────────────────────────────────
            now = time.time()
            if now - last_read >= READ_INTERVAL:
                ser.reset_input_buffer()
                ser.write(b"w\n")
                time.sleep(1.5)
                raw = ser.read(ser.in_waiting or 256).decode("utf-8", errors="replace")
                for line in raw.splitlines():
                    line = line.strip()
                    if "[WEIGHT]" in line:
                        val = line.replace("[WEIGHT]", "").strip()
                        timestamp = time.strftime("%H:%M:%S")
                        # clear line and print live reading
                        sys.stdout.write(f"\r  {timestamp}  Weight: {val:<20}\n")
                        sys.stdout.flush()
                last_read = time.time()

            # ── check for keypress ───────────────────────────────────────────
            key = get_key_nonblocking()
            if key is None:
                time.sleep(0.05)
                continue

            if key == "q":
                break

            elif key == "t":
                sys.stdout.write("\r  >> Taring...                              \n")
                sys.stdout.flush()
                resp = send_cmd(ser, "t", wait=3.0)
                sys.stdout.write(f"\r  << {resp}\n")
                sys.stdout.flush()
                last_read = 0  # force immediate read after tare

            elif key == "+":
                resp = send_cmd(ser, "+", wait=0.5)
                for line in resp.splitlines():
                    if "factor" in line.lower() or "CAL" in line:
                        sys.stdout.write(f"\r  << {line.strip()}\n")
                        sys.stdout.flush()
                last_read = 0

            elif key == "-":
                resp = send_cmd(ser, "-", wait=0.5)
                for line in resp.splitlines():
                    if "factor" in line.lower() or "CAL" in line:
                        sys.stdout.write(f"\r  << {line.strip()}\n")
                        sys.stdout.flush()
                last_read = 0

            elif key == "[":
                resp = send_cmd(ser, "+10", wait=0.5)
                for line in resp.splitlines():
                    if "factor" in line.lower() or "CAL" in line:
                        sys.stdout.write(f"\r  << {line.strip()}\n")
                        sys.stdout.flush()
                last_read = 0

            elif key == "]":
                resp = send_cmd(ser, "-10", wait=0.5)
                for line in resp.splitlines():
                    if "factor" in line.lower() or "CAL" in line:
                        sys.stdout.write(f"\r  << {line.strip()}\n")
                        sys.stdout.flush()
                last_read = 0

            elif key == "r":
                sys.stdout.write("\r  >> Reading RAW ADC...                    \n")
                sys.stdout.flush()
                resp = send_cmd(ser, "r", wait=3.0)
                sys.stdout.write(f"\r  << {resp}\n")
                sys.stdout.flush()

            elif key == "c":
                calibrate_wizard(ser)
                last_read = 0

    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, OLD_SETTINGS)
        ser.close()
        print("\n\n  Disconnected.\n")

if __name__ == "__main__":
    main()
