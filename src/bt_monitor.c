/* SPDX-License-Identifier: GPL-3.0-only */
#include "ds4hen/bt_monitor.h"
_Static_assert(sizeof(d4h_bt_event)==104 && offsetof(d4h_bt_event,report)==24,"audited callback ABI");
int d4h_bt_inspect(const d4h_bt_event *event,d4h_state *out)
{
    if (!event || !out) return D4H_E_ARGUMENT;
    if (!event->size) return D4H_BT_NO_INPUT;
    if (event->size>sizeof(event->report)) return D4H_E_SIZE;
    int ret=d4h_parse(D4H_BLUETOOTH,event->report,event->size,out);
    if (ret) return ret;
    if (event->vendor==0x054c && event->product==0x0ce6) return 0;
    return event->report[0]==0x31?D4H_BT_FORMAT_CANDIDATE:D4H_BT_COMMON_UNATTRIBUTED;
}
