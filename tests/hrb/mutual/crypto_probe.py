"""Verify actual HRB calls inside the protected auth worker, not merely linkage."""
import struct
import subprocess
import sys
worker, module = sys.argv[1:]
p = subprocess.run([worker, module], input=b'A' + bytes(32) + struct.pack('<I', 0) * 2,
                   capture_output=True, timeout=5)
assert p.returncode == 0, (p.returncode, p.stdout, p.stderr)
assert p.stdout == (b'CRYPTO-PASS|SHA256/384/512|HMAC256/384/512|binary/hex|'
                    b'random1..1024|invalid=NIL|MD5|BlowfishECB/CFB|purehelpers'), p.stdout
assert not p.stderr, p.stderr
print(p.stdout.decode('ascii'))
