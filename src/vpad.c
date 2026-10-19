/* SPDX-License-Identifier: GPL-3.0-only */
#include "ds4hen/vpad.h"
#include <string.h>
void d4h_vpad_init(d4h_vpad *v,d4h_vpad_ops ops)
{
    memset(v,0,sizeof(*v)); v->ops=ops; v->handle=-1;
}
int d4h_vpad_adopt(d4h_vpad *v,int32_t handle)
{
    if (!v || v->owned || handle < 0 || !v->ops.insert || !v->ops.remove) return -1;
    v->handle=handle; v->owned=1; v->count=0; v->last_stamp_us=0;
    v->has_input=v->neutral_sent=0; return 0;
}
static int insert(d4h_vpad *v,const d4h_state *s,uint64_t now)
{
    d4h_ps4_pad_data data;
    uint64_t stamp=now > v->last_stamp_us ? now : v->last_stamp_us+1;
    int ret;
    if (v->last_stamp_us==UINT64_MAX) return -1;
    d4h_map_ps4(s,stamp,(uint8_t)(v->count+1),&data);
    ret=v->ops.insert(v->ops.context,v->handle,&data);
    if (!ret) { v->last_stamp_us=stamp; ++v->count; }
    return ret;
}
int d4h_vpad_send(d4h_vpad *v,const d4h_state *s,uint64_t now)
{
    int ret;
    if (!v || !s || !v->owned) return -1;
    ret=insert(v,s,now);
    if (!ret) { v->last_input_us=now; v->has_input=1; v->neutral_sent=0; }
    return ret;
}
int d4h_vpad_watchdog(d4h_vpad *v,uint64_t now,uint64_t timeout)
{
    d4h_state s;
    if (!v || !v->owned || !v->has_input || v->neutral_sent) return 0;
    if (now >= v->last_input_us && now-v->last_input_us < timeout) return 0;
    d4h_neutral(&s);
    int ret=insert(v,&s,now);
    if (!ret) v->neutral_sent=1;
    return ret;
}
int d4h_vpad_stop(d4h_vpad *v,uint64_t now)
{
    d4h_state s; int a,b;
    if (!v) return -1;
    if (!v->owned) return 0;
    d4h_neutral(&s); a=insert(v,&s,now);
    b=v->ops.remove(v->ops.context,v->handle);
    if (!b) { v->owned=0; v->handle=-1; v->has_input=0; }
    return b ? b : a;
}
