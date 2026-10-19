/* SPDX-License-Identifier: GPL-3.0-only */
#include "ds4hen/hid.h"
#include "ds4hen/vpad.h"
#include "../platform/ps4/vda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); exit(1); } } while (0)
static size_t fixture(const char *name,uint8_t *r)
{
    char path[180]; unsigned byte; size_t n=0;
    snprintf(path,sizeof(path),"tests/fixtures/%s",name);
    FILE *f=fopen(path,"r"); CHECK(f!=NULL);
    while (fscanf(f,"%x",&byte)==1) { CHECK(n<128 && byte<256); r[n++]=(uint8_t)byte; }
    CHECK(feof(f)); fclose(f); return n;
}
static void crc(uint8_t *r)
{
    uint32_t c=d4h_bt_crc(r,74);
    for (unsigned i=0;i<4;++i) r[74+i]=(uint8_t)(c>>(i*8));
}
static void reports(void)
{
    uint8_t r[128]; d4h_state s,u,b,unchanged;
    size_t n=fixture("usb-neutral.hex",r);
    CHECK(d4h_parse(D4H_USB,r,n,&s)==0);
    CHECK(s.buttons==0 && s.lx==128 && !s.touch[0].active && !s.touch[1].active);
    n=fixture("usb-controls.hex",r);
    CHECK(d4h_parse(D4H_USB,r,n,&u)==0);
    CHECK(u.buttons==(D4H_CROSS|D4H_UP|D4H_RIGHT|D4H_SHARE|D4H_OPTIONS|D4H_PS|D4H_TOUCH_CLICK));
    CHECK(u.lx==0 && u.ly==255 && u.rx==64 && u.ry==192 && u.l2==17 && u.r2==254);
    CHECK(u.has_touch && u.has_motion && u.mute);
    CHECK(u.touch[0].active && u.touch[0].id==7 && u.touch[0].x==1919 && u.touch[0].y==1079);
    CHECK(!u.touch[1].active && u.touch[1].x==0 && u.touch[1].y==0);
    CHECK(u.gyro[0]==-32768 && u.gyro[1]==32767 && u.gyro[2]==-1);
    CHECK(u.accel[0]==0 && u.accel[1]==1 && u.accel[2]==-16384);
    CHECK(u.sensor_timestamp==0x12345678);
    n=fixture("bt-extended.hex",r);
    CHECK(d4h_parse(D4H_BLUETOOTH,r,n,&b)==0);
    CHECK(memcmp(&u,&b,sizeof(u))==0);
    unchanged=b;
    /* Every bit, including checksum bits, must be protected. */
    for (size_t i=1;i<n;++i) for (unsigned bit=0;bit<8;++bit) {
        r[i]^=(uint8_t)(1u<<bit);
        CHECK(d4h_parse(D4H_BLUETOOTH,r,n,&b)==D4H_E_CRC);
        CHECK(memcmp(&b,&unchanged,sizeof(b))==0);
        r[i]^=(uint8_t)(1u<<bit);
    }
    for (size_t i=0;i<n;++i) CHECK(d4h_parse(D4H_BLUETOOTH,r,i,&b)==D4H_E_SIZE);
    CHECK(d4h_parse(D4H_BLUETOOTH,r,n+1,&b)==D4H_E_SIZE);
    r[9]=0x0f; crc(r); CHECK(d4h_parse(D4H_BLUETOOTH,r,n,&b)==D4H_E_HAT);
    CHECK(d4h_parse(D4H_USB,r,n,&b)==D4H_E_REPORT);
    n=fixture("bt-simple.hex",r);
    CHECK(d4h_parse(D4H_BLUETOOTH,r,n,&b)==0);
    CHECK(b.buttons==u.buttons && b.l2==u.l2 && b.r2==u.r2);
    CHECK(!b.has_touch && !b.has_motion && !b.mute && !b.touch[0].active);
    CHECK(d4h_parse(D4H_USB,r,n,&b)==D4H_E_SIZE);
    for (size_t i=0;i<n;++i) CHECK(d4h_parse(D4H_BLUETOOTH,r,i,&b)==D4H_E_SIZE);
    CHECK(d4h_parse(D4H_BLUETOOTH,r,n+1,&b)==D4H_E_SIZE);
    n=fixture("usb-controls.hex",r);
    for (size_t i=0;i<n;++i) CHECK(d4h_parse(D4H_USB,r,i,&b)==D4H_E_SIZE);
    CHECK(d4h_parse(D4H_USB,r,n+1,&b)==D4H_E_SIZE);
    CHECK(d4h_parse(D4H_USB,NULL,n,&b)==D4H_E_ARGUMENT);
    CHECK(d4h_parse(D4H_USB,r,n,NULL)==D4H_E_ARGUMENT);
    CHECK(d4h_parse((d4h_transport)42,r,n,&b)==D4H_E_ARGUMENT);
}
static void mappings(void)
{
    static const uint32_t hats[]={0x10,0x30,0x20,0x60,0x40,0xc0,0x80,0x90,0};
    static const uint32_t face[]={D4H_SQUARE,D4H_CROSS,D4H_CIRCLE,D4H_TRIANGLE};
    static const uint32_t extra[]={D4H_L1,D4H_R1,D4H_L2,D4H_R2,D4H_SHARE,D4H_OPTIONS,D4H_L3,D4H_R3};
    uint8_t r[64]={1}; d4h_state s; d4h_ps4_pad_data p;
    for (unsigned i=0;i<16;++i) {
        r[8]=(uint8_t)i; int ret=d4h_parse(D4H_USB,r,64,&s);
        CHECK(i>8 ? ret==D4H_E_HAT : ret==0);
        if (i<9) CHECK(s.buttons==hats[i]);
    }
    for (unsigned i=0;i<4;++i) { r[8]=(uint8_t)(8|(16u<<i)); CHECK(d4h_parse(D4H_USB,r,64,&s)==0); CHECK(s.buttons==face[i]); }
    r[8]=8;
    for (unsigned i=0;i<8;++i) { r[9]=(uint8_t)(1u<<i); CHECK(d4h_parse(D4H_USB,r,64,&s)==0); CHECK(s.buttons==extra[i]); }
    r[9]=0;
    for (unsigned i=0;i<3;++i) { r[10]=(uint8_t)(1u<<i); CHECK(d4h_parse(D4H_USB,r,64,&s)==0); CHECK(s.buttons==(i==0?D4H_PS:i==1?D4H_TOUCH_CLICK:0)); CHECK(s.mute==(i==2)); }
    d4h_neutral(&s); s.has_touch=1;
    s.touch[1]=(d4h_touch){1919,1079,17,1};
    d4h_map_ps4(&s,99,7,&p);
    CHECK(p.touch_data.fingers==1 && p.touch_data.touch[0].finger==17);
    CHECK(p.touch_data.touch[0].x==1919 && p.touch_data.touch[0].y==941);
    CHECK(p.timestamp==99 && p.count==7 && p.connected==1 && p.quat[3]==1.0f);
    CHECK(p.accel[0]==0 && p.vel[0]==0 && p.padding==0);
    s.touch[0]=(d4h_touch){4095,4095,127,1};
    d4h_map_ps4(&s,0,0,&p);
    CHECK(p.touch_data.fingers==2 && p.touch_data.touch[0].x==1919 && p.touch_data.touch[0].y==941);
}
typedef struct { unsigned inserts,deletes; int fail_insert,fail_delete; d4h_ps4_pad_data data; } fake;
static int put(void *ctx,int32_t h,const d4h_ps4_pad_data *p)
{
    fake *f=ctx; CHECK(h==0x7030d); ++f->inserts; f->data=*p; return f->fail_insert;
}
static int del(void *ctx,int32_t h)
{
    fake *f=ctx; CHECK(h==0x7030d); ++f->deletes; return f->fail_delete;
}
static void lifecycle(void)
{
    fake f={0}; d4h_vpad v; d4h_state s; d4h_neutral(&s); s.buttons=D4H_CROSS;
    d4h_vpad_init(&v,(d4h_vpad_ops){&f,put,del});
    CHECK(d4h_vpad_send(&v,&s,1)==-1);
    CHECK(d4h_vpad_adopt(&v,-1)==-1);
    CHECK(d4h_vpad_adopt(&v,0x7030d)==0);
    CHECK(d4h_vpad_adopt(&v,0x7030d)==-1);
    CHECK(d4h_vpad_send(&v,&s,100)==0);
    CHECK(f.data.buttons==D4H_CROSS && f.data.timestamp==100 && f.data.count==1);
    CHECK(d4h_vpad_watchdog(&v,1099,1000)==0 && f.inserts==1);
    CHECK(d4h_vpad_watchdog(&v,1100,1000)==0 && f.inserts==2 && f.data.buttons==0);
    CHECK(d4h_vpad_watchdog(&v,1200,1000)==0 && f.inserts==2);
    CHECK(d4h_vpad_send(&v,&s,90)==0 && f.data.timestamp==1101);
    CHECK(d4h_vpad_watchdog(&v,80,1000)==0 && f.data.buttons==0); /* clock reset */
    f.fail_insert=-10;
    CHECK(d4h_vpad_send(&v,&s,2000)==-10 && v.last_input_us==90);
    f.fail_insert=0; f.fail_delete=-11;
    CHECK(d4h_vpad_stop(&v,3000)==-11 && v.owned && f.data.buttons==0);
    f.fail_delete=0;
    CHECK(d4h_vpad_stop(&v,3001)==0 && !v.owned);
    CHECK(d4h_vpad_stop(&v,3002)==0 && f.deletes==2);
    CHECK(d4h_vpad_adopt(&v,0x7030d)==0);
    for (unsigned i=0;i<300;++i) CHECK(d4h_vpad_send(&v,&s,i)==0);
    CHECK(f.data.count==(uint8_t)300 && f.data.timestamp==300);
    CHECK(d4h_vpad_stop(&v,5000)==0);
}
static void malformed(void)
{
    uint32_t seed=12345; uint8_t r[128]; d4h_state s;
    for (unsigned j=0;j<20000;++j) {
        for (size_t i=0;i<sizeof(r);++i) { seed=seed*1664525u+1013904223u; r[i]=(uint8_t)(seed>>24); }
        size_t n=j%129;
        d4h_neutral(&s); d4h_state before=s;
        int ret=d4h_parse((j&1)?D4H_USB:D4H_BLUETOOTH,r,n,&s);
        CHECK(ret<=0); if (ret) CHECK(memcmp(&s,&before,sizeof(s))==0);
    }
}
static unsigned vda_step;
static int privilege_failure;
static int32_t vi(void) { CHECK(vda_step++==0); return 0; }
static int32_t vp(int32_t p) { CHECK(vda_step++==1 && p==1); return privilege_failure; }
static int32_t va(void *p,int32_t type)
{
    const int32_t *words=p;
    CHECK(vda_step++==2 && type==3 && words[0]==32 && words[1]==1);
    CHECK((uint32_t)words[2]==0xdeadbeef);
    return (int32_t)0x803b0006u; /* status, not an owned handle */
}
static int32_t vs(int32_t h,const void *p) { (void)h; (void)p; return 0; }
static int32_t vd(int32_t h) { (void)h; return 0; }
static void vda_contract(void)
{
    d4h_vda_backend b;
    d4h_vda_api api={vi,vp,va,vs,vd};
    privilege_failure=-100; vda_step=0;
    CHECK(d4h_vda_init(&b,api)==-100 && !b.initialized);
    CHECK(d4h_vda_add(&b,1)==-1 && vda_step==2);
    privilege_failure=0; vda_step=0;
    CHECK(d4h_vda_init(&b,api)==0 && b.initialized);
    CHECK((uint32_t)d4h_vda_add(&b,1)==0x803b0006u);
    api.add=NULL;
    CHECK(d4h_vda_init(&b,api)==-1);
    CHECK(!b.initialized);
}
int main(void)
{
    reports(); mappings(); lifecycle(); malformed(); vda_contract();
    printf("PASS: %u checks; synthetic HID vectors, CRC, mapping, lifecycle, 20000 malformed inputs\n",checks);
    return 0;
}
