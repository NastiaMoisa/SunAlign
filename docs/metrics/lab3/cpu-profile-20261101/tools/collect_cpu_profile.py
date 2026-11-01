#!/usr/bin/env python3
"""Bounded SunAlign UART scenarios; GPIO stimulus comes from Wokwi chip.

One raw file per boot; progress uses only validated PANEL timestamps.
This collector does not produce simulated MCU values or proof of GPIO response.
"""
import argparse, datetime, hashlib, json, os, re, signal, time
from pathlib import Path
import serial

PANEL = re.compile(rb"PANEL (\d+) (\d+) (\d+) (-?\d+) (LEFT|RIGHT|CENTER|DARK|ERROR) (UNSET|LOWER|RAISE|LEVEL|ERROR) (\d+) (\d+)\r?")
SOURCE = "src-b16af10ff4b0"
ELF = "3bc51b829e7e8e3804919fe8866ed4408f314d19144638cf91c4123de81f19aa"
DURATIONS = {"gate": 45, "s3": 150, "fault": 15, "boundary": 120, "flash": 50}

def utc(): return datetime.datetime.now(datetime.timezone.utc).isoformat()
def sha(p):
    h=hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda: f.read(1048576),b""): h.update(b)
    return h.hexdigest()
def save(p,d):
    q=p.with_suffix(".tmp"); q.write_text(json.dumps(d,indent=2)+"\n"); os.replace(q,p)

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--workspace",type=Path,required=True)
    p.add_argument("--run-dir",type=Path,required=True)
    p.add_argument("--mode",choices=DURATIONS,required=True)
    p.add_argument("--port",type=int,default=43652)
    a=p.parse_args(); w=a.workspace.resolve()
    if sha(w/"firmware.elf") != ELF: raise SystemExit("ELF mismatch")
    a.run_dir.mkdir(parents=True,exist_ok=False); root=a.run_dir.resolve()
    st={"mode":a.mode,"source_id":SOURCE,"elf_sha256":ELF,"diagram_sha256":sha(w/"diagram.json"),
        "workspace":str(w),"port":a.port,"duration_mcu_s":DURATIONS[a.mode],"started_utc":utc(),
        "state":"connecting","complete":False,"boot_count":0,"first_mcu_ms":None,
        "last_mcu_ms":None,"commands":[],"reason":None}
    raw=root/"uart.log"; status=root/"status.json"; save(status,st)
    stop=False
    def interrupt(*_):
        nonlocal stop
        stop=True
    signal.signal(signal.SIGTERM,interrupt); signal.signal(signal.SIGINT,interrupt)
    ser=None
    try:
        deadline=time.monotonic()+120
        while not stop:
            try: ser=serial.serial_for_url(f"rfc2217://localhost:{a.port}",115200,timeout=.03);break
            except serial.SerialException:
                if time.monotonic()>=deadline: raise TimeoutError("listener unavailable")
                time.sleep(.25)
        if ser is None: raise InterruptedError("stopped before connection")
        st["connected_utc"]=utc(); st["state"]="waiting_boot"; pending=b""
        last_progress=time.monotonic(); last_save=0; next_c=next_d=next_stats=0; fault_sent=restore_sent=False
        first_imu=False; healthy_seen=False
        flash_sent=0; end_deadline=None; end_elapsed=None
        def send(data,purpose):
            ser.write(data)
            st["commands"].append({"utc":utc(),"mcu_ms":st["last_mcu_ms"],"bytes":data.decode(),"purpose":purpose})
        with raw.open("xb",buffering=0) as out:
            while not stop:
                data=ser.read(4096); now=time.monotonic()
                if data:
                    out.write(data);pending+=data
                    if len(pending)>65536: raise ValueError("unterminated UART over limit")
                    while b"\n" in pending:
                        line,pending=pending.split(b"\n",1)
                        if line.startswith(b"BOOT SunAlign lab3 "):
                            st["boot_count"]+=1
                            if st["boot_count"]!=1 or not line.startswith(("BOOT SunAlign lab3 "+SOURCE+" ").encode()): raise ValueError("wrong BOOT or reset")
                            st["state"]="running";send(b"v","clock")
                        if line.startswith(b"FIRST imu "):
                            first_imu=True
                            st["first_imu_uart"]=line.decode("utf-8","replace").rstrip()
                        m=PANEL.fullmatch(line)
                        if m and st["boot_count"]==1:
                            tick=int(m[1])
                            if int(m[2])>4095 or int(m[3])>4095 or abs(int(m[4]))>9000: raise ValueError("invalid PANEL")
                            if st["last_mcu_ms"] is not None:
                                dt=(tick-st["last_mcu_ms"])&0xffffffff
                                if dt==0 or dt>10000: raise ValueError(f"PANEL gap/reset {dt}ms")
                            if st["first_mcu_ms"] is None:
                                if tick>1000: raise ValueError("BOOT observed but early PANEL missed")
                                st["first_mcu_ms"]=tick
                            st["last_mcu_ms"]=tick;last_progress=now
                            elapsed=(tick-st["first_mcu_ms"])&0xffffffff;st["elapsed_mcu_ms"]=elapsed
                            if first_imu and m[5]!=b"ERROR" and m[6]!=b"ERROR" and not healthy_seen:
                                healthy_seen=True
                                st["healthy_first_mcu_ms"]=tick
                            if elapsed>=DURATIONS[a.mode]*1000 and end_deadline is None:
                                end_elapsed=elapsed;send(b"t","end_stats");end_deadline=now+5;st["state"]="end_stats"
                if end_deadline is not None and now>=end_deadline:
                    st["complete"]=True;st["state"]="complete";break
                if now-last_progress>120: raise TimeoutError("MCU PANEL stalled")
                if st["first_mcu_ms"] is not None and end_deadline is None:
                    elapsed=(st["last_mcu_ms"]-st["first_mcu_ms"])&0xffffffff
                    if now>=next_stats: send(b"t","stats");next_stats=now+20
                    if a.mode in ("gate","s3"):
                        # Preserve an initial valid IMU sample; flood d (real export)
                        # rather than endlessly restarting 100ms MPU settle with c.
                        if healthy_seen and elapsed>=5000 and now>=next_c:
                            send(b"c","rare_refresh");next_c=now+10
                        if healthy_seen and elapsed>=5000 and now>=next_d:
                            send(b"d","long_export");next_d=now+(.5 if a.mode=="gate" else .1)
                    elif a.mode=="fault":
                        if healthy_seen and elapsed>=4000 and not fault_sent: send(b"d","diagnostic_export_once");fault_sent=True
                        if healthy_seen and elapsed>=9000 and not restore_sent: send(b"c","diagnostic_refresh_once");restore_sent=True
                    elif a.mode=="boundary":
                        rates=(2,5,10,20,40)
                        rate=rates[min(elapsed//24000,len(rates)-1)]
                        if now>=next_d: send(b"d",f"boundary_{rate}_hz");next_d=now+1/rate
                    elif a.mode=="flash":
                        if healthy_seen and elapsed>=5000 and now>=next_d:send(b"d","long_export_for_zero");next_d=now+.5
                        if healthy_seen and elapsed>=20000+flash_sent*5000 and flash_sent<3:
                            send(b"s","actual_flash_save");flash_sent+=1
                if now-last_save>=10:st["updated_utc"]=utc();save(status,st);last_save=now
        if stop: raise InterruptedError("collector stopped")
        st["requested_interval_completed_ms"]=end_elapsed
    except BaseException as e:
        st["state"]="incomplete";st["reason"]=f"{type(e).__name__}: {e}"
    finally:
        if ser:ser.close()
        st["ended_utc"]=utc();st["raw_sha256"]=sha(raw) if raw.exists() else None;save(status,st)
    print(json.dumps({"state":st["state"],"reason":st["reason"],"elapsed_mcu_ms":st.get("elapsed_mcu_ms")}))
    return 0 if st["complete"] else 1
if __name__=="__main__":raise SystemExit(main())
