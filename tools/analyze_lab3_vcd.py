"""Measure original Wokwi VCD edges and compare with actual MCU UART events."""
import argparse,pathlib,re,json,statistics,hashlib
p=argparse.ArgumentParser();p.add_argument('vcd');p.add_argument('--uart');p.add_argument('--output',required=True);a=p.parse_args()
path=pathlib.Path(a.vcd);symbols={};levels={};t=0;unit=None;adc=[];led=[];long_start=None;long_intervals=[];pending=None;pairs=[];button_high_since=None
with path.open() as stream:
 for raw in stream:
  line=raw.strip()
  if line.startswith('$timescale'):
   m=re.search(r'(\d+)\s*(ns|us|ps|ms)',line)
   if not m: raise SystemExit('Unsupported timescale')
   unit=int(m[1])*{'ps':.001,'ns':1,'us':1000,'ms':1000000}[m[2]]
  if line.startswith('$var'):
   words=line.split();symbols[words[3]]=words[4]
  if line.startswith('#'):t=int(line[1:]);continue
  if len(line)<2 or line[0] not in '01' or line[1:] not in symbols:continue
  channel=symbols[line[1:]];value=int(line[0]);old=levels.get(channel);levels[channel]=value
  if old is None:
   if channel=='D4' and value==1 and unit is not None:button_high_since=t*unit
   continue
  if old==value:continue
  if unit is None:raise SystemExit('Missing timescale')
  at=t*unit
  if channel=='D3' and value==0:adc.append(at)
  if channel=='D5':led.append(at)
  if channel=='D6':
   if value:long_start=at
   elif long_start is not None:long_intervals.append([long_start,at]);long_start=None
  if channel=='D4':
   if value==1:button_high_since=at
   else:
    new_press=button_high_since is not None and at-button_high_since>=30000000
    if new_press:pending={'first_ns':at,'last_ns':at,'during_long':bool(levels.get('D6'))}
    elif pending is not None:pending['last_ns']=at
  if channel=='D7' and pending is not None:
   edge=pending['first_ns'];pairs.append({'edge_ns':edge,'stable_edge_ns':pending['last_ns'],'accept_ns':at,'latency_us':(at-edge)/1000,'latency_from_last_fall_us':(at-pending['last_ns'])/1000,'during_long':pending['during_long']});pending=None
if not {'D3','D4','D6','D7'}<=set(symbols.values()):raise SystemExit('Not a SunAlign Lab3 capture: required D3/D4/D6/D7 absent')
adc_limit=None;led_limit=None
if a.uart:
 raw=pathlib.Path(a.uart).read_text(errors='replace')
 if 'BOOT SunAlign' in raw:raw=raw.rsplit('BOOT SunAlign',1)[-1]
 count=re.findall(r'^STAT adc_dt (\d+) ',raw,re.M)
 if count:adc_limit=int(count[-1])
 count=re.findall(r'^STAT led_dt (\d+) ',raw,re.M)
 if count:led_limit=int(count[-1])
if adc_limit is not None:adc=adc[:adc_limit+1]
if led_limit is not None:led=led[:led_limit+1]
def stats(values):
 if not values:return None
 values=sorted(values);return {'n':len(values),'min_us':min(values),'avg_us':statistics.mean(values),'max_us':max(values),'p99_us':values[min(len(values)-1,int(len(values)*.99))]}
result={'source':str(path.resolve()),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'timescale_ns':unit,
 'first_adc_completion_ns':adc[0] if adc else None,'capture_end_ns':t*unit,
 'adc_completion_interval':stats([(b-a)/1000 for a,b in zip(adc,adc[1:])]),
 'd5_transition_interval':stats([(b-a)/1000 for a,b in zip(led,led[1:])]),
 'button_physical_to_accept':stats([p['latency_us'] for p in pairs]),'button_during_long':stats([p['latency_us'] for p in pairs if p['during_long']]),
 'long_operation_gpio_width':stats([(b-a)/1000 for a,b in long_intervals]),'button_pairs':pairs,'long_intervals_ns':long_intervals,
 'interpretation':'Wokwi edges. Input event begins at first fall after at least 30 ms HIGH; subsequent bounce falls retained separately. No physical hardware accuracy claim.'}
if a.uart:
 uart_path=pathlib.Path(a.uart)
 result['uart_sha256']=hashlib.sha256(uart_path.read_bytes()).hexdigest()
 sidecar=uart_path.with_suffix(uart_path.suffix+'.json')
 if sidecar.exists():result['run_manifest']=json.loads(sidecar.read_text())
 text=uart_path.read_text(errors='replace')
 if 'BOOT SunAlign' in text:text=text.rsplit('BOOT SunAlign',1)[-1]
 logs=re.findall(r'^ZERO (\d+) (\d+) (\d+)',text,re.M)
 result['successful_saves_uart']=len(re.findall(r'^SAVE \d+ \d+ 1\s*$',text,re.M))
 result['marker_semantics']='PA6 is the union of an active UART export and a Flash call; it cannot isolate Flash latency.'
 result['uart_zero_count']=len(logs);result['pair_count_matches_uart']=len(logs)==len(pairs)
 if len(logs)==len(pairs):
  differences=[]
  for pair,(edge,accepted,count) in zip(pairs,logs):
   pair['sample_to_accept_us']=(int(accepted)-int(edge))&0xffffffff
   pair['gpio_minus_sample_us']=pair['latency_us']-pair['sample_to_accept_us'];differences.append(pair['gpio_minus_sample_us'])
  result['gpio_minus_sample']=stats(differences)
pathlib.Path(a.output).write_text(json.dumps(result,indent=2))
