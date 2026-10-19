/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DS4HEN_CONTROLLER_H
#define DS4HEN_CONTROLLER_H
#include <stddef.h>
#include <stdint.h>

/* These are project types, not a claim about an Orbis Bluetooth ABI. */
typedef enum { D4H_USB, D4H_BLUETOOTH } d4h_transport;
enum {
    D4H_SHARE = 0x1, D4H_L3 = 0x2, D4H_R3 = 0x4, D4H_OPTIONS = 0x8,
    D4H_UP = 0x10, D4H_RIGHT = 0x20, D4H_DOWN = 0x40, D4H_LEFT = 0x80,
    D4H_L2 = 0x100, D4H_R2 = 0x200, D4H_L1 = 0x400, D4H_R1 = 0x800,
    D4H_TRIANGLE = 0x1000, D4H_CIRCLE = 0x2000, D4H_CROSS = 0x4000,
    D4H_SQUARE = 0x8000, D4H_PS = 0x10000, D4H_TOUCH_CLICK = 0x100000
};
typedef struct {
    uint16_t x, y;
    uint8_t id, active;
} d4h_touch;
typedef struct {
    uint32_t buttons;
    uint8_t lx, ly, rx, ry, l2, r2;
    uint8_t mute, has_touch, has_motion;
    d4h_touch touch[2];
    int16_t gyro[3], accel[3]; /* raw, uncalibrated; never passed as floats */
    uint32_t sensor_timestamp;
} d4h_state;
void d4h_neutral(d4h_state *state);
#endif
