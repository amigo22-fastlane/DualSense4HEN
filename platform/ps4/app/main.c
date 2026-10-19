/* SPDX-License-Identifier: GPL-3.0-only
 * Standalone OpenOrbis application; no jailbreak/credential/kernel helpers. */
#include "ds4hen/usb_diagnostic.h"
#include <orbis/libkernel.h>
#include <stdio.h>
#include <string.h>

static unsigned received,x_presses;
static int previous_x;
static void notify(const char *message)
{
    OrbisNotificationRequest request;
    memset(&request,0,sizeof(request));
    request.type=NotificationRequest; request.targetId=-1;
    snprintf(request.message,sizeof(request.message),"%s",message);
    (void)sceKernelSendNotificationRequest(0,&request,sizeof(request),0);
}
static void input(const d4h_state *state,uint64_t timestamp)
{
    (void)timestamp;
    ++received;
    int x=(state->buttons&D4H_CROSS)!=0;
    if (x && !previous_x) {
        ++x_presses;
        /* Bounded notification count; the full input stream stays in the log. */
        if (x_presses<=5) notify("DualSense4HEN: real USB report - X pressed");
    }
    previous_x=x;
}
int main(void)
{
    notify("DualSense4HEN USB diagnostic " D4H_APP_VERSION ": starts in 5s; press X during capture");
    sceKernelUsleep(5000000);
    d4h_usb_input=input;
    int result=d4h_app_run();
    d4h_usb_input=0;
    char message[192];
    snprintf(message,sizeof(message),"DualSense4HEN " D4H_APP_VERSION ": result=%d reports=%u X_presses=%u; leave app open for log collection",
        result,received,x_presses);
    notify(message);
    /* Keep the sandbox mounted for FTP collection. The USB probe has already
     * released its resources. Close this application with the PS menu. */
    for (;;) sceKernelUsleep(1000000);
}
