/* SPDX-License-Identifier: GPL-3.0-only
 * No Sony bytes: mocks enforce the audit's module and output-file scope. */
#include "../platform/ps4/runtime.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { void *address; uint32_t size; int32_t prot; } segment;
typedef struct { size_t size; char name[256]; segment segments[4]; uint32_t count; uint8_t fingerprint[20]; } info;
typedef struct { size_t size; char text[28]; uint32_t version; } sw;
static unsigned checks,opens,writes,closes,loads,releases;
static int mode;
static uint8_t rx[32],ro[16],rw[64];
d4h_runtime d4h_rt;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"AUDIT FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
int d4h_probe_main(void);
void d4h_log(const char *fmt,...) { CHECK(fmt!=NULL); }
int d4h_stopping(void) { return mode==2; }
int d4h_module(const char *path,const char *anchor)
{
    CHECK(!strcmp(path,"/system/common/lib/libSceHidControl.sprx"));
    CHECK(!strcmp(anchor,"sceHidControlGetDeviceInfo")); ++loads; return 7;
}
int d4h_release_module(int h) { CHECK(h==7); ++releases; return 0; }
static int firmware(sw *p) { CHECK(p->size==40); p->version=mode==1?0x12530001:0x12520001; return 0; }
static int module_info(int h,info *p)
{
    CHECK(h==7 && p->size==352); p->count=mode==3?5:3;
    p->segments[0]=(segment){rx,sizeof(rx),5};
    p->segments[1]=(segment){ro,sizeof(ro),1};
    p->segments[2]=(segment){rw,sizeof(rw),3}; return 0;
}
int d4h_resolve(int h,const char *name,void *dst,size_t n)
{
    CHECK(n==sizeof(void *));
    if (!strcmp(name,"sceKernelGetSystemSwVersion")) { void *p=(void *)&firmware; memcpy(dst,&p,n); return 0; }
    if (!strcmp(name,"sceKernelGetModuleInfo")) { void *p=(void *)&module_info; memcpy(dst,&p,n); return 0; }
    CHECK(h==7);
    CHECK(!strcmp(name,"sceHidControlGetDeviceInfo") || !strcmp(name,"sceHidControlGetDeviceName") || !strcmp(name,"sceHidControlGetBatteryState"));
    void *p=rx; memcpy(dst,&p,n); return 0; /* Addresses are logged, never invoked. */
}
static int open_file(const char *path,int flags,...)
{
    CHECK(!strcmp(path,"/data/dualsense4hen-hidcontrol-seg0.bin") ||
          !strcmp(path,"/data/dualsense4hen-hidcontrol-seg1.bin"));
    CHECK(flags==(O_WRONLY|O_CREAT|O_EXCL) && !(flags&O_TRUNC));
    ++opens; return mode==4?-1:9;
}
static ssize_t write_file(int fd,const void *bytes,size_t n)
{ CHECK(fd==9 && ((bytes==rx && n==sizeof(rx)) || (bytes==ro && n==sizeof(ro)))); ++writes; return mode==5?-1:(ssize_t)n; }
static int close_file(int fd) { CHECK(fd==9); ++closes; return mode==6?-1:0; }
int main(void)
{
    d4h_rt.open=open_file; d4h_rt.write=write_file; d4h_rt.close=close_file;
    for (mode=0;mode<7;++mode) {
        opens=writes=closes=loads=releases=0;
        int ret=d4h_probe_main();
        if (mode==1) { CHECK(ret==-11 && !loads); continue; }
        if (mode==2) { CHECK(ret==0 && !loads); continue; }
        CHECK(loads==1 && releases==1);
        if (mode==3) CHECK(ret==-41 && !opens && !writes && !closes);
        else if (mode==4) CHECK(ret==-43 && opens==2 && !writes && !closes);
        else if (mode==5) CHECK(ret==-44 && opens==2 && writes==2 && closes==2);
        else if (mode==6) CHECK(ret==-45 && opens==2 && writes==2 && closes==2);
        else CHECK(ret==0 && opens==2 && writes==2 && closes==2);
    }
    printf("PASS HID Control audit: %u checks; RX/R only, exclusive files, no HID calls\n",checks);
    return 0;
}
