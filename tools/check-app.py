"""Check actual SFO, GP4 and optional finished PKG header identity."""
import argparse
from pathlib import Path
import xml.etree.ElementTree as ET
from app_metadata import check_ids, sfo_fields

p = argparse.ArgumentParser()
p.add_argument('--sfo', type=Path, required=True)
p.add_argument('--gp4', type=Path, required=True)
p.add_argument('--pkg', type=Path)
args = p.parse_args()
fields = sfo_fields(args.sfo.read_bytes())
content, title = fields['CONTENT_ID'], fields['TITLE_ID']
check_ids(content, title)
project = ET.parse(args.gp4).getroot()
if project.find('volume/package').get('content_id') != content:
    raise SystemExit('GP4 Content ID differs from SFO')
if fields.get('CATEGORY') != 'gd' or fields.get('APP_TYPE') != 1:
    raise SystemExit('Unexpected application category/type')
if args.pkg:
    with args.pkg.open('rb') as f:
        header = f.read(0x70)
    if header[:4] != b'\x7fCNT' or header[0x40:0x64].decode('ascii') != content:
        raise SystemExit('PKG header Content ID differs from SFO')
print(f'PASS application metadata: {title}, {content}; installation/launch still requires PS4 test')
