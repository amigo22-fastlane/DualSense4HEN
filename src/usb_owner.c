/* SPDX-License-Identifier: GPL-3.0-only */
#include "ds4hen/usb_owner.h"
#include <fcntl.h>
#include <string.h>
/* Recovered fields used by the 12.52 GET_IFACE_DRIVER call: pointer +0,
 * max length +10, interface index +18, total descriptor 32 bytes. */
typedef struct {
    void *data;
    uint16_t language,max_length,actual_length,offset;
    uint8_t config,string,interface_index,alternate,endpoint,report,reserved[8];
} iface_name;
_Static_assert(sizeof(iface_name)==32 && offsetof(iface_name,max_length)==10 &&
               offsetof(iface_name,interface_index)==18,"audited 64-bit query descriptor");
static void reset_error(const d4h_usb_owner_api *a) { if (a->error) *a->error=0; }
static int error_value(const d4h_usb_owner_api *a) { return a->error?*a->error:-1; }
static size_t decimal(char *out,unsigned value)
{
    size_t n=0;
    if (value>=100) out[n++]=(char)('0'+value/100);
    if (value>=10) out[n++]=(char)('0'+value/10%10);
    out[n++]=(char)('0'+value%10); return n;
}
int d4h_usb_owner_query(const d4h_usb_owner_api *a,unsigned bus,unsigned address,d4h_usb_owner_result *out)
{
    if (!out) return -1;
    memset(out,0,sizeof(*out)); out->open_result=out->identity_result=out->active_result=out->name_result=out->close_result=-1;
    if (!a || !a->open || !a->query || !a->close || bus>255 || !address || address>127) return -1;
    char path[20]="/dev/ugen"; size_t n=9;
    n+=decimal(path+n,bus); path[n++]='.'; n+=decimal(path+n,address); path[n]=0;
    reset_error(a); int fd=a->open(path,O_RDONLY);
    out->open_errno=error_value(a); out->open_result=fd;
    if (fd<0) return -2;
    uint8_t descriptor[18]={0};
    reset_error(a); out->identity_result=a->query(fd,D4H_USB_DEVICE_DESC,descriptor);
    out->identity_errno=error_value(a);
    int result=-3;
    if (out->identity_result) goto done;
    out->vendor=(uint16_t)(descriptor[8]|(unsigned)descriptor[9]<<8);
    out->product=(uint16_t)(descriptor[10]|(unsigned)descriptor[11]<<8);
    if (descriptor[0]!=18 || descriptor[1]!=1 || out->vendor!=0x054c || out->product!=0x0ce6) goto done;
    int index=3;
    reset_error(a); out->active_result=a->query(fd,D4H_USB_DRIVER_ACTIVE,&index);
    out->active_errno=error_value(a);
    iface_name name; memset(&name,0,sizeof(name));
    name.data=out->driver; name.max_length=sizeof(out->driver); name.interface_index=3;
    reset_error(a); out->name_result=a->query(fd,D4H_USB_IFACE_NAME,&name);
    out->name_errno=error_value(a);
    out->driver[sizeof(out->driver)-1]=0;
    /* Keep logs bounded and printable; a failed query is not a driver name. */
    if (out->name_result) memset(out->driver,0,sizeof(out->driver));
    for (size_t i=0;i<sizeof(out->driver)-1 && out->driver[i];++i)
        if ((unsigned char)out->driver[i]<32 || (unsigned char)out->driver[i]>126) out->driver[i]='?';
    result=(out->active_result || out->name_result)?-4:0;
done:
    out->close_result=a->close(fd);
    return out->close_result?-5:result;
}
