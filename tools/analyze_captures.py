"""Measure Wokwi events and cross-check decoded bytes without generating signals."""
from pathlib import Path
import json
import re
import statistics
from hashlib import sha256

root = Path(__file__).resolve().parents[1]
folder = root / "docs/captures"
def events(path):
    time = 0
    result = []
    for line in path.read_text().splitlines():
        if line.startswith("#"):
            time = int(line[1:])
        elif len(line) == 2 and line[0] in "01":
            result.append((time,line[1],int(line[0])))
    return result
ev = events(folder / "normal.vcd")
def rises(symbol):
    return [t for t,s,v in ev if s == symbol and v == 1 and t > 0]
scl = rises("!")
periods = [b-a for a,b in zip(scl,scl[1:]) if 5000 < b-a < 15000]
adc = rises("$")
led = [t for t,s,v in ev if s == "&" and t > 2_000_000]
state = {"!":1,'"':1}
bits=[]; collecting=False
for t,s,v in ev:
    if s == '"' and state['"'] == 1 and v == 0 and state['!'] == 1 and not collecting:
        collecting=True
    elif s == '!' and state['!'] == 0 and v == 1 and collecting:
        bits.append(state['"'])
        if len(bits)==18: break
    if s in state:state[s]=v
assert bits[:9]==[1,1,0,1,0,0,0,0,0],bits
assert bits[9:18]==[0,1,1,1,0,1,0,1,0],bits
decoded=(folder/"normal-i2c.txt").read_text()
read=[int(x,16) for x in re.findall(r'Data read: ([0-9A-F]{2})',decoded)]
assert read[:7]==[0x68,0,0,0,0,0x40,0],read[:7]
fault=(folder/"address-nack-i2c.txt").read_text()
assert re.search(r'Address write: 69\n[^\n]*NACK',fault),"Missing address NACK"
uart=(root/"docs/verification/lab2/normal-uart.txt").read_text()
assert "mg=0,0,1000" in uart
out={"source":"Wokwi VCD, 1 ns event timestamps; SR downsampled to 2 MHz",
     "scl_period_ns_median":statistics.median(periods),
     "scl_hz":1e9/statistics.median(periods),
     "adc_period_ms_median":statistics.median([b-a for a,b in zip(adc,adc[1:])])/1e6,
     "led_edge_period_ms_median":statistics.median([b-a for a,b in zip(led,led[1:])])/1e6,
     "manual_bits":{"address_and_ack":bits[:9],"register_and_ack":bits[9:18]},
     "captured_accel_bytes":read[1:7],"decoded_mg":[0,0,1000],
     "files":{p.name:sha256(p.read_bytes()).hexdigest() for p in folder.glob("*.vcd")}}
(folder/"measurements.json").write_text(json.dumps(out,indent=2)+"\n")
print(json.dumps(out,indent=2))
