/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef D4H_BT_MONITOR_H
#define D4H_BT_MONITOR_H
#include "hid.h"
typedef struct {
    uint64_t device;
    uint16_t vendor,product;
    uint8_t type,status;
    uint16_t size;
    uint64_t reserved;
    uint8_t report[78],pad[2];
} d4h_bt_event;
enum { D4H_BT_NO_INPUT=1,D4H_BT_FORMAT_CANDIDATE=2,D4H_BT_COMMON_UNATTRIBUTED=3 };
/* 0: parser passed and event metadata matches VID/PID. This does not prove
 * channel PSMs, bonding, or a global scePad registration. Other nonnegative
 * classifications never identify the controller by report shape alone. */
int d4h_bt_inspect(const d4h_bt_event *,d4h_state *);
#endif
