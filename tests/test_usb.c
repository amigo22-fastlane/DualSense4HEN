/* SPDX-License-Identifier: GPL-3.0-only
 * Mock sceUsbd boundary. Does not call libusb or console APIs. */
#include "../platform/ps4/runtime.h"
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
int d4h_usb_probe(void);
d4h_runtime d4h_rt;
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"USB FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static int mode,claimed,closed,released,freed_list,freed_config,exited,unloaded,reads;
static uint64_t clock_us;
static unsigned raw_samples,mapped_samples,parse_samples;
static char logs[65536];
static size_t log_size;
static unsigned long long last_parse_ms;
static int controls,control_maps;
static int dev_token,handle_token;
static int fake_errno;
static int *error_address(void) { return &fake_errno; }
#ifdef D4H_USB_ALT_ZERO_PROBE
static int alt_calls;
static int32_t select_alt(libusb_device_handle *h,int32_t iface_number,int32_t setting)
{
    CHECK(h==(libusb_device_handle *)&handle_token && iface_number==3 && setting==0);
    ++alt_calls; if (mode==18) { fake_errno=13; return -1; } return 0;
}
#endif
static uint8_t hid_extra[]={9,0x21,0x11,0x01,0,1,0x22,32,0};
static struct libusb_endpoint_descriptor endpoint[2];
static struct libusb_interface_descriptor alt[2];
static struct libusb_interface iface[2];
static struct libusb_config_descriptor config;
static libusb_device *devices[2];
void d4h_log(const char *fmt,...)
{
    if (strstr(fmt,"ds4-dry-run")) ++mapped_samples;
    if (strstr(fmt,"usb-control-map")) ++control_maps;
    if (strstr(fmt,"usb-parse OK")) ++parse_samples;
    va_list ap; va_start(ap,fmt);
    int n=vsnprintf(logs+log_size,sizeof(logs)-log_size,fmt,ap);
    va_end(ap);
    CHECK(n>=0 && (size_t)n<sizeof(logs)-log_size);
    if (strstr(fmt,"usb-parse OK")) CHECK(sscanf(logs+log_size,"usb-parse OK t_ms=%llu",&last_parse_ms)==1);
    log_size+=(size_t)n;
}
void d4h_hex(const char *label,const uint8_t *p,size_t n)
{
    CHECK(p && ((!strcmp(label,"usb-hid-report-descriptor-prefix") && n==32) || n==64));
    if (!strcmp(label,"usb-raw")) ++raw_samples;
}
int d4h_stopping(void) { return mode==8; }
int d4h_module(const char *p,const char *anchor) { CHECK(strstr(p,"libSceUsbd.sprx")!=NULL && !strcmp(anchor,"sceUsbdInit")); return mode==1?-1:50; }
static int32_t init_usb(void) { return mode==2?-1:0; }
static void exit_usb(void) { ++exited; }
static int64_t list_usb(libusb_device ***p) { *p=devices; return mode==3?0:1; }
static void free_list(libusb_device **p,int32_t unref_devices) { CHECK(p==devices && unref_devices==1); ++freed_list; }
static int32_t desc_usb(libusb_device *d,struct libusb_device_descriptor *p)
{
    CHECK(d==devices[0]); p->idVendor=0x054c; p->idProduct=mode==4?0x0001:0x0ce6; return 0;
}
static int32_t cfg_usb(libusb_device *d,struct libusb_config_descriptor **p)
{
    CHECK(d==devices[0]); *p=mode==5?NULL:&config; return mode==5?-1:0;
}
static void free_cfg(struct libusb_config_descriptor *p) { CHECK(p==&config); ++freed_config; }
static int32_t open_usb(libusb_device *d,libusb_device_handle **p)
{
    CHECK(d==devices[0]); *p=(libusb_device_handle *)&handle_token; return 0;
}
static void close_usb(libusb_device_handle *h) { CHECK(h==(libusb_device_handle *)&handle_token); ++closed; }
static int32_t claim(libusb_device_handle *h,int32_t i)
{
    CHECK(h==(libusb_device_handle *)&handle_token && i==3); ++claimed; return mode==6?-6:0;
}
static int32_t release(libusb_device_handle *h,int32_t i)
{
    CHECK(h==(libusb_device_handle *)&handle_token && i==3); ++released; return 0;
}
static int32_t transfer(libusb_device_handle *h,unsigned char e,unsigned char *p,int32_t n,int32_t *got,uint32_t timeout)
{
    CHECK(h==(libusb_device_handle *)&handle_token && e==0x84 && n==64 && timeout==100);
    ++reads;
    if (mode==9) return LIBUSB_ERROR_TIMEOUT;
    if (mode==19) return (int32_t)0x80240007u;
    if (mode==10) return LIBUSB_ERROR_NO_DEVICE;
    if (mode==14) { fake_errno=45; return (int32_t)0x802400ffu; } /* Simulated backend errno. */
    if (mode==12 && reads>2) return LIBUSB_ERROR_NO_DEVICE;
    memset(p,0,64); p[0]=1; p[1]=p[2]=p[3]=p[4]=128;
    p[8]=8; p[33]=p[37]=0x80; *got=64;
    if (mode==13) p[8]=15; /* Invalid hat: never map it. */
    if (mode==11 && clock_us>20000000) p[10]=1; /* PS late in capture. */
    return 0;
}
static int32_t control(libusb_device_handle *h,uint8_t type,uint8_t request,uint16_t value,
    uint16_t index,unsigned char *data,uint16_t length,uint32_t timeout)
{
    CHECK(h==(libusb_device_handle *)&handle_token && index==3 && timeout==100);
    ++controls;
    if (request==6) {
        CHECK(type==0x81 && value==0x2200 && length==32);
        memset(data,0,length); return 32;
    }
    CHECK(type==0xa1 && request==1 && value==0x0101 && length==64);
    if (mode!=15) return (int32_t)0x802400ffu;
    int32_t got=0;
    CHECK(transfer(h,0x84,data,length,&got,timeout)==0);
    return got;
}
static uint64_t time_us(void) { clock_us+=mode==11?4000:1000000; return clock_us; }
int d4h_release_module(int h)
{
    CHECK(h==50); ++unloaded; return 0;
}
int d4h_resolve(int h,const char *name,void *dst,size_t size)
{
    if (!strcmp(name,"__error")) {
        CHECK(h==0x2001); __typeof__(&error_address) ptr=&error_address;
        CHECK(size==sizeof(ptr)); memcpy(dst,&ptr,size); return 0;
    }
    CHECK(h==50);
#define EXPORT(symbol,fn) if (!strcmp(name,symbol)) { __typeof__(&fn) ptr=&fn; CHECK(size==sizeof(ptr)); memcpy(dst,&ptr,size); return 0; }
    EXPORT("sceUsbdInit",init_usb)
    EXPORT("sceUsbdExit",exit_usb)
    EXPORT("sceUsbdGetDeviceList",list_usb)
    EXPORT("sceUsbdFreeDeviceList",free_list)
    EXPORT("sceUsbdGetDeviceDescriptor",desc_usb)
    EXPORT("sceUsbdGetActiveConfigDescriptor",cfg_usb)
    EXPORT("sceUsbdFreeConfigDescriptor",free_cfg)
    EXPORT("sceUsbdOpen",open_usb)
    EXPORT("sceUsbdClose",close_usb)
    EXPORT("sceUsbdClaimInterface",claim)
    EXPORT("sceUsbdReleaseInterface",release)
    EXPORT("sceUsbdInterruptTransfer",transfer)
    if (mode!=17) { EXPORT("sceUsbdControlTransfer",control) }
#ifdef D4H_USB_ALT_ZERO_PROBE
    EXPORT("sceUsbdSetInterfaceAltSetting",select_alt)
#endif
#undef EXPORT
    return -1;
}
static void reset(int m)
{
    mode=m; claimed=closed=released=freed_list=freed_config=exited=unloaded=reads=0; clock_us=0;
    raw_samples=mapped_samples=parse_samples=0; log_size=0; logs[0]=0; last_parse_ms=0;
    controls=control_maps=0;
    fake_errno=77;
#ifdef D4H_USB_ALT_ZERO_PROBE
    alt_calls=0;
#endif
    memset(&d4h_rt,0,sizeof(d4h_rt)); d4h_rt.time_us=time_us;
    memset(&config,0,sizeof(config)); memset(iface,0,sizeof(iface));
    memset(alt,0,sizeof(alt)); memset(endpoint,0,sizeof(endpoint));
    devices[0]=(libusb_device *)&dev_token; devices[1]=NULL;
    config.bNumInterfaces=2; config.interface=iface;
    iface[0].num_altsetting=iface[1].num_altsetting=1;
    iface[0].altsetting=&alt[0]; iface[1].altsetting=&alt[1];
    /* Audio first, then HID interface 3 (not assumed to be interface 0). */
    alt[0].bInterfaceClass=1; alt[0].bNumEndpoints=1; alt[0].endpoint=&endpoint[0];
    endpoint[0].bEndpointAddress=0x81; endpoint[0].bmAttributes=1; endpoint[0].wMaxPacketSize=512;
    alt[1].bInterfaceNumber=3; alt[1].bInterfaceClass=mode==7?1:3;
    alt[1].bNumEndpoints=1; alt[1].endpoint=&endpoint[1];
    alt[1].extra=hid_extra; alt[1].extra_length=sizeof(hid_extra);
    endpoint[1].bEndpointAddress=0x84; endpoint[1].bmAttributes=3; endpoint[1].wMaxPacketSize=64;
}
int main(void)
{
    reset(0); CHECK(d4h_usb_probe()==0); CHECK(reads>0 && claimed==1 && released==1 && closed==1);
    CHECK(freed_config==1 && freed_list==1 && exited==1 && unloaded==1);
    reset(1); CHECK(d4h_usb_probe()==-20 && !exited && !unloaded);
    reset(2); CHECK(d4h_usb_probe()==-21 && !exited && unloaded==1);
    reset(3); CHECK(d4h_usb_probe()==-22 && freed_list==1 && exited==1);
    reset(4); CHECK(d4h_usb_probe()==-22 && !claimed && freed_list==1);
    reset(5); CHECK(d4h_usb_probe()==-23 && !freed_config && freed_list==1 && exited==1);
    reset(6); CHECK(d4h_usb_probe()==-3 && closed==1 && !released && freed_config==1);
    reset(7); CHECK(d4h_usb_probe()==-1 && !claimed && !closed && freed_config==1);
    reset(8); CHECK(d4h_usb_probe()==-22 && !claimed && freed_list==1 && exited==1);
    reset(9); CHECK(d4h_usb_probe()==-5 && reads>0 && released==1 && closed==1);
    reset(10); CHECK(d4h_usb_probe()==-6 && reads==1 && released==1 && closed==1);
    reset(11); CHECK(d4h_usb_probe()==0 && reads>7000 && raw_samples<=120 && raw_samples>100);
    CHECK(raw_samples==mapped_samples && raw_samples==parse_samples);
    CHECK(strstr(logs,"ds4-dry-run bytes=120 buttons=0x10000")!=NULL);
    CHECK(last_parse_ms>=29000); /* Samples still present near end. */
    reset(12); CHECK(d4h_usb_probe()==-6 && reads==3 && released==1 && closed==1);
    reset(13); CHECK(d4h_usb_probe()==-5 && !mapped_samples && !raw_samples && released==1 && closed==1);
    reset(14); CHECK(d4h_usb_probe()==-6 && reads==1 && released==1 && closed==1 && !mapped_samples);
    CHECK(strstr(logs,"io_error=0x802400ff")!=NULL);
    CHECK(strstr(logs,"bytes=0 errno=45")!=NULL && fake_errno==77);
    reset(19); CHECK(d4h_usb_probe()==-5 && reads>1 && released==1 && closed==1 && !mapped_samples);
    reset(15); CHECK(d4h_usb_probe()==0 && controls==2 && control_maps==1 && released==1);
    reset(16); alt[1].extra_length=2; CHECK(d4h_usb_probe()==0 && controls==1 && !control_maps);
    reset(17); CHECK(d4h_usb_probe()==0 && !controls && released==1);
    reset(0); alt[1].extra=NULL; CHECK(d4h_usb_probe()==0 && controls==1);
    reset(0); alt[1].extra_length=-1; CHECK(d4h_usb_probe()==0 && controls==1);
    reset(0); {
        static uint8_t malformed[]={0,0x21};
        alt[1].extra=malformed; alt[1].extra_length=sizeof(malformed);
        CHECK(d4h_usb_probe()==0 && controls==1);
    }
#ifdef D4H_USB_ALT_ZERO_PROBE
    CHECK(alt_calls==1);
    reset(18); CHECK(d4h_usb_probe()==-7 && alt_calls==1 && released==1 && closed==1 && !reads && !controls);
#endif
    reset(0); alt[1].bAlternateSetting=1; CHECK(d4h_usb_probe()==-1 && !claimed);
    reset(0); endpoint[1].bEndpointAddress=4; CHECK(d4h_usb_probe()==-1 && !claimed);
    reset(0); endpoint[1].wMaxPacketSize=32; CHECK(d4h_usb_probe()==-1 && !claimed);
    printf("PASS USB: %u checks, descriptor selection, parsing, bounded capture and cleanup failures\n",checks);
    return 0;
}
