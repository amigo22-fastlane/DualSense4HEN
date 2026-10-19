/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef D4H_PAD_SNAPSHOT_H
#define D4H_PAD_SNAPSHOT_H
#include <orbis/_types/pad.h>
#include <stdint.h>
typedef struct {
    int32_t (*get_handle)(int32_t,uint32_t,uint32_t);
    int32_t (*open)(int32_t,int32_t,int32_t,void *);
    int32_t (*close)(int32_t);
    int32_t (*read_state)(int32_t,OrbisPadData *);
    int32_t (*info)(int32_t,OrbisPadInformation *);
} d4h_pad_read_api;
typedef struct {
    int32_t handle;
    int owned;
    unsigned reads,errors,connected,x_down[3],x_changes[3],l2_active[3];
    int seen_x,previous_x,previous_connected,previous_l2;
} d4h_pad_read_slot;
int d4h_pad_read_open(const d4h_pad_read_api *,d4h_pad_read_slot *,int32_t user);
void d4h_pad_read_tick(const d4h_pad_read_api *,d4h_pad_read_slot *,unsigned slot,unsigned phase);
int d4h_pad_read_close(const d4h_pad_read_api *,d4h_pad_read_slot *);
#endif
