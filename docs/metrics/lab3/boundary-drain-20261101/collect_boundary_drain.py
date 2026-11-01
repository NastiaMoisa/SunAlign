#!/usr/bin/env python3
"""Bounded Wokwi TX-load sweep; command rates use host time, not MCU latency."""
import argparse, datetime, hashlib, json, re, signal, time
from pathlib import Path
import serial

SOURCE='src-af2d5aa374d1'
ELF='9791f5375be0fea0e576c3d3c857ba3c16355593547e42675980834e502f8471'
PANEL=re.compile(r'^PANEL (\d+) (\d+) (\d+) (-?\d+) (LEFT|RIGHT|CENTER|DARK|ERROR) (UNSET|LOWER|RAISE|LEVEL|ERROR) (\d+) (\d+)$')
RES=re.compile(r'^RES (\d+) (\d+) (\d+) (\d+) (\d+) NA$')
SCHED=re.compile(r'^SCHED (\d+) (\d+) (\d+) (\d+)$')
DEADLINE=re.compile(r'^DEADLINE (\d+) (\d+) (\d+) (\d+)$')
CLOCK=re.compile(r'^CLOCK mode=systick_ms resolution_us=1000 configured=1 wcet=unavailable cpu=unavailable masked_ticks=may_be_lost$')
STAT=re.compile(r'^STAT (board|imu|panel|led|log|loop|command|button|save|imu_dt|adc_dt|panel_dt|led_dt|log_dt|dump_wire) \d+ \d+ \d+ \d+$')
NAMES={'board','imu','panel','led','log','loop','command','button','save','imu_dt','adc_dt','panel_dt','led_dt','log_dt','dump_wire'}
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def utc(): return datetime.datetime.now(datetime.timezone.utc).isoformat()
class Report:
 def __init__(self): self.stats=set(); self.res=self.sched=self.deadline=None; self.clock=False
 def add(self,line):
  m=STAT.fullmatch(line)
  if m: self.stats.add(m[1])
  for attr,pattern in [('res',RES),('sched',SCHED),('deadline',DEADLINE)]:
   m=pattern.fullmatch(line)
   if m: setattr(self,attr,[int(x) for x in m.groups()])
  if CLOCK.fullmatch(line): self.clock=True
 def complete(self): return self.stats==NAMES and self.res is not None and self.sched is not None and self.deadline is not None and self.clock

def main():
 p=argparse.ArgumentParser();p.add_argument('--workspace',type=Path,required=True);p.add_argument('--run-dir',type=Path,required=True);p.add_argument('--port',type=int,default=43614);a=p.parse_args()
 if sha(a.workspace/'firmware.elf')!=ELF: raise SystemExit('ELF mismatch')
 a.run_dir.mkdir(parents=True,exist_ok=False); raw=a.run_dir/'uart.log'; path=a.run_dir/'status.json'
 st={'source_id':SOURCE,'elf_sha256':ELF,'diagram_sha256':sha(a.workspace/'diagram.json'),'state':'connecting','phases':[],'commands':[],'boot_count':0,'started_utc':utc()}; stop=False;ser=None;pending=b'';tick=None;first_imu=False;healthy=False;report=None
 def save(): path.write_text(json.dumps(st,ensure_ascii=False,indent=2)+'\n')
 def interrupt(*_):
  nonlocal stop
  stop=True
 signal.signal(signal.SIGTERM,interrupt);signal.signal(signal.SIGINT,interrupt)
 def send(v,purpose):
  ser.write(v);st['commands'].append({'utc':utc(),'last_panel_ms':tick,'bytes':v.decode(),'purpose':purpose})
 def pump(out,seconds):
  nonlocal pending,tick,first_imu,healthy,report
  end=time.monotonic()+seconds
  while time.monotonic()<end:
   if stop: raise InterruptedError('interrupted')
   data=ser.read(4096)
   if not data: continue
   out.write(data);pending+=data
   if len(pending)>65536:raise ValueError('unterminated input')
   while b'\n' in pending:
    b,pending=pending.split(b'\n',1);line=b.rstrip(b'\r').decode('ascii','replace')
    if line.startswith('BOOT SunAlign lab3 '):
     st['boot_count']+=1
     if st['boot_count']!=1 or not line.startswith('BOOT SunAlign lab3 '+SOURCE+' '):raise ValueError('unexpected boot/source')
    if line.startswith('FIRST imu '):first_imu=True
    m=PANEL.fullmatch(line)
    if m:
     new=int(m[1]);
     if tick is not None and not 0<(new-tick)<=10000:raise ValueError('PANEL gap/reset')
     tick=new
     if first_imu and m[5]!='ERROR' and m[6]!='ERROR':healthy=True
    if report is not None: report.add(line)
 try:
  end=time.monotonic()+120
  while not stop and ser is None:
   try:ser=serial.serial_for_url(f'rfc2217://localhost:{a.port}',115200,timeout=.02)
   except serial.SerialException:
    if time.monotonic()>end:raise TimeoutError('listener unavailable')
    time.sleep(.2)
  if ser is None:raise InterruptedError('stopped')
  with raw.open('xb',buffering=0) as out:
   st['state']='waiting_healthy';save();end=time.monotonic()+45
   while (not healthy or st['boot_count']!=1) and time.monotonic()<end:pump(out,.1)
   if not healthy or st['boot_count']!=1:raise TimeoutError('healthy startup missing')
   st['healthy_panel_ms']=tick
   def snapshot():
    nonlocal report
    report=Report();send(b't','quiet_stats');end=time.monotonic()+15
    while not report.complete() and time.monotonic()<end:pump(out,.05)
    if not report.complete():raise TimeoutError('complete quiet statistics unavailable')
    val={'res':report.res,'sched':report.sched,'deadline':report.deadline,'panel_ms':tick};report=None
    return val
   pump(out,4);st['baseline']=snapshot();save()
   for hz in (2,5,10,20,40):
    phase={'target_hz':hz,'start_utc':utc(),'start_panel_ms':tick,'sent':0}; start=time.monotonic();next_send=start;end=start+8;send_times=[]
    while time.monotonic()<end:
     now=time.monotonic()
     if now>=next_send:send(b'd',f'phase_{hz}');send_times.append(now);phase['sent']+=1;next_send=now+1/hz
     pump(out,min(.01,max(0,end-time.monotonic())))
    phase['send_span_s']=send_times[-1]-send_times[0] if len(send_times)>1 else None
    phase['actual_send_hz']=(len(send_times)-1)/phase['send_span_s'] if len(send_times)>1 else None
    phase['last_send_panel_ms']=tick;st['phases'].append(phase);save()
    pump(out,4);phase['snapshot']=snapshot();phase['end_utc']=utc();save()
   st['state']='complete'
 except BaseException as e:st['state']='incomplete';st['reason']=f'{type(e).__name__}: {e}'
 finally:
  if ser:ser.close()
  st['ended_utc']=utc();st['raw_sha256']=sha(raw) if raw.exists() else None;save()
 print(json.dumps({'state':st['state'],'reason':st.get('reason'),'phases':len(st['phases'])}))
 return 0 if st['state']=='complete' else 1
if __name__=='__main__':raise SystemExit(main())
