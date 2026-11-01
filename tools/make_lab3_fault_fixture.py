"""Derive an explicit model fault fixture without altering the baseline."""
from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1]
base=root/"diagram.json"
diagram=json.loads(base.read_text())
diagram["parts"] += [
    {"type":"wokwi-slide-switch","id":"addressMode","top":80,"left":540,"attrs":{"value":"","bounce":"0"}},
    {"type":"wokwi-slide-switch","id":"sdaClamp","top":170,"left":540,"attrs":{"value":"","bounce":"0"}},
]
diagram["connections"]=[c for c in diagram["connections"] if set(c[:2]) not in
                         [{"mpu:AD0","nucleo:GND.1"},{"logic:D5","nucleo:PA5"}]]
diagram["connections"] += [
    ["addressMode:1","nucleo:GND.1","black",[]],
    ["addressMode:2","mpu:AD0","purple",[]],
    ["addressMode:3","nucleo:3V3","red",[]],
    ["logic:D5","mpu:AD0","purple",[]],
    ["sdaClamp:2","mpu:SDA","blue",[]],
    ["sdaClamp:3","nucleo:GND.1","black",[]],
]
out=root/"docs/metrics/lab3/diagram.fault-switches.json"
out.write_text(json.dumps(diagram,indent=2)+"\n")
info={
    "baseline_sha256":hashlib.sha256(base.read_bytes()).hexdigest(),
    "fixture_sha256":hashlib.sha256(out.read_bytes()).hexdigest(),
    "changes":["AD0 SPDT switch: left GND=0x68; right VCC=0x69",
               "SDA clamp SPDT switch: left open; right SDA-to-GND",
               "D5 is AD0 in this fixture, not LED; all other analyzer channels unchanged"],
    "status":"prepared, not yet executed; runtime behavior of model must be observed",
    "references":["https://docs.wokwi.com/parts/wokwi-slide-switch",
                  "https://docs.wokwi.com/parts/wokwi-mpu6050"],
    "scope":"Only model circuit fault. No physical hardware or power-cut claim.",
}
out.with_suffix(".json.provenance.json").write_text(json.dumps(info,indent=2))
print(out.relative_to(root),info["fixture_sha256"])
