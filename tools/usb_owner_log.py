"""Interpret ONE newly appended USB-owner transaction, never a HID capture.

Synthetic unit log fixtures verify this interpreter, not PS4 compatibility.
"""
import re

BOOT = 'DualSense4HEN diagnostic: userspace bootstrap, no kernel patches'
START = 'USB owner revision='


def assess_usb_owner(log):
    text = log.decode('utf-8', errors='replace') if isinstance(log, bytes) else log
    result = {'status': 'NO_NEW_RUN', 'complete': False,
              'binding': 'UNKNOWN', 'input_capture': 'NOT_ATTEMPTED',
              'physical_input_proven': False, 'cause_0x802400ff': 'NOT_DETERMINED'}
    if not text.strip():
        return result
    if (text.count(BOOT) != 1 or text.count(START) > 1 or
            text.count('bootstrap relocated; resolving imports') > 1):
        result['status'] = 'AMBIGUOUS_OR_UNFRAMED_LOG'
        return result
    run = text[text.index(BOOT):]
    exits = re.findall(r'^probe exit=(-?\d+)\r?$', run, re.M)
    if len(exits) != 1:
        result['status'] = 'INCOMPLETE_OR_AMBIGUOUS_EXIT'
        return result
    exit_code = int(exits[0])
    result['exit'] = exit_code
    # Runtime logs exit before unloading its owned modules: wait for cleanup too.
    loads = re.findall(r'^load /system/common/lib/libSceUsbd\.sprx handle=0x([0-9a-f]+) start_result=0x([0-9a-f]+)\r?$', run, re.M)
    if len(loads) > 1:
        result['status'] = 'AMBIGUOUS_MODULE_LIFECYCLE'
        return result
    if loads:
        handle = int(loads[0][0], 16)
        if handle < 0x80000000:
            pattern = rf'^unload owned module=0x{handle:x} ret=0x([0-9a-f]+) result=0x([0-9a-f]+)\r?$'
            unload = list(re.finditer(pattern, run, re.M))
            exit_position = run.index('probe exit=')
            if len(unload) != 1 or unload[0].start() < exit_position:
                result['status'] = 'WAITING_FOR_MODULE_CLEANUP'
                return result
            if any(int(v, 16) != 0 for v in unload[0].groups()):
                result['status'] = 'MODULE_CLEANUP_FAILED'
                return result
    result['complete'] = True
    if START not in run:
        result['status'] = 'REFUSED_BEFORE_USB_START'
        return result
    starts = re.findall(r'^USB owner revision=(\d+) firmware=0x([0-9a-f]+);.*$', run, re.M)
    if len(starts) != 1 or starts[0][0] != '1' or int(starts[0][1], 16) >> 16 != 0x1252:
        result['status'] = 'UNSUPPORTED_RUN_PROFILE'
        return result
    result['firmware_raw'] = f'0x{int(starts[0][1], 16):08x}'
    refused = {-12: 'RESIDENT_OR_UNAVAILABLE_MODULE', -13: 'LIBRARY_PROFILE_REFUSED',
               -14: 'IMPORTS_UNAVAILABLE', -15: 'GETTER_ABI_REFUSED',
               -16: 'INITIALIZATION_FAILED'}
    if exit_code in refused:
        result['status'] = ('INCONSISTENT_GUARD_RESULT' if 'USB owner count=' in run
                            else refused[exit_code])
        return result
    if 'USB owner ABI profile verified: 65536-byte 12.52 RX, CRC(A1)=0x3efeb3ed' not in run:
        result['status'] = 'PROFILE_EVIDENCE_MISSING'
        return result
    completions = re.findall(r'^USB owner complete=(-?\d+);.*$', run, re.M)
    if len(completions) != 1 or int(completions[0]) != exit_code:
        result['status'] = 'MISSING_OR_INCONSISTENT_SUMMARY'
        return result
    counts = re.findall(r'^USB owner count=(-?\d+)\r?$', run, re.M)
    if len(counts) != 1:
        result['status'] = 'ENUMERATION_EVIDENCE_MISSING'
        return result
    count = int(counts[0])
    result['enumerated_count'] = count
    identified = re.findall(r'^USB owner identified 054c:0ce6 bus=(\d+) address=(\d+) iface=3\r?$', run, re.M)
    if not identified:
        if exit_code != -17:
            result['status'] = 'INCONSISTENT_TARGET_RESULT'
        else:
            result['status'] = ('ENUMERATION_FAILED' if count < 0 else
                                'ENUMERATION_LIMIT_REFUSED' if count > 256 else
                                'ENUMERATION_EMPTY' if count == 0 else 'TARGET_NOT_IDENTIFIED')
        return result
    if len(identified) != 1 or not 0 < count <= 256:
        result['status'] = 'AMBIGUOUS_TARGET'
        return result
    result['bus'], result['address'] = map(int, identified[0])
    identity = re.findall(r'^USB owner open=(-?\d+) errno=(-?\d+) identity_ret=(-?\d+) errno=(-?\d+) vid=([0-9a-f]+) pid=([0-9a-f]+)\r?$', run, re.M)
    metadata = re.findall(r'^USB owner ACTIVE raw=(-?\d+) errno=(-?\d+) NAME raw=(-?\d+) errno=(-?\d+) driver=(.*?) close=(-?\d+) result=(-?\d+)\r?$', run, re.M)
    if len(identity) != 1 or len(metadata) != 1:
        result['status'] = 'QUERY_EVIDENCE_MISSING'
        return result
    fd, oe, ir, ie = map(int, identity[0][:4])
    vid, pid = (int(v, 16) for v in identity[0][4:])
    ar, ae, nr, ne = map(int, metadata[0][:4])
    driver = metadata[0][4]
    close, summary = map(int, metadata[0][5:])
    result.update(open_result=fd, open_errno=oe, identity_result=ir, identity_errno=ie,
                  vendor=f'{vid:04x}', product=f'{pid:04x}', active_result=ar, active_errno=ae,
                  name_result=nr, name_errno=ne, driver=driver, close_result=close)
    if summary != exit_code:
        result['status'] = 'INCONSISTENT_QUERY_RESULT'
    elif exit_code == -2 and fd < 0:
        result['status'] = 'NODE_OPEN_REFUSED'
    elif exit_code == -3:
        result['status'] = 'IDENTITY_QUERY_FAILED' if ir else 'IDENTITY_CHANGED_OR_INVALID'
    elif fd < 0 or ir or (vid, pid) != (0x054c, 0x0ce6):
        result['status'] = 'INVALID_IDENTITY_EVIDENCE'
    elif exit_code not in (0, -4, -5):
        result['status'] = 'UNCLASSIFIED_FAILURE'
    else:
        if nr == 0:
            result['binding'] = 'DRIVER_NAME_REPORTED' if driver else 'EMPTY_NAME_NOT_PROOF_OF_NO_DRIVER'
        if close or exit_code == -5:
            result['status'] = 'NODE_CLOSE_FAILED'
        elif ar or nr or exit_code == -4:
            result['status'] = 'METADATA_QUERY_REFUSED'
        else:
            result['status'] = 'DRIVER_NAME_REPORTED' if driver else 'EMPTY_DRIVER_NAME'
    return result
