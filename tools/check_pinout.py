"""Check the declared board mapping against the saved CubeMX pin inventory."""
import json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
diagram = json.loads((root / "diagram.json").read_text())
connections = {frozenset(c[:2]) for c in diagram["connections"]}
required = {"PB8": "mpu:SCL", "PB9": "mpu:SDA", "PA0": "ldrLeft:AO",
            "PA1": "ldrRight:AO", "PB0": "zero:1.l", "PA4": "logic:D3",
            "PA2": "$serialMonitor:RX", "PA3": "$serialMonitor:TX"}
for pin, target in required.items():
    assert frozenset(("nucleo:" + pin, target)) in connections, (pin, target)
ioc = dict(line.split("=", 1) for line in (root / "firmware/firmware.ioc").read_text().splitlines() if "=" in line)
for pin, signal in {"PB8": "I2C1_SCL", "PB9": "I2C1_SDA", "PA0": "ADC1_IN0", "PA1": "ADC1_IN1",
                    "PB0": "GPIO_Input", "PA4": "GPIO_Output", "PA5": "GPIO_Output",
                    "PA2": "USART2_TX", "PA3": "USART2_RX"}.items():
    assert ioc[pin + ".Signal"] == signal, pin
assert ioc["RCC.SYSCLKFreq_VALUE"] == "48000000"
assert ioc["I2C1.Timing"] == "0x20303E5D"
assert ioc["Mcu.Package"] == "LQFP48"
print("PASS pinout: diagram.json and .ioc agree on sensors, UART, marker, LED and clock")
