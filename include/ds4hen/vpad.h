/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DS4HEN_VPAD_H
#define DS4HEN_VPAD_H
#include "ps4_pad.h"
/* Project backend interface. These names are NOT invented SDK exports.
 * adopt accepts only a backend-proven ID created by this instance. */
typedef struct {
    void *context;
    int (*insert)(void *, int32_t, const d4h_ps4_pad_data *);
    int (*remove)(void *, int32_t);
} d4h_vpad_ops;
typedef struct {
    d4h_vpad_ops ops;
    int32_t handle;
    uint8_t owned,count,has_input,neutral_sent;
    uint64_t last_input_us,last_stamp_us;
} d4h_vpad;
void d4h_vpad_init(d4h_vpad *, d4h_vpad_ops);
int d4h_vpad_adopt(d4h_vpad *, int32_t owned_handle);
int d4h_vpad_send(d4h_vpad *, const d4h_state *, uint64_t now_us);
int d4h_vpad_watchdog(d4h_vpad *, uint64_t now_us, uint64_t timeout_us);
/* Always neutralize before deletion; failed deletion preserves ownership. */
int d4h_vpad_stop(d4h_vpad *, uint64_t now_us);
#endif
