/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef D4H_USB_OWNER_H
#define D4H_USB_OWNER_H
#include <stddef.h>
#include <stdint.h>
/* Numeric requests observed in the approved 12.52 Usbd RX, not copied
 * blindly from another OS. No detach, FS setup, transfer or power operation. */
#define D4H_USB_DEVICE_DESC 0x40125569ul
#define D4H_USB_DRIVER_ACTIVE 0x8004557cul
#define D4H_USB_IFACE_NAME 0xc0205579ul
typedef struct {
    int (*open)(const char *,int,...);
    int (*query)(int,unsigned long,void *);
    int (*close)(int);
    int *error;
} d4h_usb_owner_api;
typedef struct {
    int open_result,open_errno,identity_result,identity_errno;
    int active_result,active_errno,name_result,name_errno,close_result;
    uint16_t vendor,product;
    char driver[128];
} d4h_usb_owner_result;
/* Fixed interface 3 of identified 054c:0ce6. Only opens one known node RO.
 * 0 means queries completed, not that a driver exists or caused a failure. */
int d4h_usb_owner_query(const d4h_usb_owner_api *,unsigned,unsigned,d4h_usb_owner_result *);
#endif
