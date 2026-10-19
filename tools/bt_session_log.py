"""Conservative interpretation of one existing-session probe, not a radio trace."""
import argparse
import json
import re
from pathlib import Path


def assess(text):
    starts = list(re.finditer(r'^connected-bt revision=\d+ .*$', text, re.M))
    result = {'complete': False, 'l2cap_control': 'UNKNOWN',
              'l2cap_interrupt': 'UNKNOWN', 'global_pad_registration': 'UNKNOWN',
              'pad_control_validated': False, 'bt_delivery': 'UNOBSERVED'}
    if not starts:
        return result
    run = text[starts[-1].start():]
    result['complete'] = bool(re.search(r'^connected-bt complete=-?\d+;', run, re.M)
                              and re.search(r'^probe exit=-?\d+\s*$', run, re.M))
    if not result['complete']:
        return result
    summary = re.search(r'^BT summary callbacks=(\d+) events=(\d+) parsed=(\d+) invalid=(\d+)', run, re.M)
    if summary:
        result['bt_counts'] = dict(zip(('callbacks', 'events', 'parsed', 'invalid'),
                                      map(int, summary.groups())))
    counts = re.search(r'^BT connected summary metadata_matches=(\d+) format31_candidates=(\d+) common_unattributed=(\d+)', run, re.M)
    if counts:
        matched, candidates, common = map(int, counts.groups())
        if matched:
            result['bt_delivery'] = 'PARSED_WITH_MATCHING_CALLBACK_METADATA'
        elif candidates:
            result['bt_delivery'] = 'VALID_FORMAT_31_WITHOUT_TARGET_IDENTITY'
        elif common:
            result['bt_delivery'] = 'COMMON_FORMAT_WITHOUT_TARGET_IDENTITY'
    pads = [dict(zip(('slot', 'reads', 'errors', 'connected_reads', 'idle_x', 'target_x', 'control_l2'), map(int, m)))
            for m in re.findall(r'^PAD summary user_slot=(\d+) reads=(\d+) errors=(\d+) connected_reads=(\d+) idle_X=(\d+) DualSense_phase_X=(\d+) DS4_control_L2=(\d+)', run, re.M)]
    result['pad_slots'] = pads
    validated = [p for p in pads if p['errors'] == 0 and p['connected_reads'] > 0
                 and p['control_l2'] > 0 and p['idle_x'] == 0]
    result['pad_control_validated'] = bool(validated)
    if validated:
        result['pad_observation'] = ('X_PRESENT_DURING_TARGET_PHASE_IDENTITY_UNPROVEN'
                                     if any(p['target_x'] for p in validated)
                                     else 'NO_X_IN_VALIDATED_CLIENT_VIEW')
    else:
        result['pad_observation'] = 'UNTESTED_OR_CONTROL_FAILED'
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    print(json.dumps(assess(args.log.read_text(encoding='utf-8', errors='replace')), indent=2))
