/* SPDX-License-Identifier: GPL-3.0-only
 * Mock only the PS4 boundary: no device, driver or pairing is simulated as
 * hardware success. Verify passive mode's lifetime and lack of registration. */
#include "../platform/ps4/runtime.h"
#include "ds4hen/hid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
int d4h_bt_observe(void);
d4h_runtime d4h_rt;
typedef struct { void *address; uint32_t size; int32_t prot; } segment;
typedef struct {
    size_t size; char name[256]; segment segments[4];
    uint32_t count; uint8_t fingerprint[20];
} info;
typedef struct {
    uint64_t device; uint16_t vendor,product;
    uint8_t type,status; uint16_t size; uint64_t reserved;
    uint8_t report[78],pad[2];
} bt_event;
static unsigned checks,initializations,callbacks,uncallbacks,joins,registrations,pins;
static int mode;
static uint64_t clock_us;
static unsigned char image[0x10000];
static int worker;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"BT FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static uint64_t now(void) { return clock_us; }
static int pause_us(unsigned n) { CHECK(n==100000); clock_us+=n; return 0; }
int d4h_stopping(void) { return mode==5; }
void d4h_log(const char *fmt,...) { CHECK(fmt!=NULL); }
void d4h_hex(const char *label,const uint8_t *p,size_t n) { (void)label; (void)p; (void)n; }
int d4h_module(const char *path,const char *anchor)
{ CHECK(strstr(path,"libSceBluetoothHid.sprx") && !strcmp(anchor,"sceBluetoothHidInit")); return 7; }
int d4h_module_owned(int h) { CHECK(h==7); return mode!=1; }
void d4h_pin_module(int h) { CHECK(h==7); ++pins; }
uint32_t d4h_bt_crc(const uint8_t *p,size_t n)
{ CHECK(p==image && n==16384); return mode==2?0:0xb489ea13u; }
int d4h_parse(d4h_transport t,const uint8_t *p,size_t n,d4h_state *s)
{ (void)t; (void)p; (void)n; (void)s; CHECK(0); return -1; }
static int32_t initialize(void) { ++initializations; return mode==3?-1:0; }
static int32_t callback(void (*fn)(const bt_event *,int,void *),void *context,const void *param)
{ CHECK(fn && context && !param); ++callbacks; return mode==4?-1:0; }
static int32_t uncallback(void) { ++uncallbacks; return 0; }
static int32_t register_device(uint16_t a,uint16_t b)
{ CHECK(a==0x054c && b==0x0ce6); ++registrations; return (int32_t)0x81540002u; }
static int32_t unregister_device(uint16_t a,uint16_t b)
{ (void)a; (void)b; CHECK(0); return 0; }
static int32_t join(void *thread,void **result)
{ CHECK(thread==&worker && result && uncallbacks==1); ++joins; return 0; }
static int32_t get_info(int32_t h,info *p)
{
    CHECK(h==7 && p->size==sizeof(*p));
    p->count=2; p->segments[0]=(segment){image,16384,5};
    p->segments[1]=(segment){image+0x8000,16384,3}; return 0;
}
int d4h_resolve(int h,const char *name,void *dst,size_t n)
{
    (void)h;
#define FN(symbol,fn) if (!strcmp(name,symbol)) { __typeof__(&fn) p=&fn; CHECK(n==sizeof(p)); memcpy(dst,&p,n); return 0; }
    FN("sceBluetoothHidInit",initialize)
    FN("sceBluetoothHidRegisterCallback",callback)
    FN("sceBluetoothHidUnregisterCallback",uncallback)
    FN("sceBluetoothHidRegisterDevice",register_device)
    FN("sceBluetoothHidUnregisterDevice",unregister_device)
    FN("scePthreadJoin",join)
    FN("sceKernelGetModuleInfo",get_info)
#undef FN
    CHECK(0); return -1;
}
int main(void)
{
    for (mode=0;mode<=5;++mode) {
        initializations=callbacks=uncallbacks=joins=registrations=pins=0; clock_us=0;
        void *thread=&worker; memcpy(image+0x8048,&thread,sizeof(thread));
        d4h_rt.time_us=now; d4h_rt.usleep=pause_us;
        int ret=d4h_bt_observe();
        if (mode==1) { CHECK(ret==-31 && !initializations); continue; }
        if (mode==2) { CHECK(ret==-34 && !initializations); continue; }
        if (mode==3) { CHECK(ret==-36 && !callbacks && !pins); continue; }
        if (mode==4) { CHECK(ret==-37 && pins==1 && !uncallbacks); continue; }
        CHECK(initializations==1 && callbacks==1 && pins==1 && uncallbacks==1 && joins==1);
#ifdef D4H_BT_PASSIVE
        CHECK(ret==0 && registrations==0);
        CHECK(clock_us==(mode==5?0:30000000u));
#else
        CHECK(ret==-38 && registrations==1 && clock_us==0);
#endif
    }
    printf("PASS BT: %u checks; mock ownership, fingerprint, bounded wait and callback join\n",checks);
    return 0;
}
