"""Bounded GoldHEN klog capture. Keep full system logs local; print D4H only."""
import argparse
import socket
import time
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--host', required=True)
p.add_argument('--port', type=int, default=3232)
p.add_argument('--seconds', type=float, default=12)
p.add_argument('--output', type=Path, default=Path('build/hardware/probe.klog'))
args = p.parse_args()
if not 0 < args.seconds <= 45:
    p.error('--seconds must be 0..45')
data = bytearray()
with socket.create_connection((args.host, args.port), timeout=8) as sock:
    sock.settimeout(0.5)
    print('Klog connected; bounded capture started.', flush=True)
    deadline = time.monotonic()+args.seconds
    while time.monotonic() < deadline:
        try:
            part = sock.recv(65536)
        except socket.timeout:
            continue
        if not part:
            break
        data.extend(part)
        if len(data) > 1024*1024:
            del data[:-1024*1024]
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_bytes(data)
lines = [line for line in data.decode('utf-8', errors='replace').splitlines() if '[d4h]' in line]
print('\n'.join(lines) if lines else 'No [d4h] boot markers observed.')
print(f'{len(data)} bytes captured locally to {args.output}; no full system log published.')
