"""Plot the recorded UART residual; do not interpret it as CPU latency."""
import argparse
import json
from pathlib import Path
import matplotlib.pyplot as plt

p = argparse.ArgumentParser()
p.add_argument("analysis")
p.add_argument("--output", required=True)
a = p.parse_args()
d = json.loads(Path(a.analysis).read_text())
w = d["uart_residual_windows"]
origin = w[0]["min_residual_us"]
fig, ax = plt.subplots(figsize=(10, 3.7))
ax.step([x["start_mcu_ms"] / 1000 for x in w],
        [(x["min_residual_us"] - origin) / 1000 for x in w], where="post", color="#26436b")
for x in d["large_residual_steps"]:
    ax.scatter(x["start_mcu_ms"] / 1000, (x["min_residual_us"] - origin) / 1000,
               color="#aa4433", s=28, zorder=3)
ax.set(xlabel="Reported MCU time, s", ylabel="Minimum wire minus MCU residual, ms",
       title=f"SunAlign {d['scenario']}: original VCD UART, 30 s windows")
ax.grid(alpha=.2)
fig.tight_layout()
fig.savefig(a.output, dpi=180)
plt.close(fig)
