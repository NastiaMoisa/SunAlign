"""Summarize fresh simulator boots using original VCD t=0, not host arrivals."""
import argparse, hashlib, json, pathlib, re, statistics, subprocess, sys
p=argparse.ArgumentParser()
p.add_argument("directory")
p.add_argument("--output",required=True)
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
output=pathlib.Path(a.output)
output.parent.mkdir(parents=True,exist_ok=True)
detail=output.parent/"startup-details"
detail.mkdir(exist_ok=True)
trials=[]
for path in sorted(pathlib.Path(a.directory).glob("boot-*.vcd")):
 wire=detail/(path.stem+"-wire.json")
 gpio=detail/(path.stem+"-gpio.json")
 subprocess.run([sys.executable,str(root/"tools/decode_lab3_uart.py"),str(path),"--output",str(wire)],check=True,capture_output=True)
 subprocess.run([sys.executable,str(root/"tools/analyze_lab3_vcd.py"),str(path),"--output",str(gpio)],check=True,capture_output=True)
 d=json.loads(wire.read_text());g=json.loads(gpio.read_text())
 first={}
 for line in d["lines"]:
  m=re.fullmatch(r"FIRST (adc|imu) (\d+)",line["text"])
  if m and m[1] not in first:
   first[m[1]]={"firmware_from_observe_init_us":int(m[2]),"uart_wire_from_simulation_start_us":line["wire_ns"]/1000}
 banners=[x["text"] for x in d["lines"] if x["text"].startswith("BOOT SunAlign ")]
 trials.append({"source":str(path.resolve()),"sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
                "banners":banners,"framing_errors":d["framing_errors"],"first":first,
                "first_adc_gpio_from_simulation_start_us":g["first_adc_completion_ns"]/1000 if g["first_adc_completion_ns"] is not None else None,
                "complete":bool(banners) and set(first)=={"adc","imu"} and not d["framing_errors"]})
def stats(values):
 return {"n":len(values),"min_us":min(values),"avg_us":statistics.mean(values),"max_us":max(values)} if values else None
complete=[t for t in trials if t["complete"]]
result={"scope":"Fresh Wokwi simulation t=0. ADC is PA4 completion; IMU is first valid-data notification on TX, an upper bound including cooperative dispatch and UART delay. Not physical power ramp.",
        "trials":trials,"complete_trials":len(complete),
        "adc_gpio":stats([t["first_adc_gpio_from_simulation_start_us"] for t in complete]),
        "imu_uart_notification":stats([t["first"]["imu"]["uart_wire_from_simulation_start_us"] for t in complete]),
        "adc_internal":stats([t["first"]["adc"]["firmware_from_observe_init_us"] for t in complete]),
        "imu_internal":stats([t["first"]["imu"]["firmware_from_observe_init_us"] for t in complete])}
output.write_text(json.dumps(result,indent=2))
print("Fresh complete boots:",len(complete),"of",len(trials),result["adc_gpio"],result["imu_uart_notification"])
