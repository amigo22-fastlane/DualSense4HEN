/* SPDX-License-Identifier: GPL-3.0-only
 * Query interface ownership without libusb Open/Claim/FS setup or transfers. */
#include "runtime.h"
#include "ds4hen/hid.h"
#include "ds4hen/usb_owner.h"
#include <libusb.h>
#include <string.h>
typedef struct { size_t size; char text[28]; uint32_t version; } sw;
typedef struct { void *address; uint32_t size; int32_t prot; } segment;
typedef struct { size_t size; char name[256]; segment segments[4]; uint32_t count; uint8_t fingerprint[20]; } info;
_Static_assert(sizeof(info)==352,"audited module info");
static int (*native_ioctl)(int,unsigned long,...);
static int query(int fd,unsigned long request,void *argument)
{
    if (request!=D4H_USB_DEVICE_DESC && request!=D4H_USB_DRIVER_ACTIVE && request!=D4H_USB_IFACE_NAME) return -1;
    return native_ioctl(fd,request,argument);
}
static int kernel(const char *name,void *out,size_t n)
{ return d4h_resolve(0x2001,name,out,n) && d4h_resolve(1,name,out,n); }
int d4h_probe_main(void)
{
    sw version; memset(&version,0,sizeof(version)); version.size=sizeof(version);
    int (*firmware)(sw *)=0; int32_t (*module_info)(int32_t,info *)=0;
    if (kernel("sceKernelGetSystemSwVersion",&firmware,sizeof(firmware)) ||
        kernel("sceKernelGetModuleInfo",&module_info,sizeof(module_info)) ||
        kernel("ioctl",&native_ioctl,sizeof(native_ioctl))) return -10;
    if (firmware(&version) || (version.version>>16)!=0x1252 || d4h_stopping()) return -11;
    d4h_log("USB owner revision=1 firmware=0x%x; RO query only; no detach/claim/FS/transfer/power\n",version.version);
    int h=d4h_module("/system/common/lib/libSceUsbd.sprx","sceUsbdInit");
    if (h<0 || !d4h_module_owned(h)) { d4h_log("USB owner refuses resident/shared Usbd\n"); return -12; }
    info module; memset(&module,0,sizeof(module)); module.size=sizeof(module);
    if (module_info(h,&module) || !module.count || module.count>4 || !module.segments[0].address ||
        module.segments[0].size!=65536 || !(module.segments[0].prot&1) ||
        d4h_bt_crc(module.segments[0].address,65536)!=0x3efeb3edu) return -13;
    d4h_log("USB owner ABI profile verified: 65536-byte 12.52 RX, CRC(A1)=0x3efeb3ed\n");
    int32_t (*init)(void)=0; void (*exit_usb)(void)=0;
    int64_t (*list)(libusb_device ***)=0; void (*free_list)(libusb_device **,int32_t)=0;
    int32_t (*desc)(libusb_device *,struct libusb_device_descriptor *)=0;
    uint8_t (*bus)(libusb_device *)=0;
    uint8_t (*address)(libusb_device *)=0;
#define RESOLVE(dst,name) d4h_resolve(h,name,&dst,sizeof(dst))
    if (RESOLVE(init,"sceUsbdInit") || RESOLVE(exit_usb,"sceUsbdExit") ||
        RESOLVE(list,"sceUsbdGetDeviceList") || RESOLVE(free_list,"sceUsbdFreeDeviceList") ||
        RESOLVE(desc,"sceUsbdGetDeviceDescriptor") || RESOLVE(bus,"sceUsbdGetBusNumber") ||
        RESOLVE(address,"sceUsbdGetDeviceAddress")) return -14;
#undef RESOLVE
    /* Profile-verified leaf getters. No reads from inferred device internals. */
    uintptr_t base=(uintptr_t)module.segments[0].address;
    if ((uintptr_t)bus!=base+0x420 || (uintptr_t)address!=base+0x430) {
        d4h_log("USB owner getter ABI mismatch; refusing enumeration\n"); return -15;
    }
    int *(*error_address)(void)=0; int *error=0;
    if (!kernel("__error",&error_address,sizeof(error_address))) error=error_address();
    int ret=init(); d4h_log("USB owner Init=0x%x\n",ret);
    if (ret) return -16;
    libusb_device **devices=0; int64_t count=list(&devices);
    d4h_log("USB owner count=%lld\n",(long long)count);
    ret=-17;
    if (count>=0 && count<=256 && devices) for (int64_t i=0;i<count && !d4h_stopping();++i) {
        struct libusb_device_descriptor d; memset(&d,0,sizeof(d));
        if (desc(devices[i],&d) || d.idVendor!=0x054c || d.idProduct!=0x0ce6) continue;
        unsigned b=bus(devices[i]),a=address(devices[i]);
        d4h_log("USB owner identified 054c:0ce6 bus=%u address=%u iface=3\n",b,a);
        d4h_usb_owner_api api={d4h_rt.open,query,d4h_rt.close,error}; d4h_usb_owner_result observed;
        memset(&observed,0,sizeof(observed));
        ret=d4h_usb_owner_query(&api,b,a,&observed);
        d4h_log("USB owner open=%d errno=%d identity_ret=%d errno=%d vid=%04x pid=%04x\n",
            observed.open_result,observed.open_errno,observed.identity_result,observed.identity_errno,observed.vendor,observed.product);
        d4h_log("USB owner ACTIVE raw=%d errno=%d NAME raw=%d errno=%d driver=%s close=%d result=%d\n",
            observed.active_result,observed.active_errno,observed.name_result,observed.name_errno,observed.driver,observed.close_result,ret);
        break;
    }
    if (devices) free_list(devices,1);
    exit_usb();
    d4h_log("USB owner complete=%d; ACTIVE/name never proves cause of previous interrupt failure\n",ret);
    return ret;
}
