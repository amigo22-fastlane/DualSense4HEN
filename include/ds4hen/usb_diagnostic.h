/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef D4H_USB_DIAGNOSTIC_H
#define D4H_USB_DIAGNOSTIC_H
#include "controller.h"
#define D4H_APP_VERSION "1.03"
/* Called synchronously only after a real received report passes the parser.
 * A host test can inject the USB API, never fabricate console evidence. */
typedef void (*d4h_usb_input_fn)(const d4h_state *,uint64_t);
extern d4h_usb_input_fn d4h_usb_input;
int d4h_usb_probe(void);
int d4h_app_run(void);
#endif
