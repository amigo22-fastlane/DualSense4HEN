/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DS4HEN_HID_H
#define DS4HEN_HID_H
#include "controller.h"
enum { D4H_OK = 0, D4H_E_ARGUMENT = -1, D4H_E_REPORT = -2,
       D4H_E_SIZE = -3, D4H_E_CRC = -4, D4H_E_HAT = -5 };
/* Input includes report ID, excludes HIDP 0xA1. Exact lengths required.
 * Bluetooth 0x31 CRC is mandatory. On error *out is unchanged. */
int d4h_parse(d4h_transport transport, const uint8_t *report, size_t size,
              d4h_state *out);
uint32_t d4h_bt_crc(const uint8_t *report, size_t size_without_crc);
#endif
