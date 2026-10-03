"""Reject dependencies on HAL, Core and Platform in portable layers."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
allowed = {"sunalign.h", "mpu6050.h"}
errors = []
for folder in ("App", "Devices"):
    for path in (root / "firmware" / folder).glob("*.[ch]"):
        text = path.read_text()
        for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"', text, re.M):
            if name not in allowed:
                errors.append(f"{path.name}: forbidden include {name}")
        if re.search(r'\b(?:HAL_\w+|GPIO[A-F]|I2C1|USART2)\b', text):
            errors.append(f"{path.name}: hardware dependency")
if errors:
    raise SystemExit("\n".join(errors))
print("PASS architecture: App and Devices depend only on portable interfaces")
