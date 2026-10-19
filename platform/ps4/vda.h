/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DS4HEN_VDA_H
#define DS4HEN_VDA_H
#include "ds4hen/vpad.h"
/* Prototypes inferred by Control4Free/SplashDown, not Sony public headers. */
typedef struct {
    int32_t (*init)(void);
    int32_t (*privilege)(int32_t);
    int32_t (*add)(void *,int32_t);
    int32_t (*insert)(int32_t,const void *);
    int32_t (*remove)(int32_t);
} d4h_vda_api;
typedef struct { d4h_vda_api api; int initialized; } d4h_vda_backend;
/* Caller must load libSceMbus first and resolve ALL pointers beforehand.
 * Caller supplies any necessary authority; this module changes no credentials. */
int d4h_vda_init(d4h_vda_backend *,d4h_vda_api);
/* Status is NEVER treated as an ID. Caller must recover a proven fresh ID. */
int32_t d4h_vda_add(d4h_vda_backend *,int32_t user);
d4h_vpad_ops d4h_vda_ops(d4h_vda_backend *);
#endif
