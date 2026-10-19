"""Read only this project's app log over FTP; no console writes or launch."""
import argparse
from datetime import datetime, timezone
from ftplib import FTP, error_perm
import hashlib
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--host', required=True)
p.add_argument('--port', type=int, default=2121)
p.add_argument('--output', type=Path, default=Path('build/hardware/app-usb.log'))
args = p.parse_args()
paths = ['/data/dualsense4hen-app.log',
         '/mnt/sandbox/DSHN00001_000/data/dualsense4hen-app.log',
         '/mnt/sandbox/DSHN00001_000/download0/dualsense4hen-app.log']
found = False
with FTP() as ftp:
    ftp.connect(args.host, args.port, timeout=10)
    ftp.login()
    for remote in paths:
        data = bytearray()
        def append(part):
            if len(data) + len(part) > 2*1024*1024:
                raise ValueError('App log exceeds 2 MiB; refusing unbounded download')
            data.extend(part)
        try:
            ftp.retrbinary('RETR '+remote, append)
        except error_perm as error:
            print(remote+': '+str(error))
            continue
        text = data.decode('utf-8', errors='replace')
        if 'DualSense4HEN' not in text:
            print(remote+': missing project header; not accepted as diagnostic log')
            continue
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(data)
        args.output.with_suffix('.json').write_text(json.dumps({
            'collected_utc': datetime.now(timezone.utc).isoformat(),
            'remote_path': remote, 'sha256': hashlib.sha256(data).hexdigest(),
            'bytes': len(data), 'note': 'Collection alone does not prove USB input.'
        }, indent=2)+'\n', encoding='utf-8')
        print(text)
        print('Saved to '+str(args.output))
        found = True
        break
if not found:
    raise SystemExit('No app log found. Keep application open; report notification text.')
