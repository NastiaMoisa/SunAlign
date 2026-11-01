"""Fingerprint the compiled project sources, including dirty/untracked content."""
from pathlib import Path
import argparse,hashlib
parser=argparse.ArgumentParser()
parser.add_argument('--variant',choices=['app','flash'],default='app')
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
h=hashlib.sha256()
h.update(('variant:'+args.variant+'\0').encode())
paths=[root/'firmware/Makefile',root/'firmware/STM32C031C6Tx_FLASH.ld',root/'firmware/startup_stm32c031xx.s',root/'docs/vendor-manifest.json',Path(__file__).resolve()]
for directory in ['App','Devices','Platform','Core','Diagnostics']:
    paths.extend((root/'firmware'/directory).rglob('*.[ch]'))
for p in sorted(paths):
    h.update(str(p.relative_to(root)).encode()); h.update(b'\0');h.update(p.read_bytes())
print('src-'+h.hexdigest()[:12])
