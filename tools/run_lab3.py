"""Record actual MCU serial output over Wokwi's documented RFC2217 transport.

No simulated numbers are manufactured here. Time limits use PANEL's MCU tick.
Install pyserial, start this script, then start the simulator in the GUI slot.
"""
import argparse, datetime, hashlib, json, pathlib, re, time
import serial
p=argparse.ArgumentParser()
p.add_argument('mode',choices=['S1','S2','S3','boundary'])
p.add_argument('--seconds',type=int)
p.add_argument('--rate',type=float,default=20,help='requested host bursts/s; S3 burst contains s+d')
p.add_argument('--port',type=int,default=43614)
p.add_argument('--out',required=True)
p.add_argument('--build-dir',default='firmware/build',help='selected ELF and build_id.h relative to repository')
a=p.parse_args(); duration=a.seconds or (3600 if a.mode=='S2' else 300)
out=pathlib.Path(a.out); out.parent.mkdir(parents=True,exist_ok=True)
if out.exists(): raise SystemExit('Refusing to overwrite existing evidence: '+str(out))
root=pathlib.Path(__file__).resolve().parents[1]
build=root/a.build_dir
manifest={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(), 'mode':a.mode,
          'requested_mcu_seconds':duration,'requested_host_bursts_hz':a.rate,'port':a.port,
          'elf_sha256':hashlib.sha256((build/'firmware.elf').read_bytes()).hexdigest(),
          'build_directory':a.build_dir,
          'diagram_sha256':hashlib.sha256((root/'diagram.json').read_bytes()).hexdigest(),
          'transport':'Wokwi VS Code RFC2217, no host time used as MCU latency'}
connect_deadline=time.monotonic()+120
while True:
 try:
  ser=serial.serial_for_url(f'rfc2217://localhost:{a.port}',115200,timeout=0.01)
  break
 except serial.SerialException:
  if time.monotonic()>=connect_deadline: raise
  time.sleep(0.5)
start=time.monotonic(); next_stats=start; next_command=start; next_nominal=start
first_tick=None; final_tick=None; sent=0; tail=b''; armed=False; rejected_panels=0
expected=(build/'build_id.h').read_text().split(chr(34))[1]
manifest['source_id']=expected
try:
 with out.open('xb') as stream:
  while True:
   data=ser.read(4096)
   if data:
    stream.write(data); stream.flush(); tail+=data
    while b'\n' in tail:
     line,tail=tail.split(b'\n',1)
     if line.startswith(('BOOT SunAlign lab3 '+expected).encode()):armed=True
     match=re.fullmatch(rb'PANEL (\d+) (\d+) (\d+) (-?\d+) (LEFT|RIGHT|CENTER|DARK|ERROR) (UNSET|LOWER|RAISE|LEVEL|ERROR) (\d+) (\d+)\r?',line)
     if match and armed:
      if first_tick is None and int(match[1])>1000:
       armed=False;continue
      candidate=int(match[1])
      if int(match[2])>4095 or int(match[3])>4095 or abs(int(match[4]))>9000 or (final_tick is not None and ((candidate-final_tick)&0xffffffff)>10000):
       rejected_panels+=1;continue
      final_tick=int(match[1]); first_tick=final_tick if first_tick is None else first_tick
    if first_tick is not None and ((final_tick-first_tick)&0xffffffff)>=duration*1000: break
   now=time.monotonic()
   if now>=next_stats:
    ser.write(b't'); sent+=1; next_stats=now+30
   if a.mode=='S2' and now>=next_nominal:
    ser.write(b'cd'); sent+=2; next_nominal=now+60
   if a.mode in ['S3','boundary'] and now>=next_command:
    # Real storage work only if a valid ZERO exists. No synthetic delay.
    ser.write(b'sd' if a.mode=='S3' else b'd'); sent+=2 if a.mode=='S3' else 1
    next_command=now+1/a.rate
   if now-start>max(duration*10,600): raise TimeoutError('MCU duration not reached within wall-clock bound')
  ser.write(b't'); deadline=time.monotonic()+3
  while time.monotonic()<deadline:
   stream.write(ser.read(4096))
except BaseException as error:
 manifest['error']=repr(error)
 raise
finally:
 ser.close(); manifest.update(first_tick_ms=first_tick,final_tick_ms=final_tick,
   wall_seconds=time.monotonic()-start,commands_sent=sent,
   completed=first_tick is not None and ((final_tick-first_tick)&0xffffffff)>=duration*1000,
   rejected_implausible_panel_records=rejected_panels)
 if out.exists(): manifest['log_sha256']=hashlib.sha256(out.read_bytes()).hexdigest()
 out.with_suffix(out.suffix+'.json').write_text(json.dumps(manifest,indent=2))
