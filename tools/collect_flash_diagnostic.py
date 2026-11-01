"""Capture the actual standalone Flash probe; never synthesize HAL outcomes."""
import argparse,datetime,hashlib,json,pathlib,re,time
import serial
p=argparse.ArgumentParser()
p.add_argument('--out',required=True)
p.add_argument('--port',type=int,default=43614)
p.add_argument('--build-dir',default='firmware/build-flash-diagnostic')
p.add_argument('--timeout',type=float,default=120)
a=p.parse_args(); root=pathlib.Path(__file__).resolve().parents[1]
out=pathlib.Path(a.out);out.parent.mkdir(parents=True,exist_ok=True)
if out.exists():raise SystemExit('Refusing to overwrite '+str(out))
build=root/a.build_dir; source=(build/'build_id.h').read_text().split('"')[1]
meta={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'source_id':source,
      'elf_sha256':hashlib.sha256((build/'firmware.elf').read_bytes()).hexdigest(),
      'diagram_sha256':hashlib.sha256((root/'diagram.json').read_bytes()).hexdigest(),
      'transport':'RFC2217, standalone diagnostic, host time is not MCU latency',
      'diagnostic_complete':False,'boot_seen':False,'records':[]}
ser=None; started=time.monotonic()
try:
 while ser is None:
  try:ser=serial.serial_for_url(f'rfc2217://localhost:{a.port}',115200,timeout=.05)
  except serial.SerialException:
   if time.monotonic()-started>a.timeout:raise TimeoutError('No diagnostic transport')
   time.sleep(.25)
 tail=b'';armed=False
 with out.open('xb') as stream:
  while time.monotonic()-started<a.timeout:
   block=ser.read(4096)
   stream.write(block);stream.flush();tail+=block
   finished=False
   while b'\n' in tail:
    line,tail=tail.split(b'\n',1);text=line.decode('ascii',errors='replace').rstrip('\r')
    if text.startswith('BOOT SunAlign FLASH_DIAG '+source):armed=True;meta['boot_seen']=True
    if armed and text.startswith(('FLASH_','CLOCK ')):
     meta['records'].append(text)
     if text.startswith('FLASH_DIAG_DONE '):meta['diagnostic_complete']=True;finished=True
     if text.startswith('FLASH_DIAG_ABORT '):meta['aborted']=text;finished=True
   if finished:break
  else:meta['timeout']=True
except BaseException as error:
 meta['collector_error']=repr(error)
 raise
finally:
 if ser:ser.close()
 meta['wall_seconds']=time.monotonic()-started
 if out.exists():meta['log_sha256']=hashlib.sha256(out.read_bytes()).hexdigest()
 out.with_suffix(out.suffix+'.json').write_text(json.dumps(meta,indent=2))
print('diagnostic_complete',meta['diagnostic_complete'],'records',len(meta['records']))
