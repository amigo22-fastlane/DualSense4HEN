/* SPDX-License-Identifier: GPL-3.0-only
 * Call sequence and 32-byte AddDevice block adapted from Control4Free
 * Copyright (c) 2026 MoHadiShibli; derived from seregonwar/SplashDown.
 * No credentials, kernel offsets or memory patches are implemented here. */
#include "vda.h"
typedef struct { int32_t size,user,unknown[6]; } add_param;
_Static_assert(sizeof(add_param)==32,"VDA AddDevice ABI");
int d4h_vda_init(d4h_vda_backend *b,d4h_vda_api api)
{
    if (!b) return -1;
    b->initialized=0;
    if (!api.init || !api.privilege || !api.add || !api.insert || !api.remove) return -1;
    b->api=api;
    int ret=api.init();
    if (ret) return ret;
    ret=api.privilege(1);
    if (!ret) b->initialized=1;
    return ret;
}
int32_t d4h_vda_add(d4h_vda_backend *b,int32_t user)
{
    if (!b || !b->initialized) return -1;
    add_param p={32,user,{0}};
    for (unsigned i=0;i<6;++i) p.unknown[i]=(int32_t)0xdeadbeefu;
    return b->api.add(&p,3);
}
static int insert(void *p,int32_t h,const d4h_ps4_pad_data *data)
{
    d4h_vda_backend *b=p;
    return b->initialized ? b->api.insert(h,data) : -1;
}
static int remove_pad(void *p,int32_t h)
{
    d4h_vda_backend *b=p;
    return b->initialized ? b->api.remove(h) : -1;
}
d4h_vpad_ops d4h_vda_ops(d4h_vda_backend *b)
{
    return (d4h_vpad_ops){b,insert,remove_pad};
}
