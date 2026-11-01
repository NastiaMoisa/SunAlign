#!/usr/bin/env python3
"""Sum actual foreground-scope GPIO high intervals against independent VCD time.

The caller supplies wire.json from decode_lab3_uart.py. No SysTick duration is
summed. IRQs preempting a high scope remain included; ISR time outside scopes,
scheduling checks and inter-task spin are excluded. The 40 boot calibration
pulses estimate visible empty-pulse width only, not all instrumentation cost.
"""
import argparse, hashlib, json, re, statistics
from pathlib import Path

def sha(p):
 h=hashlib.sha256()
 with open(p,'rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()

def summarize(vcd,wire,start_ms,end_ms):
 d=json.loads(Path(wire).read_text())
 if d.get('sha256')!=sha(vcd) or d.get('framing_errors')!=0:raise ValueError('VCD hash mismatch or UART framing errors')
 lines=d['lines']; boot=[x for x in lines if x['text'].startswith('BOOT SunAlign lab3 src-b16af10ff4b0 ')]
 if len(boot)!=1:raise ValueError('expected one diagnostic BOOT')
 points=d['accepted_panel_timestamps']; bytime={p['mcu_ms']:p['wire_ns'] for p in points}
 if start_ms not in bytime or end_ms not in bytime:raise ValueError('window endpoints absent')
 start,end=bytime[start_ms],bytime[end_ms]
 anchor_end=end
 if end<=start or abs((end-start)-(end_ms-start_ms)*1_000_000)>100_000_000:raise ValueError('incompatible MCU/wire window')
 nsec=1_000_000_000;bins=(end-start)//nsec
 if bins<10:raise ValueError('need >=10 full one-second VCD bins')
 bins=int(bins);end=start+bins*nsec; totals=[0]*bins;durations=[];cal=[];last=0;rise=None;level=None;sym=None;timescale=False;t=0
 with open(vcd) as f:
  for line in f:
   x=line.strip()
   if x.startswith('$timescale'):timescale=('1ns' in x.replace(' ',''))
   elif x.startswith('$var') and x.split()[4]=='D6':sym=x.split()[3]
   elif x.startswith('#'):t=int(x[1:]);last=t
   elif sym and x in ('0'+sym,'1'+sym):
    val=int(x[0])
    if level is None:level=val;rise=t if val else None;continue
    if val==level:continue
    if val:rise=t
    else:
     if rise is None:raise ValueError('orphan marker fall')
     hi=t-rise
     if t<boot[0]['wire_ns'] and len(cal)<40:cal.append(hi)
     a=max(rise,start);b=min(t,end)
     if b>a:
      durations.append((b-a)/1000)
      i=(a-start)//nsec
      while a<b:
       j=min(b,start+(i+1)*nsec);totals[int(i)]+=j-a;a=j;i+=1
     rise=None
    level=val
 if not timescale or not sym or last<end:raise ValueError('VCD incomplete or wrong timescale')
 if len(cal)!=40:raise ValueError(f'expected 40 initial calibration pulses, found {len(cal)}')
 if not durations:raise ValueError('no foreground scope intervals')
 return {'source_id':'src-b16af10ff4b0','vcd_sha256':d['sha256'],'window':{'mcu_start_ms':start_ms,'mcu_end_anchor_ms':end_ms,'wire_start_ns':start,'wire_end_ns':end,'wire_end_anchor_ns':anchor_end,'full_seconds':bins,'basis':'wire timestamps of PANEL; 1 s bins use VCD clock'},'scope_intervals':{'n':len(durations),'min_us':min(durations),'avg_us':statistics.fmean(durations),'max_us':max(durations)},'foreground_fraction':{'per_second_percent':[x/nsec*100 for x in totals],'min_percent':min(totals)/nsec*100,'avg_percent':sum(totals)/(bins*nsec)*100,'max_percent':max(totals)/nsec*100},'empty_marker_pulses':{'n':len(cal),'min_us':min(cal)/1000,'avg_us':statistics.fmean(cal)/1000,'max_us':max(cal)/1000},'limits':['Diagnostic build only; not the frozen S1/S2/S3 ELF.','GPIO marker covers executed foreground scopes, including ADC and I2C polling inside active tasks; empty not-due checks and inter-task spin are excluded.','IRQs during a marked interval are included; ISR work outside those intervals is excluded.','Marker writes and altered execution perturb timing; empty pulse width is not total probe overhead.','Asynchronous UART wire time is not CPU time; only foreground TX setup and formatting are marked.','PANEL wire timestamps include serial-queue latency; 1 ns VCD timescale is not a physical accuracy guarantee.']}

def main():
 p=argparse.ArgumentParser();p.add_argument('vcd');p.add_argument('wire');p.add_argument('--output',required=True);p.add_argument('--start-ms',type=int,default=2500);p.add_argument('--end-ms',type=int,default=14500);a=p.parse_args();r=summarize(a.vcd,a.wire,a.start_ms,a.end_ms);Path(a.output).write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n');print(json.dumps({'window':r['window'],'foreground_fraction':r['foreground_fraction'],'scope_intervals':r['scope_intervals'],'empty_marker_pulses':r['empty_marker_pulses']}))
if __name__=='__main__':main()
