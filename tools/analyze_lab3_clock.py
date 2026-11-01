"""Compare original wire time, MCU timestamps and matched ADC extrema.

Minimum UART residual per 30-second window reduces queue-delay influence.
It is a diagnostic, not a proof of a simulator implementation's root cause.
"""
import argparse, json, pathlib, re, statistics
p=argparse.ArgumentParser()
p.add_argument("scenario")
p.add_argument("--directory",default="docs/metrics/lab3")
a=p.parse_args()
folder=pathlib.Path(a.directory)
uart=json.loads((folder/(a.scenario+".json")).read_text())
gpio=json.loads((folder/(a.scenario+"-vcd.json")).read_text())
wire=json.loads((folder/(a.scenario+"-wire.json")).read_text())
points=[{**v,'residual_us':v['wire_ns']/1000-v['mcu_ms']*1000} for v in wire.get('accepted_panel_timestamps',[])]
exports=[]
zeros=[]
for line in wire["lines"]:
 text=line["text"]
 m=re.fullmatch(r"PANEL (\d+) .*",text)
 if m and 'accepted_panel_timestamps' not in wire:points.append({"mcu_ms":int(m[1]),"wire_ns":line["wire_ns"],"residual_us":line["wire_ns"]/1000-int(m[1])*1000})
 m=re.fullmatch(r"EXPORT_DONE (\d+) (\d+)",text)
 if m:exports.append({"start_us_mod32":int(m[1]),"duration_us":int(m[2]),"wire_ns":line["wire_ns"]})
 m=re.fullmatch(r"ZERO (\d+) (\d+) (\d+) src=poll",text)
 if m:zeros.append({"sample_us_mod32":int(m[1]),"accept_us_mod32":int(m[2]),"count":int(m[3]),"wire_ns":line["wire_ns"]})
bins={}
for point in points:
 bins.setdefault(point["mcu_ms"]//30000,[]).append(point["residual_us"])
windows=[{"start_mcu_ms":key*30000,"n":len(values),"min_residual_us":min(values),"median_residual_us":statistics.median(values),"max_residual_us":max(values)} for key,values in sorted(bins.items())]
for previous,current in zip(windows,windows[1:]):
 current["min_step_us"]=current["min_residual_us"]-previous["min_residual_us"]
adc_fw=uart["statistics"].get("adc_dt")
adc_wire=gpio.get("adc_completion_interval")
comparison=None
if adc_fw and adc_wire:
 comparison={"same_interval_count":adc_fw["n"]==adc_wire["n"],"firmware":adc_fw,"vcd":adc_wire,
             "vcd_minus_firmware_us":{key:adc_wire[key]-adc_fw[key] for key in ["min_us","avg_us","max_us"]}}
result={"scenario":a.scenario,"clock_review_required":uart.get("clock_review_required"),
        "rejected_uart_panel_records":wire.get("rejected_panel_records",[]),
        "wire_clock_check":wire.get("clock_check"),"framing_errors":wire["framing_errors"],
        "paired_adc":comparison,"uart_residual_windows":windows,
        "large_residual_steps":[w for w in windows if abs(w.get("min_step_us",0))>10000],
        "export_us_rollovers":[{"before":before,"after":after} for before,after in zip(exports,exports[1:]) if after["start_us_mod32"]<before["start_us_mod32"]],
        "zero_events":zeros,
        "interpretation":"Wire residual includes UART queue delay; a step is an observed diagnostic and needs matched GPIO/firmware evidence. No physical oscillator or simulator-root-cause claim."}
(folder/(a.scenario+"-clock.json")).write_text(json.dumps(result,indent=2))
print(a.scenario,"pairedADC",comparison,"largeResidualSteps",len(result["large_residual_steps"]),"usRollovers",len(result["export_us_rollovers"]))
