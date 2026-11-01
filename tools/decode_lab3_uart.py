"""Decode own VCD UART TX and retain independent wire timestamps for each line."""
import argparse,bisect,json,pathlib,re,statistics,hashlib
p=argparse.ArgumentParser();p.add_argument('vcd');p.add_argument('--output',required=True);a=p.parse_args()
t=0;sym=None;long_sym=None;edges=[];levels=[];long_start=None;long_intervals=[]
with open(a.vcd) as f:
 for raw in f:
  x=raw.strip()
  if x.startswith('$var'):
   if x.split()[4]=='D2':sym=x.split()[3]
   if x.split()[4]=='D6':long_sym=x.split()[3]
  elif x.startswith('$timescale'):
   assert re.search(r'\b1\s*ns\b',x),'Decoder requires actual 1 ns VCD timescale'
  elif x.startswith('#'):t=int(x[1:])
  elif sym and x in ['0'+sym,'1'+sym]:
   v=int(x[0]);
   if not levels or v!=levels[-1]:edges.append(t);levels.append(v)
  elif long_sym and x in ['0'+long_sym,'1'+long_sym]:
   if x[0]=='1':long_start=t
   elif long_start is not None:long_intervals.append((long_start,t));long_start=None
bit=8687.5;available=0;data=[];errors=0
for i,(at,v) in enumerate(zip(edges,levels)):
 if v or at<available or i==0 or levels[i-1]!=1:continue
 byte=0
 for k in range(8):
  j=bisect.bisect_right(edges,at+(k+1.5)*bit)-1;byte|=levels[j]<<k
 stop=levels[bisect.bisect_right(edges,at+9.5*bit)-1]
 if not stop:errors+=1
 data.append((at,byte));available=at+9.6*bit
lines=[];buf=bytearray();begin=0
for at,b in data:
 if not buf:begin=at
 if b==10:
  lines.append({'wire_ns':begin,'wire_end_ns':at+10*bit,'byte_count':len(buf)+1,'text':buf.decode(errors='replace').rstrip('\r')});buf.clear()
 else:buf.append(b)
points=[];rejected_panels=[]
for line in lines:
 m=re.fullmatch(r'PANEL (\d+) (\d+) (\d+) (-?\d+) (LEFT|RIGHT|CENTER|DARK|ERROR) (UNSET|LOWER|RAISE|LEVEL|ERROR) (\d+) (\d+)',line['text'])
 if m:
  tick=int(m[1])
  if int(m[2])>4095 or int(m[3])>4095 or abs(int(m[4]))>9000 or (points and not 0<(tick-points[-1][0])<=10000):
   rejected_panels.append({'wire_ns':line['wire_ns'],'text':line['text'],'reason':'range or chronological step incompatible with adjacent accepted records'});continue
  points.append((tick,line['wire_ns']))
 elif line['text'].startswith('PANEL '):
  rejected_panels.append({'wire_ns':line['wire_ns'],'text':line['text'],'reason':'incomplete or malformed record after UART loss'})
result={'source':str(pathlib.Path(a.vcd).resolve()),'sha256':hashlib.sha256(pathlib.Path(a.vcd).read_bytes()).hexdigest(),'bit_ns':bit,'framing_errors':errors,'lines':lines}
result['accepted_panel_timestamps']=[{'mcu_ms':ms,'wire_ns':ns} for ms,ns in points]
result['rejected_panel_records']=rejected_panels
byte_starts=[at for at,_ in data]
operation=[]
for lo,hi in long_intervals:
 n=bisect.bisect_left(byte_starts,hi)-bisect.bisect_left(byte_starts,lo)
 operation.append({'start_ns':lo,'end_ns':hi,'duration_us':(hi-lo)/1000,
                   'byte_frame_starts':n,'minimum_8n1_wire_us':n*10*bit/1000,
                   'scope':'All UART frames starting during PA6 HIGH, including competing telemetry; partial boundary frames can shift count by one.'})
result['long_operation_uart']=operation
ends=[x['end_ns'] for x in operation]
for line in lines:
 if line['text'].startswith('EXPORT_DONE '):
  i=bisect.bisect_right(ends,line['wire_ns'])-1
  if i>=0 and line['wire_ns']-ends[i]<5000000:
   operation[i]['export_done_text']=line['text']
if len(points)>1:
 dt=points[-1][0]-points[0][0];wire=points[-1][1]-points[0][1]
 result['clock_check']={'panel_points':len(points),'mcu_elapsed_ms':dt,'wire_elapsed_ms':wire/1e6,'wire_to_mcu_ratio':wire/(dt*1e6),'monotonic':all(b[0]>a[0] for a,b in zip(points,points[1:]))}
pathlib.Path(a.output).write_text(json.dumps(result,indent=2))
print(result.get('clock_check'), 'framing_errors',errors)
