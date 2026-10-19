/* SPDX-License-Identifier: GPL-3.0-only
 * sceUsbd signatures checked against OpenOrbis include/orbis/Usbd.h.
 * Descriptor types supplied by the SDK's libusb.h; no Linux libusb calls. */
#include "runtime.h"
#include "ds4hen/hid.h"
#include "ds4hen/ps4_pad.h"
#include <libusb.h>
#include <string.h>
typedef struct {
    int32_t (*init)(void);
    int32_t (*exit)(void);
    int32_t (*list)(libusb_device ***);
    int32_t (*free_list)(libusb_device **);
    int32_t (*descriptor)(libusb_device *,struct libusb_device_descriptor *);
    int32_t (*config)(libusb_device *,struct libusb_config_descriptor **);
    int32_t (*free_config)(struct libusb_config_descriptor *);
    int32_t (*open)(libusb_device *,libusb_device_handle **);
    void (*close)(libusb_device_handle *);
    int32_t (*claim)(libusb_device_handle *,int32_t);
    int32_t (*release)(libusb_device_handle *,int32_t);
    int32_t (*read)(libusb_device_handle *,unsigned char,unsigned char *,int32_t,int32_t *,uint32_t);
} usb_api;
static int resolve_usb(int h,usb_api *u)
{
    memset(u,0,sizeof(*u));
    return D4H_RESOLVE(h,*u,init,"sceUsbdInit") ||
        D4H_RESOLVE(h,*u,exit,"sceUsbdExit") ||
        D4H_RESOLVE(h,*u,list,"sceUsbdGetDeviceList") ||
        D4H_RESOLVE(h,*u,free_list,"sceUsbdFreeDeviceList") ||
        D4H_RESOLVE(h,*u,descriptor,"sceUsbdGetDeviceDescriptor") ||
        D4H_RESOLVE(h,*u,config,"sceUsbdGetActiveConfigDescriptor") ||
        D4H_RESOLVE(h,*u,free_config,"sceUsbdFreeConfigDescriptor") ||
        D4H_RESOLVE(h,*u,open,"sceUsbdOpen") ||
        D4H_RESOLVE(h,*u,close,"sceUsbdClose") ||
        D4H_RESOLVE(h,*u,claim,"sceUsbdClaimInterface") ||
        D4H_RESOLVE(h,*u,release,"sceUsbdReleaseInterface") ||
        D4H_RESOLVE(h,*u,read,"sceUsbdInterruptTransfer");
}
static int find_hid(const struct libusb_config_descriptor *c,int *iface,uint8_t *endpoint)
{
    if (!c || !c->interface) return -1;
    for (unsigned i=0;i<c->bNumInterfaces;++i) {
        const struct libusb_interface *in=&c->interface[i];
        if (!in->altsetting) continue;
        for (int j=0;j<in->num_altsetting;++j) {
            const struct libusb_interface_descriptor *alt=&in->altsetting[j];
            if (alt->bInterfaceClass!=3 || alt->bAlternateSetting!=0 || !alt->endpoint) continue;
            for (unsigned k=0;k<alt->bNumEndpoints;++k) {
                const struct libusb_endpoint_descriptor *e=&alt->endpoint[k];
                if ((e->bEndpointAddress&0x80) && (e->bmAttributes&3)==3 && (e->wMaxPacketSize&0x7ff)>=64) {
                    *iface=alt->bInterfaceNumber; *endpoint=e->bEndpointAddress; return 0;
                }
            }
        }
    }
    return -1;
}
static int capture(usb_api *u,libusb_device *dev,const struct libusb_config_descriptor *config)
{
    int iface=-1; uint8_t endpoint=0;
    libusb_device_handle *handle=0;
    if (find_hid(config,&iface,&endpoint)) { d4h_log("no HID interrupt IN >=64 at alt 0\n"); return -1; }
    d4h_log("HID interface=%d endpoint=0x%x\n",iface,endpoint);
    int ret=u->open(dev,&handle);
    d4h_log("sceUsbdOpen=0x%x\n",ret);
    if (ret || !handle) return -2;
    ret=u->claim(handle,iface);
    d4h_log("sceUsbdClaimInterface=0x%x (no detach/reset fallback)\n",ret);
    if (ret) { u->close(handle); return -3; }
    uint64_t start=d4h_rt.time_us(),last_sample=0;
    unsigned good=0,bad=0,timeouts=0,samples=0; int io_error=0;
    d4h_log("USB capture begin: <=30s / <=30000 reads; hold each input >=1s; DS4 dry-run only\n");
    /* Sample throughout the capture, not just the first second. Parse/map every
     * valid report, but never call VDA or change process privileges here. */
    for (unsigned attempts=0;attempts<30000 && !d4h_stopping();++attempts) {
        uint64_t now=d4h_rt.time_us();
        if (now<start || now-start>=30000000u) break;
        uint8_t report[128]; int32_t received=0;
        ret=u->read(handle,endpoint,report,sizeof(report),&received,100);
        if (ret==LIBUSB_ERROR_TIMEOUT) { ++timeouts; continue; }
        if (ret) { io_error=ret; d4h_log("interrupt error=0x%x bytes=%d\n",ret,received); break; }
        if (received<=0 || received>(int32_t)sizeof(report)) { ++bad; continue; }
        d4h_state state;
        int parse=d4h_parse(D4H_USB,report,(size_t)received,&state);
        if (!parse) {
            ++good;
            d4h_ps4_pad_data pad;
            d4h_map_ps4(&state,now,(uint8_t)good,&pad);
            if (samples<120 && (!samples || now-last_sample>=250000u)) {
                last_sample=now; ++samples;
                d4h_hex("usb-raw",report,(size_t)received);
                d4h_log("usb-parse OK t_ms=%llu buttons=0x%x sticks=%u,%u,%u,%u triggers=%u,%u touch=%u,%u\n",
                    (unsigned long long)((now-start)/1000u),state.buttons,state.lx,state.ly,state.rx,state.ry,state.l2,state.r2,
                    state.touch[0].active,state.touch[1].active);
                d4h_log("ds4-dry-run bytes=%zu buttons=0x%x sticks=%u,%u,%u,%u triggers=%u,%u fingers=%u touch0=%u,%u touch1=%u,%u\n",
                    sizeof(pad),pad.buttons,pad.lx,pad.ly,pad.rx,pad.ry,pad.l2,pad.r2,pad.touch_data.fingers,
                    pad.touch_data.touch[0].x,pad.touch_data.touch[0].y,
                    pad.touch_data.touch[1].x,pad.touch_data.touch[1].y);
            }
        } else { ++bad; if (bad<=16) { d4h_hex("usb-invalid",report,(size_t)received); d4h_log("usb-parse error=%d\n",parse); } }
    }
    d4h_log("USB summary good=%u bad=%u timeouts=%u samples=%u io_error=0x%x stopped=%d\n",
        good,bad,timeouts,samples,io_error,d4h_stopping());
    int release=u->release(handle,iface);
    d4h_log("sceUsbdReleaseInterface=0x%x\n",release);
    u->close(handle);
    return release ? -4 : io_error ? -6 : good ? 0 : -5;
}
int d4h_usb_probe(void)
{
    d4h_log("USB probe revision=2 begin; enumeration then bounded HID capture; no VDA calls\n");
    usb_api u;
    int h=d4h_module("/system/common/lib/libSceUsbd.sprx","sceUsbdInit");
    if (h<0) return -20;
    int result=-21;
    if (resolve_usb(h,&u)) { d4h_log("USB export unavailable; abort\n"); goto unload; }
    int ret=u.init();
    d4h_log("sceUsbdInit=0x%x\n",ret);
    if (ret) goto unload; /* Do not exit a context that we did not initialize. */
    libusb_device **list=0;
    int32_t count=u.list(&list);
    d4h_log("sceUsbdGetDeviceList count=%d list=%p first=%p\n",count,(void *)list,
            count>=0 && list ? (void *)list[0] : (void *)0);
    result=-22;
    if (count>=0 && list) {
        /* Only the standard DualSense VID/PID; Edge is a separate test target. */
        for (int32_t i=0;i<count && !d4h_stopping();++i) {
            struct libusb_device_descriptor desc;
            memset(&desc,0,sizeof(desc));
            ret=u.descriptor(list[i],&desc);
            d4h_log("USB device index=%d descriptor status=0x%x\n",i,ret);
            if (ret) continue;
            d4h_log("USB descriptor VID=%04x PID=%04x bcdUSB=0x%x bcdDevice=0x%x class=%u subclass=%u protocol=%u configs=%u ep0=%u length=%u type=%u\n",
                desc.idVendor,desc.idProduct,desc.bcdUSB,desc.bcdDevice,desc.bDeviceClass,desc.bDeviceSubClass,
                desc.bDeviceProtocol,desc.bNumConfigurations,desc.bMaxPacketSize0,desc.bLength,desc.bDescriptorType);
            if (desc.idVendor!=0x054c || desc.idProduct!=0x0ce6) continue;
            d4h_log("DualSense VID=%04x PID=%04x\n",desc.idVendor,desc.idProduct);
            struct libusb_config_descriptor *config=0;
            ret=u.config(list[i],&config);
            d4h_log("active config=0x%x\n",ret);
            if (!ret && config) {
                d4h_log("USB config value=%u interfaces=%u total_length=%u attributes=0x%x max_power=%u\n",
                    config->bConfigurationValue,config->bNumInterfaces,config->wTotalLength,config->bmAttributes,config->MaxPower);
                if (config->interface) for (unsigned n=0;n<config->bNumInterfaces;++n) {
                    const struct libusb_interface *in=&config->interface[n];
                    if (!in->altsetting) continue;
                    for (int a=0;a<in->num_altsetting;++a) {
                        const struct libusb_interface_descriptor *alt=&in->altsetting[a];
                        d4h_log("USB interface=%u alt=%u class=%u subclass=%u protocol=%u endpoints=%u\n",
                            alt->bInterfaceNumber,alt->bAlternateSetting,alt->bInterfaceClass,
                            alt->bInterfaceSubClass,alt->bInterfaceProtocol,alt->bNumEndpoints);
                        if (alt->endpoint) for (unsigned e=0;e<alt->bNumEndpoints;++e) {
                            const struct libusb_endpoint_descriptor *ep=&alt->endpoint[e];
                            d4h_log("USB endpoint=0x%x attributes=0x%x max_packet=%u interval=%u\n",
                                ep->bEndpointAddress,ep->bmAttributes,ep->wMaxPacketSize,ep->bInterval);
                        }
                    }
                }
                result=capture(&u,list[i],config); u.free_config(config);
            }
            else result=-23;
            break;
        }
        u.free_list(list);
    }
    (void)u.exit();
    d4h_log("sceUsbdExit called; return value is not treated as status\n");
unload:
    {
        d4h_log("release USB=0x%x\n",d4h_release_module(h));
    }
    return result;
}
