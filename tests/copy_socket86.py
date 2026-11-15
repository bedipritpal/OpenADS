#!/usr/bin/env python3
"""Actual TCP FileCopy regression using the built client and server."""
import argparse
import ctypes as C
import pathlib
import socket
import subprocess
import tempfile
import time


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--client', required=True)
    p.add_argument('--server', required=True)
    p.add_argument('--harbour-probe')
    p.add_argument('--harbour-only', action='store_true')
    args = p.parse_args()
    if not args.harbour_only:
        dll = C.CDLL(str(pathlib.Path(args.client).resolve()))
        U32, U16, H = C.c_uint32, C.c_uint16, C.c_uint32
        dll.AdsConnect60.argtypes = [C.c_char_p, U16, C.c_char_p, C.c_char_p, U32, C.POINTER(H)]
        dll.AdsConnect60.restype = U32
        dll.AdsCopyFile.argtypes = [H, C.c_char_p, C.c_char_p]
        dll.AdsCopyFile.restype = U32
        dll.AdsDisconnect.argtypes = [H]
        dll.AdsDisconnect.restype = U32
    with tempfile.TemporaryDirectory(prefix='openads-copy86-') as directory:
        root = pathlib.Path(directory)
        data = root / 'data'
        data.mkdir()
        body = bytes(range(256)) * 1027 + b'\x00end\xff'
        (data / 'copy-source.bin').write_bytes(body)
        with socket.socket() as s:
            s.bind(('127.0.0.1', 0))
            port = s.getsockname()[1]
        log = root / 'server.txt'
        with log.open('wb') as output:
            server = subprocess.Popen([str(pathlib.Path(args.server).resolve()), '--host', '127.0.0.1',
                                       '--port', str(port), '--data', str(data), '--enable_file_func'],
                                      stdout=output, stderr=subprocess.STDOUT, cwd=root)
            try:
                deadline = time.monotonic() + 20
                while True:
                    if server.poll() is not None:
                        raise RuntimeError('server exited: ' + log.read_text(errors='replace'))
                    try:
                        with socket.create_connection(('127.0.0.1', port), timeout=.3):
                            break
                    except OSError:
                        if time.monotonic() >= deadline:
                            raise
                        time.sleep(.1)
                uri = ('tcp://127.0.0.1:%d/' % port + data.as_posix()).encode()
                if not args.harbour_only:
                    h = H()
                    assert dll.AdsConnect60(uri, 2, None, None, 0, C.byref(h)) == 0
                    assert h.value
                    try:
                        assert dll.AdsCopyFile(h, b'copy-source.bin', b'copy-target.bin') == 0
                        assert (data / 'copy-target.bin').read_bytes() == body
                        (data / 'copy-target.bin').write_bytes(b'old content')
                        assert dll.AdsCopyFile(h, b'copy-source.bin', b'copy-target.bin') == 0
                        assert (data / 'copy-target.bin').read_bytes() == body
                        assert dll.AdsCopyFile(h, b'missing.bin', b'must-not-exist.bin') != 0
                        assert not (data / 'must-not-exist.bin').exists()
                        assert dll.AdsCopyFile(h, b'copy-source.bin', b'../outside.bin') != 0
                        assert not (root / 'outside.bin').exists()
                    finally:
                        dll.AdsDisconnect(h)
                if args.harbour_probe:
                    result = subprocess.run([str(pathlib.Path(args.harbour_probe).resolve()), uri.decode()],
                                            cwd=root, capture_output=True, text=True, timeout=30)
                    print(result.stdout, end='')
                    if result.returncode or 'COPY86-ACTUAL-TCP PASS' not in result.stdout:
                        raise RuntimeError('Harbour copy failed: ' + result.stdout + result.stderr)
                if not args.harbour_only:
                    print('COPY86-C-ABI-ACTUAL-TCP PASS bytes=%d' % len(body))
            finally:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait(timeout=10)


if __name__ == '__main__':
    main()
