/* SPDX-License-Identifier: GPL-3.0-only */
#include "ds4hen/usb_owner.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,opened,closed,queried;
static int mode,error_slot;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"OWNER FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static int open_node(const char *p,int flags,...)
{
    CHECK(!strcmp(p,"/dev/ugen0.2") && flags==O_RDONLY); ++opened;
    CHECK(error_slot==0); if (mode==1) { error_slot=13; return -1; }
    return mode==7?0:9;
}
static int close_node(int fd) { CHECK(fd==(mode==7?0:9)); ++closed; return mode==5?-1:0; }
static int request(int fd,unsigned long cmd,void *arg)
{
    CHECK(fd==(mode==7?0:9) && error_slot==0); ++queried;
    if (queried==1) {
        CHECK(cmd==D4H_USB_DEVICE_DESC);
        if (mode==6) { error_slot=5; return -1; }
        uint8_t *d=arg; d[0]=18; d[1]=1; d[8]=0x4c; d[9]=5; d[10]=mode==2?0xc4:0xe6; d[11]=0x0c; return 0;
    }
    if (queried==2) {
        CHECK(cmd==D4H_USB_DRIVER_ACTIVE && *(int *)arg==3);
        if (mode==3) { error_slot=6; return -1; } return 0;
    }
    CHECK(queried==3 && cmd==D4H_USB_IFACE_NAME);
    /* Inspect only fields independently verified in the native wrapper. */
    uint8_t *p=arg; void *data=0; uint16_t size=0;
    memcpy(&data,p,8); memcpy(&size,p+10,2); CHECK(size==128 && p[18]==3);
    if (mode==4) { error_slot=1; return -1; }
    if (mode==8) memset(data,1,size);
    else strcpy(data,"uhid0: <synthetic test driver>");
    return 0;
}
int main(void)
{
    d4h_usb_owner_api api={open_node,request,close_node,&error_slot};
    d4h_usb_owner_result r;
    for (mode=0;mode<=8;++mode) {
        opened=closed=queried=0; error_slot=99;
        int ret=d4h_usb_owner_query(&api,0,2,&r);
        CHECK(opened==1);
        if (mode==1) { CHECK(ret==-2 && r.open_errno==13 && !closed && !queried); continue; }
        CHECK(closed==1);
        if (mode==2 || mode==6) { CHECK(ret==-3 && queried==1); continue; }
        CHECK(queried==3);
        if (mode==3) CHECK(ret==-4 && r.active_errno==6);
        else if (mode==4) CHECK(ret==-4 && r.name_errno==1 && !r.driver[0]);
        else if (mode==5) CHECK(ret==-5 && r.close_result==-1);
        else CHECK(ret==0 && r.vendor==0x054c && r.product==0x0ce6);
        if (mode==8) CHECK(r.driver[0]=='?' && r.driver[127]==0);
    }
    opened=0; CHECK(d4h_usb_owner_query(&api,0,0,&r)==-1 && !opened);
    CHECK(d4h_usb_owner_query(&api,256,2,&r)==-1 && !opened);
    CHECK(d4h_usb_owner_query(&api,0,128,&r)==-1 && !opened);
    CHECK(d4h_usb_owner_query(NULL,0,2,&r)==-1);
    printf("PASS USB ownership: %u checks; identity recheck, RO queries, no state changes\n",checks);
    return 0;
}
