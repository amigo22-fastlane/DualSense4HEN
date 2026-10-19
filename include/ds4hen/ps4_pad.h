/* SPDX-License-Identifier: GPL-3.0-only
 * ScePadData layout adapted from MoHadiShibli/Control4Free (2026), GPL-3.0,
 * whose VDA code derives from seregonwar/SplashDown. See THIRD_PARTY.md.
 * Reverse engineered user-space ABI; not a public Sony SDK declaration. */
#ifndef DS4HEN_PS4_PAD_H
#define DS4HEN_PS4_PAD_H
#include "controller.h"
typedef struct { uint16_t x,y; uint8_t finger,pad[3]; } d4h_ps4_touch;
typedef struct {
    uint8_t fingers,pad1[3]; uint32_t pad2; d4h_ps4_touch touch[2];
} d4h_ps4_touch_data;
typedef struct {
    uint32_t buttons;
    uint8_t lx,ly,rx,ry,l2,r2; uint16_t padding;
    float quat[4],vel[3],accel[3];
    d4h_ps4_touch_data touch_data;
    uint8_t connected,align[3]; uint64_t timestamp;
    uint8_t ext[16],count,unknown[15];
} d4h_ps4_pad_data;
_Static_assert(sizeof(d4h_ps4_pad_data)==120, "ScePadData ABI size");
_Static_assert(offsetof(d4h_ps4_pad_data,touch_data)==52, "touch ABI");
_Static_assert(offsetof(d4h_ps4_pad_data,timestamp)==80, "timestamp ABI");
_Static_assert(offsetof(d4h_ps4_pad_data,count)==104, "counter ABI");
void d4h_map_ps4(const d4h_state *, uint64_t timestamp_us, uint8_t count,
                  d4h_ps4_pad_data *);
#endif
