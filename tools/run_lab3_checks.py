"""Additional real Wokwi trials, kept separate from S1/S2/S3.

Uses MCU PANEL timestamps for phase duration. Host schedules input only.
No firmware image writes, mocked sensor results, or fabricated samples.
"""
import argparse
import datetime
import hashlib
import json
import pathlib
import re
import time
import serial

p = argparse.ArgumentParser()
p.add_argument("mode", choices=["faults", "startups", "boundary"])
p.add_argument("--out", required=True)
p.add_argument("--port", type=int, default=43614)
p.add_argument("--count", type=int, default=30)
p.add_argument("--phase-seconds", type=int, default=20)
p.add_argument("--rates", default="1,5,10,20,50,100,200,500,1000")
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
out = pathlib.Path(a.out)
if out.exists():
    raise SystemExit("Refusing to overwrite evidence: " + str(out))
out.parent.mkdir(parents=True, exist_ok=True)
manifest = {
    "mode": a.mode, "started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "source_id": (root / "firmware/build/build_id.h").read_text().split('"')[1],
    "elf_sha256": hashlib.sha256((root / "firmware/build/firmware.elf").read_bytes()).hexdigest(),
    "diagram_sha256": hashlib.sha256((root / "diagram.json").read_bytes()).hexdigest(),
    "trials": [], "completed": False,
    "time_basis": "MCU PANEL for durations; host clock only schedules inputs and bounds waiting",
}
ser = serial.serial_for_url(f"rfc2217://localhost:{a.port}", 115200, timeout=0.005)
stream = out.open("xb")
tail = b""
lines = []
panel = None
panels = []

def read():
    global tail, panel
    data = ser.read(4096)
    if data:
        stream.write(data)
        stream.flush()
        tail += data
        while b"\n" in tail:
            raw, tail = tail.split(b"\n", 1)
            line = raw.decode(errors="replace").strip("\r\x00")
            lines.append(line)
            m = re.fullmatch(r"PANEL (\d+) (\d+) (\d+) (-?\d+) (\S+) (\S+) (\d+) (\d+)", line)
            if m:
                panel = {"ms": int(m[1]), "horizontal": m[5], "vertical": m[6],
                         "zero_count": int(m[7]), "imu_errors": int(m[8])}
                panels.append(dict(panel))

def until(predicate, seconds=60):
    deadline = time.monotonic() + seconds
    while not predicate():
        read()
        if time.monotonic() > deadline:
            raise TimeoutError("Condition not reached in " + str(seconds) + " host seconds")

def snapshot():
    # Let prior output drain, then collect a complete diagnostic report.
    deadline = time.monotonic() + 0.6
    while time.monotonic() < deadline:
        read()
    start = len(lines)
    ser.write(b"t")
    until(lambda: any(s.startswith("CLOCK mode=") for s in lines[start:]))
    block = lines[start:]
    stats = {}
    for line in block:
        m = re.fullmatch(r"STAT (\S+) (\d+) (\d+) (\d+) (\d+)", line)
        if m:
            stats[m[1]] = dict(zip(["n", "min_us", "avg_us", "max_us"], map(int, m.groups()[1:])))
    resources = [list(map(int, s.split()[1:])) for s in block if re.fullmatch(r"RES(?: \d+){6}", s)]
    deadlines = [list(map(int, s.split()[1:])) for s in block if re.fullmatch(r"DEADLINE(?: \d+){4}", s)]
    return {"byte_offset": stream.tell(), "panel": dict(panel) if panel else None,
            "statistics": stats, "resources": resources[-1] if resources else None,
            "deadlines": deadlines[-1] if deadlines else None}

try:
    until(lambda: panel is not None)
    manifest["initial"] = snapshot()
    if a.mode == "faults":
        for i in range(a.count):
            before = dict(panel)
            event_start = len(panels)
            offset = stream.tell()
            ser.write(b"n")
            until(lambda: any(v["imu_errors"]>before["imu_errors"] and v["horizontal"]=="ERROR" for v in panels[event_start:]))
            fault = next(v for v in panels[event_start:] if v["imu_errors"]>before["imu_errors"] and v["horizontal"]=="ERROR")
            until(lambda: any(v["imu_errors"]==fault["imu_errors"] and v["horizontal"]!="ERROR" for v in panels[event_start:]))
            recovered = next(v for v in panels[event_start:] if v["imu_errors"]==fault["imu_errors"] and v["horizontal"]!="ERROR")
            manifest["trials"].append({"trial": i + 1, "byte_offset": offset, "before": before,
                                       "first_error_panel": fault, "first_recovered_panel": recovered,
                                       "scope": "diagnostic address NACK at 0x69, not a physical disconnection"})
    elif a.mode == "startups":
        for i in range(a.count):
            start = len(lines)
            offset = stream.tell()
            ser.write(b"r")
            until(lambda: any(s.startswith("BOOT SunAlign lab3 ") for s in lines[start:]))
            until(lambda: any(s.startswith("FIRST adc ") for s in lines[start:]) and
                          any(s.startswith("FIRST imu ") for s in lines[start:]))
            block = lines[start:]
            first = {s.split()[1]: int(s.split()[2]) for s in block if re.fullmatch(r"FIRST (adc|imu) \d+", s)}
            stores = [s for s in block if s.startswith("STORE ")]
            manifest["trials"].append({"trial": i + 1, "byte_offset": offset,
                                       "first_from_observe_init_us": first, "store": stores[-1] if stores else None,
                                       "scope": "NVIC software reset, excludes pre-observe_init time and power ramp"})
            # Ensure the transport has consumed at least one fresh PANEL before next reset.
            end = len(lines)
            until(lambda: any(s.startswith("PANEL ") for s in lines[end:]))
    else:
        for rate in map(float, a.rates.split(",")):
            before = snapshot()
            start_ms = panel["ms"]
            started = time.monotonic()
            sent = 0
            while ((panel["ms"] - start_ms) & 0xffffffff) < a.phase_seconds * 1000:
                read()
                due = int((time.monotonic() - started) * rate) + 1 - sent
                if due > 0:
                    batch = min(due, 100)
                    ser.write(b"d" * batch)
                    sent += batch
                if time.monotonic() - started > a.phase_seconds * 10 + 10:
                    raise TimeoutError("Boundary phase MCU duration not reached")
            phase_end = dict(panel)
            after = snapshot()
            n0 = before["statistics"].get("command", {}).get("n")
            n1 = after["statistics"].get("command", {}).get("n")
            elapsed = (after["panel"]["ms"] - before["panel"]["ms"]) & 0xffffffff
            # Both snapshots include their own t. Subtract the one end t.
            received = n1 - n0 - 1 if n0 is not None and n1 is not None else None
            manifest["trials"].append({
                "requested_host_bursts_hz": rate, "host_commands_sent": sent,
                "phase_start_mcu_ms": start_ms, "phase_end_panel": phase_end,
                "before": before, "after": after, "snapshot_window_ms": elapsed,
                "received_d_commands": received,
                "received_d_per_mcu_s": received * 1000 / elapsed if received is not None and elapsed else None,
            })
    manifest["final"] = snapshot()
    manifest["completed"] = True
except BaseException as error:
    manifest["error"] = repr(error)
    raise
finally:
    ser.close()
    stream.close()
    manifest["log_sha256"] = hashlib.sha256(out.read_bytes()).hexdigest()
    out.with_suffix(out.suffix + ".json").write_text(json.dumps(manifest, indent=2))
