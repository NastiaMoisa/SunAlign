import hashlib,json,tempfile,unittest
from pathlib import Path
from analyze_cpu_profile import summarize
class ProfileCheck(unittest.TestCase):
 def test_vcd_window_excludes_calibration_and_uses_clipped_high_time(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td);v=p/'x.vcd';w=p/'wire.json';parts=['$timescale 1ns $end\n','$var wire 1 x D6 $end\n','#0\n0x\n']
   for i in range(40):
    a=1000+i*200;parts.extend([f'#{a}\n1x\n',f'#{a+100}\n0x\n'])
   for i in range(12):
    a=2_500_000_000+i*1_000_000_000;parts.extend([f'#{a}\n1x\n',f'#{a+1_000_000}\n0x\n'])
   parts.append('#15000000000\n');v.write_text(''.join(parts));w.write_text(json.dumps({'sha256':hashlib.sha256(v.read_bytes()).hexdigest(),'framing_errors':0,'lines':[{'text':'BOOT SunAlign lab3 src-b16af10ff4b0 systick_1ms','wire_ns':1_000_000_000}],'accepted_panel_timestamps':[{'mcu_ms':2500,'wire_ns':2_000_000_000},{'mcu_ms':14500,'wire_ns':14_000_000_000}]}))
   r=summarize(v,w,2500,14500);self.assertEqual(r['empty_marker_pulses']['n'],40);self.assertEqual(r['scope_intervals']['n'],12);self.assertAlmostEqual(r['foreground_fraction']['avg_percent'],0.1);self.assertAlmostEqual(r['scope_intervals']['avg_us'],1000)
 def test_rejects_baseline_firmware_boot(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td);v=p/'x';w=p/'w';v.write_text('x');w.write_text(json.dumps({'sha256':hashlib.sha256(v.read_bytes()).hexdigest(),'framing_errors':0,'lines':[{'text':'BOOT SunAlign lab3 src-af2d5aa374d1 systick_1ms','wire_ns':1}],'accepted_panel_timestamps':[]}))
   with self.assertRaisesRegex(ValueError,'diagnostic BOOT'):summarize(v,w,2500,14500)
 def test_rejects_hash_mismatch(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td);v=p/'x';w=p/'w';v.write_text('x');w.write_text(json.dumps({'sha256':'bad','framing_errors':0}));
   with self.assertRaises(ValueError):summarize(v,w,2500,14500)
if __name__=='__main__':unittest.main()
