"""Parse firmware measurements; missing metrics remain absent, never zero-filled."""
import argparse,json,re,statistics,pathlib,hashlib
p=argparse.ArgumentParser();p.add_argument('log');p.add_argument('--output',required=True);a=p.parse_args()
text=pathlib.Path(a.log).read_text(errors='replace'); text='BOOT SunAlign'+text.rsplit('BOOT SunAlign',1)[-1] if 'BOOT SunAlign' in text else text
result={'source':str(pathlib.Path(a.log).resolve()),'log_sha256':hashlib.sha256(pathlib.Path(a.log).read_bytes()).hexdigest(),'statistics':{},'events':{}}
banner=re.search(r'BOOT SunAlign lab3 (\S+)',text)
result['source_id']=banner[1] if banner else None
for name,n,lo,avg,hi in re.findall(r'^STAT (\S+) (\d+) (\d+) (\d+) (\d+)\r?$',text,re.M):
 result['statistics'][name]={'n':int(n),'min_us':int(lo) if int(n) else None,'avg_us':int(avg) if int(n) else None,'max_us':int(hi) if int(n) else None}
for name,pattern in [('zero_sample_to_accept',r'^ZERO (\d+) (\d+) \d+'),('save',r'^SAVE \d+ (\d+) (\d+)')]:
 matches=re.findall(pattern,text,re.M)
 values=[((int(y)-int(x))&0xffffffff) for x,y in matches] if name.startswith('zero') else [int(x) for x,y in matches if y=='1']
 if values:
  values.sort();result['events'][name]={'n':len(values),'min_us':min(values),'avg_us':statistics.mean(values),'max_us':max(values),'p99_us':values[min(len(values)-1,int(len(values)*.99))]}
for period,name in [(100000,'imu_dt'),(100000,'adc_dt'),(1000,'panel_dt'),(500000,'led_dt'),(500000,'log_dt')]:
 if name in result['statistics'] and result['statistics'][name]['n']:
  s=result['statistics'][name];s['jitter_min_us']=s['min_us']-period;s['jitter_max_us']=s['max_us']-period
result['resources']=[dict(zip(['stack_used','heap_peak','rx_lost','rx_errors','tx_lost','service_permille'],[*(int(v) for v in m[:5]), None if m[5]=='NA' else int(m[5])])) for m in re.findall(r'^RES (\d+) (\d+) (\d+) (\d+) (\d+) (\d+|NA)',text,re.M)]
result['deadlines']=[dict(zip(['panel','led','log','command'],map(int,m))) for m in re.findall(r'^DEADLINE (\d+) (\d+) (\d+) (\d+)',text,re.M)]
result['schedule']=[dict(zip(['panel_late_ms','panel_skips','led_late_ms','log_late_ms'],map(int,m))) for m in re.findall(r'^SCHED (\d+) (\d+) (\d+) (\d+)',text,re.M)]
attempts=re.findall(r'^SAVE (\d+) (\d+) ([01])\r?$',text,re.M)
result['save_attempts']={'n':len(attempts),'successful':sum(ok=='1' for _,_,ok in attempts),'failed':sum(ok=='0' for _,_,ok in attempts)}
result['evidence_sufficiency']={'zero_events':len(re.findall(r'^ZERO ',text,re.M)),'periodic_counts':{n:s['n'] for n,s in result['statistics'].items() if n.endswith('_dt')}}
result['clock']=[dict(zip(['reads','wraps','max_gap_mod_us'],map(int,m))) for m in re.findall(r'CLOCK mode=sampled16 reads=(\d+) wraps=(\d+) gap_mod_us=(\d+)',text)]
result['clock_review_required']=any(x['max_gap_mod_us']>=10000 for x in result['clock'])
result['timing_status']= 'requires TIM3-vs-VCD validation; simulation values are not physical MCU guarantees'
pathlib.Path(a.output).write_text(json.dumps(result,indent=2))
