"""Extract limited native input evidence from a local klog, without device attribution.
A cumulative klog is historical. No DeviceId is ever adopted as a VDA handle.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path


def native_evidence(text):
    devices = {}
    for line in text.splitlines():
        if re.search(r'\[(?:d4h|c4f)\]', line, re.I):
            continue
        event = re.search(r'#LOGIN MGR# Receive Event : SCE_MBUS_EVENT_DEVICE_(ADDED|REMOVED|STATUS_UPDATED|OWNER_CHANGED) \[DeviceId:0x([0-9a-fA-F]{1,16})\]', line)
        if event:
            label, value = event.groups()
            key = f'0x{int(value, 16):x}'
            item = devices.setdefault(key, {'events': [], 'models': [], 'subtypes': []})
            item['events'].append(label)
            subtype = re.search(r'\[subType:(\d+)\]', line, re.I)
            if subtype and int(subtype[1]) not in item['subtypes']:
                item['subtypes'].append(int(subtype[1]))
        model = re.search(r'\[o\]\[0x([0-9a-fA-F]{1,16})\]\[0x[0-9a-fA-F]+\]\[([^\]]{1,40})\]', line)
        if model:
            key = f'0x{int(model[1], 16):x}'
            item = devices.setdefault(key, {'events': [], 'models': [], 'subtypes': []})
            if model[2] not in item['models']:
                item['models'].append(model[2])
    return {'scope': 'HISTORICAL_LOG_NOT_CURRENT_INVENTORY', 'devices': devices,
            'dualsense_identity': 'UNKNOWN', 'l2cap_control': 'UNKNOWN',
            'l2cap_interrupt': 'UNKNOWN', 'virtual_device_ownership': 'NOT_PROVEN',
            'hid_report_reception': 'NOT_PROVEN_BY_MBUS_EVENTS'}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('log', type=Path)
    args = p.parse_args()
    raw = args.log.read_bytes()
    result = native_evidence(raw.decode('utf-8', errors='replace'))
    result['log_sha256'] = hashlib.sha256(raw).hexdigest()
    print(json.dumps(result, indent=2))
