"""Review candidate clock records; independent VCD validation is still required."""
import argparse
import hashlib
import json
import pathlib
import re

p = argparse.ArgumentParser()
p.add_argument('log')
p.add_argument('--output', required=True)
p.add_argument('--source-id', required=True, help='expected loaded firmware source ID')
a = p.parse_args()
path = pathlib.Path(a.log)
raw = path.read_bytes()
text = raw.decode('ascii', errors='replace')
# RFC2217 can include an earlier run. A malformed last boot invalidates identity.
banners = list(re.finditer(r'^BOOT SunAlign[^\r\n]*', text, re.M))
banner = banners[-1] if banners else None
current = text[banner.start():] if banner else ''
identity = re.fullmatch(r'BOOT SunAlign lab3 (src-[0-9a-f]{12})\b[^\r\n]*', banner.group() if banner else '')
source_id = identity.group(1) if identity else None
clock_fields = ('valid', 'errors', 'reads', 'extra_wraps', 'gap_ms', 'shadow', 'old_gap_us', 'nested')
shadow_fields = ('previous', 'low', 'delta', 'fine_us', 'at_ms', 'ipsr', 'mask', 'fine0', 'fine1', 'coarse0', 'coarse1', 'errors')
def pattern(prefix, fields):
    return re.compile(re.escape(prefix) + ' '.join(re.escape(key) + r'=(\d+)' for key in fields))
clock_re = pattern('CLOCK mode=pair24 ', clock_fields)
shadow_re = pattern('SHADOW ', shadow_fields)
clocks, shadows, malformed = [], [], []
for line in current.splitlines():
    if line.startswith('CLOCK '):
        match = clock_re.fullmatch(line)
        if match:
            clocks.append(dict(zip(clock_fields, map(int, match.groups()))))
        else:
            malformed.append(line)
    elif line.startswith('SHADOW '):
        match = shadow_re.fullmatch(line)
        if match:
            shadows.append(dict(zip(shadow_fields, map(int, match.groups()))))
        else:
            malformed.append(line)
identity_ok = source_id == a.source_id
health = bool(clocks) and identity_ok and not malformed and all(
    x['valid'] == 1 and x['errors'] == 0 and x['nested'] == 0 and x['gap_ms'] <= 60000
    for x in clocks)
result = {
    'source': str(path.resolve()), 'log_sha256': hashlib.sha256(raw).hexdigest(),
    'expected_source_id': a.source_id, 'observed_source_id': source_id,
    'identity_ok': identity_ok, 'clock_reports': clocks, 'first_shadow_reports': shadows,
    'malformed_clock_or_shadow_lines': malformed, 'internal_health_pass': health,
    'independent_vcd_validation_required': True,
    'interpretation': ('Complete records and source identity are required. Truncated lines that lose their '
                       'prefix may be undetectable. Internal 2ms consistency does not prove microsecond '
                       'accuracy; compare matched GPIO and UART VCD independently.'),
}
pathlib.Path(a.output).write_text(json.dumps(result, indent=2))
print('identity_ok', identity_ok, 'internal_health_pass', health, 'independent VCD still required')
