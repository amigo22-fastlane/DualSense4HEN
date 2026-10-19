"""Do not mistake missing observer events for missing HID channels."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from bt_session_log import assess

START = 'connected-bt revision=1 firmware_status=0x0 firmware=0x12520001\n'
END = 'connected-bt complete=0; absence is not proof\nprobe exit=0\n'
BT = ('BT summary callbacks=0 events=0 parsed=0 invalid=0\n'
      'BT connected summary metadata_matches=0 format31_candidates=0 common_unattributed=0;\n')
PAD = 'PAD summary user_slot=0 reads=350 errors=0 connected_reads=350 idle_X=0 DualSense_phase_X=0 DS4_control_L2=25\n'


class BtSessionLogTests(unittest.TestCase):
    def test_empty_subscriber_unvalidated_pad(self):
        result = assess(START + BT + END)
        self.assertTrue(result['complete'])
        self.assertFalse(result['pad_control_validated'])
        self.assertEqual(result['l2cap_interrupt'], 'UNKNOWN')
        self.assertEqual(result['global_pad_registration'], 'UNKNOWN')

    def test_valid_control_only_bounds_client_view(self):
        result = assess(START + BT + PAD + END)
        self.assertTrue(result['pad_control_validated'])
        self.assertEqual(result['pad_observation'], 'NO_X_IN_VALIDATED_CLIENT_VIEW')
        self.assertEqual(result['l2cap_control'], 'UNKNOWN')

    def test_candidate_is_not_identity(self):
        result = assess(START + BT.replace('format31_candidates=0', 'format31_candidates=3') + END)
        self.assertEqual(result['bt_delivery'], 'VALID_FORMAT_31_WITHOUT_TARGET_IDENTITY')

    def test_target_metadata_does_not_prove_channels_or_pad(self):
        result = assess(START + BT.replace('metadata_matches=0', 'metadata_matches=2') + PAD + END)
        self.assertEqual(result['bt_delivery'], 'PARSED_WITH_MATCHING_CALLBACK_METADATA')
        self.assertEqual(result['l2cap_interrupt'], 'UNKNOWN')

    def test_incomplete_latest_run_never_borrows_old_success(self):
        result = assess(START + BT + PAD + END + START)
        self.assertFalse(result['complete'])
        self.assertFalse(result['pad_control_validated'])

    def test_read_errors_invalidate_control(self):
        self.assertFalse(assess(START + PAD.replace('errors=0', 'errors=1') + END)['pad_control_validated'])

    def test_x_correlation_does_not_prove_device_identity(self):
        result = assess(START + PAD.replace('DualSense_phase_X=0', 'DualSense_phase_X=7') + END)
        self.assertEqual(result['pad_observation'], 'X_PRESENT_DURING_TARGET_PHASE_IDENTITY_UNPROVEN')
        self.assertEqual(result['global_pad_registration'], 'UNKNOWN')


if __name__ == '__main__':
    unittest.main()
