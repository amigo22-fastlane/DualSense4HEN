"""Prepare offline, or run USB-owner ONLY after a separately approved USB window.

Default commands never install PKG, alter pairing or permissions, or retry an ELF
send. Approval flags record the user's decision; they do not grant permission.
"""
import argparse
import contextlib
from datetime import datetime, timezone
import ftplib
import hashlib
import io
import importlib.util
import json
from pathlib import Path
import socket
import time

from probe_log import appended_log
from usb_owner_log import assess_usb_owner

ROOT = Path(__file__).resolve().parents[1]
ELF = ROOT / 'dist/research/dualsense4hen-usb-owner.elf'
EXPECTED_SHA256 = '2b1c221e8577db8deb311c39addae7e1994aac8f2af4ed2ea8e09dacf4c5b212'
REMOTE_LOG = '/data/dualsense4hen-probe.log'
REMOTE_STOP = '/data/dualsense4hen.stop'
MAX_LOG = 4 * 1024 * 1024
_spec = importlib.util.spec_from_file_location('d4h_elf_validation', ROOT / 'tools/check-elf.py')
_validator = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_validator)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare(path=ELF):
    data = path.read_bytes()
    sha = digest(data)
    if sha != EXPECTED_SHA256:
        raise ValueError('ELF differs from the reviewed USB-owner artifact; refusing')
    with contextlib.redirect_stdout(io.StringIO()) as validation:
        _validator.check(path)
    return data, {'elf': path.name, 'sha256': sha, 'bytes': len(data),
                  'structural_validation': validation.getvalue().strip().replace(str(path), path.name),
                  'target': 'PS4 12.52; runtime library/profile guards required',
                  'hardware_execution': 'NOT_PERFORMED_BY_PREPARE',
                  'input_reports': 'NOT_CAPTURED_BY_THIS_PROBE',
                  'ready': 'FOR_SEPARATE_OWNER_APPROVAL_ONLY'}


def read_remote(ftp, path, missing_ok=False):
    if path not in (REMOTE_LOG, REMOTE_STOP):
        raise ValueError('Remote file outside diagnostic scope')
    cap = MAX_LOG if path == REMOTE_LOG else 64
    data = bytearray()

    def receive(block):
        if len(block) > cap - len(data):
            raise ValueError('Remote diagnostic file exceeds fixed limit')
        data.extend(block)

    try:
        ftp.retrbinary('RETR ' + path, receive)
    except ftplib.error_perm as error:
        message = str(error).lower()
        # A generic 550 could be permission denied: do not equate it with absent.
        if missing_ok and message.startswith('550') and any(
                word in message for word in ('not found', 'no such file', 'does not exist')):
            return None
        if (missing_ok and message.strip() == '550 requested action not taken. file unavailable.'
                and not listed_in_data(ftp, path)):
            return None  # Complete parent-name listing, not the ambiguous 550, proves absence.
        raise
    return bytes(data)


def listed_in_data(ftp, path):
    """Only names, discard unrelated entries; bounded fallback for GoldHEN 550."""
    if path not in (REMOTE_LOG, REMOTE_STOP):
        raise ValueError('Metadata target outside diagnostic scope')
    wanted = path.rsplit('/', 1)[1]
    found, count = False, 0

    def receive(line):
        nonlocal found, count
        count += 1
        if count > 4096 or len(line) > 1024:
            raise ValueError('Parent metadata listing exceeds fixed limit')
        if line.rstrip('/').rsplit('/', 1)[-1] == wanted:
            found = True

    ftp.retrlines('NLST /data', receive)
    return found


def connect_ftp(host):
    ftp = ftplib.FTP()
    try:
        ftp.connect(host, 2121, timeout=8)
        ftp.login('anonymous', '')
        return ftp
    except Exception:
        ftp.close()
        raise


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def collect(directory, host=None, wait=0):
    manifest = json.loads((directory / 'manifest.json').read_text(encoding='utf-8'))
    if manifest['sha256'] != EXPECTED_SHA256:
        raise ValueError('Manifest does not identify the reviewed artifact')
    before = (directory / 'baseline.log').read_bytes()
    if digest(before) != manifest['baseline_sha256']:
        raise ValueError('Baseline integrity mismatch')
    endpoint = host or manifest['host']
    deadline = time.monotonic() + wait
    while True:
        with connect_ftp(endpoint) as ftp:
            after = read_remote(ftp, REMOTE_LOG, missing_ok=True)
        if after is None:
            result = {'status': 'LOG_MISSING_AFTER_BASELINE' if before else 'WAITING_FOR_LOG',
                      'complete': False, 'physical_input_proven': False}
            write_json(directory / 'analysis.json', result)
            if before or time.monotonic() >= deadline:
                return result
            time.sleep(min(1, max(0, deadline - time.monotonic())))
            continue
        (directory / 'cumulative.log').write_bytes(after)
        try:
            delta = appended_log(before, after)
        except ValueError:
            result = {'status': 'LOG_REPLACED_OR_TRUNCATED', 'complete': False,
                      'physical_input_proven': False}
            write_json(directory / 'analysis.json', result)
            return result
        (directory / 'new.log').write_bytes(delta)
        result = assess_usb_owner(delta)
        result.update(new_log_sha256=digest(delta), new_log_bytes=len(delta),
                      collected_at_utc=datetime.now(timezone.utc).isoformat())
        write_json(directory / 'analysis.json', result)
        if result['complete'] or time.monotonic() >= deadline:
            return result
        time.sleep(min(1, max(0, deadline - time.monotonic())))


def require_approval(args):
    if not args.bt_finished or not args.usb_approved:
        raise ValueError('Run requires prior owner approval, --bt-finished AND --usb-approved')


def run(args):
    require_approval(args)  # Before any file creation or network connection.
    data, manifest = prepare()
    args.directory.mkdir(parents=True, exist_ok=False)
    manifest.update(host=args.host, bt_observation_finished=True, usb_separately_approved=True,
                    created_at_utc=datetime.now(timezone.utc).isoformat(),
                    send_attempted=False, tcp_send_completed=False)
    write_json(args.directory / 'manifest.json', manifest)
    try:
        with connect_ftp(args.host) as ftp:
            if read_remote(ftp, REMOTE_STOP, missing_ok=True) is not None:
                raise ValueError('Stop marker exists; refusing run, marker will not be removed')
            before = read_remote(ftp, REMOTE_LOG, missing_ok=True)
        before = b'' if before is None else before
        (args.directory / 'baseline.log').write_bytes(before)
        manifest.update(baseline_sha256=digest(before), baseline_bytes=len(before),
                        send_attempted=True, send_started_at_utc=datetime.now(timezone.utc).isoformat())
        write_json(args.directory / 'manifest.json', manifest)
        # Exactly one send attempt. A failed send can still have started a payload.
        with socket.create_connection((args.host, 9090), timeout=8) as loader:
            loader.sendall(data)
            loader.shutdown(socket.SHUT_WR)
        manifest['tcp_send_completed'] = True
        write_json(args.directory / 'manifest.json', manifest)
        result = collect(args.directory, wait=30)
        print(json.dumps(result, indent=2))
        return (0 if result['status'] in ('DRIVER_NAME_REPORTED', 'EMPTY_DRIVER_NAME') else
                3 if result['complete'] else 2)
    except Exception as error:
        manifest['host_error'] = str(error)
        write_json(args.directory / 'manifest.json', manifest)
        raise  # Never chmod, clear a stop marker, or resend to recover.


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('prepare', help='Offline artifact verification; no PS4 connection')
    execute = commands.add_parser('run', help='ONLY after separately approved USB window')
    execute.add_argument('--host', required=True)
    execute.add_argument('--directory', type=Path, required=True)
    execute.add_argument('--bt-finished', action='store_true')
    execute.add_argument('--usb-approved', action='store_true')
    fetch = commands.add_parser('collect', help='Fetch log for an existing run; never send ELF')
    fetch.add_argument('directory', type=Path)
    fetch.add_argument('--host')
    analyze = commands.add_parser('analyze', help='Offline interpretation of ONE new.log')
    analyze.add_argument('log', type=Path)
    args = parser.parse_args(argv)
    if args.command == 'prepare':
        print(json.dumps(prepare()[1], indent=2))
    elif args.command == 'run':
        return run(args)
    elif args.command == 'collect':
        print(json.dumps(collect(args.directory, args.host), indent=2))
    else:
        print(json.dumps(assess_usb_owner(args.log.read_bytes()), indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
