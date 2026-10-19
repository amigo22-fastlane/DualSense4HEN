/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented byte decoder. Layout references: Sony's Linux
 * hid-playstation dualsense_input_report; nondebug/dualsense descriptors.
 * No operating system structures or packed pointer casts are used. */
#include "ds4hen/hid.h"
#include <string.h>

void d4h_neutral(d4h_state *s)
{
    memset(s, 0, sizeof(*s));
    s->lx = s->ly = s->rx = s->ry = 128;
}
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int16_t signed16(const uint8_t *p)
{
    int32_t v = (int32_t)p[0] | (int32_t)p[1] << 8;
    return (int16_t)(v >= 32768 ? v - 65536 : v);
}
static uint32_t crc_byte(uint32_t crc, uint8_t b)
{
    crc ^= b;
    for (unsigned i = 0; i < 8; ++i)
        crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    return crc;
}
uint32_t d4h_bt_crc(const uint8_t *p, size_t n)
{
    uint32_t crc = crc_byte(0xffffffffu, 0xa1);
    for (size_t i = 0; i < n; ++i) crc = crc_byte(crc, p[i]);
    return ~crc;
}
static int buttons(d4h_state *s, uint8_t a, uint8_t b, uint8_t c)
{
    static const uint32_t hats[9] = {
        D4H_UP, D4H_UP | D4H_RIGHT, D4H_RIGHT, D4H_RIGHT | D4H_DOWN,
        D4H_DOWN, D4H_DOWN | D4H_LEFT, D4H_LEFT, D4H_LEFT | D4H_UP, 0
    };
    static const uint32_t face[4] = {D4H_SQUARE,D4H_CROSS,D4H_CIRCLE,D4H_TRIANGLE};
    static const uint32_t other[8] = {
        D4H_L1,D4H_R1,D4H_L2,D4H_R2,D4H_SHARE,D4H_OPTIONS,D4H_L3,D4H_R3
    };
    if ((a & 15) > 8) return D4H_E_HAT;
    s->buttons = hats[a & 15];
    for (unsigned i = 0; i < 4; ++i) if (a & (16u << i)) s->buttons |= face[i];
    for (unsigned i = 0; i < 8; ++i) if (b & (1u << i)) s->buttons |= other[i];
    if (c & 1) s->buttons |= D4H_PS;
    if (c & 2) s->buttons |= D4H_TOUCH_CLICK;
    return D4H_OK;
}
int d4h_parse(d4h_transport t, const uint8_t *r, size_t n, d4h_state *out)
{
    const uint8_t *p;
    d4h_state s;
    int ret;
    if (!r || !out || (t != D4H_USB && t != D4H_BLUETOOTH)) return D4H_E_ARGUMENT;
    if (!n) return D4H_E_SIZE;
    d4h_neutral(&s);
    if (t == D4H_BLUETOOTH && r[0] == 1) {
        if (n != 10) return D4H_E_SIZE;
        s.lx=r[1]; s.ly=r[2]; s.rx=r[3]; s.ry=r[4]; s.l2=r[8]; s.r2=r[9];
        ret = buttons(&s,r[5],r[6],r[7]);
    } else {
        if (t == D4H_USB && r[0] == 1) {
            if (n != 64) return D4H_E_SIZE;
            p = r + 1;
        } else if (t == D4H_BLUETOOTH && r[0] == 0x31) {
            if (n != 78) return D4H_E_SIZE;
            if (d4h_bt_crc(r,74) != le32(r+74)) return D4H_E_CRC;
            p = r + 2;
        } else return D4H_E_REPORT;
        s.lx=p[0]; s.ly=p[1]; s.rx=p[2]; s.ry=p[3]; s.l2=p[4]; s.r2=p[5];
        ret = buttons(&s,p[7],p[8],p[9]);
        s.mute = (p[9] >> 2) & 1;
        s.has_touch = s.has_motion = 1;
        for (unsigned i = 0; i < 3; ++i) {
            s.gyro[i] = signed16(p + 15 + i*2);
            s.accel[i] = signed16(p + 21 + i*2);
        }
        s.sensor_timestamp = le32(p+27);
        for (unsigned i = 0; i < 2; ++i) {
            const uint8_t *q = p + 32 + i*4;
            s.touch[i].id = q[0] & 127;
            s.touch[i].active = (q[0] & 128) == 0;
            if (s.touch[i].active) {
                s.touch[i].x = (uint16_t)(q[1] | (q[2] & 15) << 8);
                s.touch[i].y = (uint16_t)((q[2] >> 4) | q[3] << 4);
            }
        }
    }
    if (ret) return ret;
    *out = s;
    return D4H_OK;
}
