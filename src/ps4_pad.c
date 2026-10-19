/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ds4hen/ps4_pad.h"
#include <string.h>
static uint16_t clamp_scale(uint16_t v, uint16_t from, uint16_t to)
{
    if (v > from) v = from;
    return (uint16_t)(((uint32_t)v * to + from/2u) / from);
}
void d4h_map_ps4(const d4h_state *s, uint64_t stamp, uint8_t count,
                  d4h_ps4_pad_data *p)
{
    memset(p,0,sizeof(*p));
    p->buttons=s->buttons; p->lx=s->lx; p->ly=s->ly; p->rx=s->rx; p->ry=s->ry;
    p->l2=s->l2; p->r2=s->r2; p->quat[3]=1.0f;
    p->connected=1; p->timestamp=stamp; p->count=count;
    /* DualSense 1920x1080 -> DS4 1920x942. Raw IMU has no DS4 calibration. */
    if (s->has_touch) for (unsigned i=0;i<2;++i) if (s->touch[i].active) {
        d4h_ps4_touch *t = &p->touch_data.touch[p->touch_data.fingers++];
        t->finger=s->touch[i].id;
        t->x=clamp_scale(s->touch[i].x,1919,1919);
        t->y=clamp_scale(s->touch[i].y,1079,941);
    }
}
