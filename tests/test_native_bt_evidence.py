import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from native_bt_evidence import native_evidence


class NativeBtEvidenceTests(unittest.TestCase):
    def test_native_model_is_not_dualsense_identity_or_vda_ownership(self):
        log = ('<118>#LOGIN MGR# Receive Event : SCE_MBUS_EVENT_DEVICE_ADDED [DeviceId:0x50300][type:1][subType:2]\n'
               '<118> [o][0x0000000000050300][0x12345678][DUALSHOCK4]\n')
        r = native_evidence(log)
        self.assertEqual(r['devices']['0x50300']['models'], ['DUALSHOCK4'])
        self.assertEqual(r['virtual_device_ownership'], 'NOT_PROVEN')
        self.assertEqual(r['dualsense_identity'], 'UNKNOWN')
        self.assertEqual(r['l2cap_interrupt'], 'UNKNOWN')
        self.assertNotIn('12345678', str(r))

    def test_mirrored_or_synthetic_log_line_cannot_claim_native_event(self):
        r = native_evidence('[c4f] #LOGIN MGR# Receive Event : SCE_MBUS_EVENT_DEVICE_ADDED [DeviceId:0x123][subType:2]')
        self.assertEqual(r['devices'], {})

    def test_removed_is_history_not_current_connection(self):
        r = native_evidence('#LOGIN MGR# Receive Event : SCE_MBUS_EVENT_DEVICE_REMOVED [DeviceId:0x123][SubType:2]')
        self.assertEqual(r['scope'], 'HISTORICAL_LOG_NOT_CURRENT_INVENTORY')
        self.assertEqual(r['devices']['0x123']['events'], ['REMOVED'])


if __name__ == '__main__':
    unittest.main()
