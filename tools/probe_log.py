"""Local-only extraction of newly appended diagnostic output."""


def appended_log(before: bytes, after: bytes) -> bytes:
    if not after.startswith(before):
        raise ValueError('Console log was replaced/truncated; cannot isolate this run.')
    return after[len(before):]


def usb_outcome(log: bytes) -> str:
    # Do not interpret an old cumulative exit=0 as evidence of a new USB run.
    start = log.rfind(b'USB probe revision=')
    if start < 0:
        return 'USB execution unconfirmed (no new USB start marker).'
    run = log[start:]
    if b'probe exit=' not in run:
        return 'USB run incomplete; fetch the log again after capture ends.'
    if b'probe exit=0\n' in run and b'USB summary good=' in run:
        return 'USB probe reports completion; inspect raw/parse/map/cleanup lines and confirm on console.'
    return 'USB run ended without full capture success; inspect enumeration and error/cleanup lines.'
