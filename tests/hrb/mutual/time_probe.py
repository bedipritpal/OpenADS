"""Run the protected worker, not a standalone Harbour VM, to prove HRB calls."""
import subprocess, struct, sys, datetime
worker, module = sys.argv[1:]
def field(data): return struct.pack('<I', len(data)) + data
p = subprocess.run([worker, module],input=b'A'+bytes(32)+field(b'time48')+field(b'worker-owned'),capture_output=True,timeout=5)
assert p.returncode == 0, (p.returncode,p.stdout,p.stderr)
text=p.stdout.decode('ascii')
parts=text.split('|')
assert len(parts)==5 and parts[0]=='TIME48-PASS', (text,p.stderr)
assert parts[1]=='2026-01-15 12:34:56.789' and parts[2]=='20260115123456789',text
local=datetime.datetime.strptime(parts[1],'%Y-%m-%d %H:%M:%S.%f')
utc=datetime.datetime.strptime(parts[3],'%Y-%m-%d %H:%M:%S.%f')
assert (local-utc).total_seconds()==int(parts[4]),text
assert not p.stderr,p.stderr
print(text)
print('Actual worker HRB hb_DateTime / hb_TSToUTC / hb_TSToStr / hb_StrToTS / hb_TToS / hb_SToT / hb_UTCOffset / hb_UTCToTS PASS')
