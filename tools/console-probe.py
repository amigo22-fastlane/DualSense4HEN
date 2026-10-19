"""Send a diagnostic ELF to an explicitly supplied console; collect its log.
No dependencies. Uses GoldHEN's raw TCP ELF loader and FTP server.
Never autoruns, updates firmware, changes plugins.ini or uploads kernel patches.
"""
import argparse
import ftplib
import hashlib
import io
import socket
import time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--host', required=True)
parser.add_argument('--loader-port', type=int, default=9090)
parser.add_argument('--ftp-port', type=int, default=2121)
parser.add_argument('--elf', type=Path)
parser.add_argument('--fetch-only', action='store_true')
parser.add_argument('--stop', action='store_true')
parser.add_argument('--output', type=Path, default=Path('build/hardware/probe.log'))
parser.add_argument('--wait', type=float, default=5)
parser.add_argument('--print-lines', type=int, default=80)
args = parser.parse_args()
if not 0 <= args.wait <= 45:
    parser.error('--wait must be between 0 and 45 seconds')
if not (args.elf or args.fetch_only or args.stop):
    parser.error('specify --elf, --fetch-only, or --stop')

def ftp_connect():
    f = ftplib.FTP()
    f.connect(args.host, args.ftp_port, timeout=8)
    f.login('anonymous', '')
    return f

with ftp_connect() as ftp:
    if args.stop:
        ftp.storbinary('STOR /data/dualsense4hen.stop', io.BytesIO(b''))
        print('Stop marker written; probe will exit at the next bounded read.')
    if args.elf:
        # Do not clear a user's stop request automatically.
        try:
            ftp.retrbinary('RETR /data/dualsense4hen.stop', lambda _: None)
        except ftplib.error_perm as e:
            if not str(e).startswith('550'):
                raise
        else:
            raise SystemExit('Stop marker present; remove it intentionally before a new probe.')

if args.elf:
    data = args.elf.read_bytes()
    if data[:4] != b'\x7fELF':
        raise SystemExit('Input is not an ELF file')
    print(f'Sending {args.elf.name}: {len(data)} bytes, SHA256 {hashlib.sha256(data).hexdigest()}')
    with socket.create_connection((args.host, args.loader_port), timeout=8) as sock:
        sock.sendall(data)
        sock.shutdown(socket.SHUT_WR)
    print('ELF sent; a successful TCP send does not prove execution.')

if args.elf or args.fetch_only:
    time.sleep(args.wait)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    result = bytearray()
    with ftp_connect() as ftp:
        try:
            ftp.retrbinary('RETR /data/dualsense4hen-probe.log', result.extend)
        except ftplib.error_perm as e:
            raise SystemExit(f'No probe log available: {e}. Execution is unconfirmed.')
    args.output.write_bytes(result)
    lines = result.decode('utf-8', errors='replace').splitlines()
    print('\n'.join(lines[-max(1,args.print_lines):]))
    print(f'Full cumulative console log saved to {args.output}')
