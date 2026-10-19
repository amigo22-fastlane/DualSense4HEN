/* SPDX-License-Identifier: GPL-3.0-only */
#include "runtime.h"
#include <string.h>
#include <fcntl.h>
typedef struct { size_t size; char text[28]; uint32_t version; } sw_version;
_Static_assert(sizeof(sw_version)==40,"OpenOrbis OrbisKernelSwVersion");
int d4h_usb_probe(void);
int d4h_bt_observe(void);
#ifdef D4H_ABI_DUMP
typedef struct { void *address; uint32_t size; int32_t prot; } module_segment;
typedef struct {
    size_t size; char name[256]; module_segment segments[4];
    uint32_t segment_count; uint8_t fingerprint[20];
} module_info;
_Static_assert(sizeof(module_info)==352,"OpenOrbis OrbisKernelModuleInfo");
static void dump_bt(int h,const char *const *names,size_t count)
{
    int32_t (*get_info)(int32_t,module_info *)=0;
    if (d4h_resolve(0x2001,"sceKernelGetModuleInfo",&get_info,sizeof(get_info)) &&
        d4h_resolve(1,"sceKernelGetModuleInfo",&get_info,sizeof(get_info))) return;
    module_info info; memset(&info,0,sizeof(info)); info.size=sizeof(info);
    int ret=get_info(h,&info);
    if (ret || info.segment_count>4 || !info.segment_count) { d4h_log("BT module info error=0x%x\n",ret); return; }
    info.name[255]=0;
    d4h_log("BT ABI research module=%s segments=%u\n",info.name,info.segment_count);
    uintptr_t base=(uintptr_t)info.segments[0].address;
    for (size_t i=0;i<count;++i) {
        void *p=0;
        if (!d4h_resolve(h,names[i],&p,sizeof(p)))
            d4h_log("BT symbol %s offset=0x%llx\n",names[i],(unsigned long long)((uintptr_t)p-base));
    }
    for (uint32_t i=0;i<info.segment_count;++i) {
        module_segment *s=&info.segments[i];
        d4h_log("BT segment %u offset=0x%llx bytes=%u prot=0x%x\n",i,
            (unsigned long long)((uintptr_t)s->address-base),s->size,s->prot);
        if (!s->address || !s->size || s->size>4*1024*1024 || !(s->prot&1)) continue;
        char path[]="/data/dualsense4hen-bt-seg0.bin";
        path[sizeof(path)-6]=(char)('0'+i);
        int fd=d4h_rt.open(path,O_WRONLY|O_CREAT|O_TRUNC,0600);
        if (fd<0) { d4h_log("BT segment file open failed\n"); continue; }
        size_t done=0;
        while (done<s->size) {
            ssize_t n=d4h_rt.write(fd,(const uint8_t *)s->address+done,s->size-done);
            if (n<=0 || (size_t)n>s->size-done) break;
            done+=(size_t)n;
        }
        d4h_rt.close(fd);
        d4h_log("BT saved %s bytes=%zu\n",path,done);
    }
}
#endif
static void symbols(const char *path,const char *const *names,size_t count,int dump)
{
    int h=d4h_module(path,names[0]);
    if (h<0) return;
    for (size_t i=0;i<count;++i) {
        void *p=0; int ret=d4h_resolve(h,names[i],&p,sizeof(p));
        d4h_log("export %s: %s\n",names[i],ret?"unavailable":"present (not called)");
    }
#ifdef D4H_ABI_DUMP
    if (dump) dump_bt(h,names,count);
#else
    (void)dump;
#endif
    int ret=d4h_release_module(h);
    d4h_log("release %s: ret=0x%x\n",path,ret);
}
int d4h_probe_main(void)
{
    int (*get_sw)(sw_version *)=0;
    sw_version sw;
    memset(&sw,0,sizeof(sw)); sw.size=sizeof(sw);
    if (d4h_resolve(0x2001,"sceKernelGetSystemSwVersion",&get_sw,sizeof(get_sw)) &&
        d4h_resolve(1,"sceKernelGetSystemSwVersion",&get_sw,sizeof(get_sw))) {
        d4h_log("cannot resolve firmware query; refusing probe\n"); return -10;
    }
    int ret=get_sw(&sw);
    sw.text[27]=0;
    d4h_log("firmware status=0x%x numeric=0x%x text=%s\n",ret,sw.version,sw.text);
    if (ret || (sw.version>>16)!=0x1252) {
        d4h_log("target is 12.52 only; no module probing on this firmware\n"); return -11;
    }
    if (d4h_stopping()) { d4h_log("stop marker present\n"); return 0; }
    static const char *const bt[]={
        "sceBluetoothHidInit","sceBluetoothHidParamInitialize",
        "sceBluetoothHidThreadParamInitialize","sceBluetoothHidRegisterCallback",
        "sceBluetoothHidUnregisterCallback","sceBluetoothHidRegisterDevice",
        "sceBluetoothHidUnregisterDevice","sceBluetoothHidGetDeviceInfo",
        "sceBluetoothHidGetDeviceName","sceBluetoothHidGetReportDescriptor",
        "sceBluetoothHidGetInputReport","sceBluetoothHidGetFeatureReport",
        "sceBluetoothHidSetFeatureReport","sceBluetoothHidSetOutputReport",
        "sceBluetoothHidInterruptOutput","sceBluetoothHidDisconnectDevice",
        "sceBluetoothHidDebugGetVersion"
    };
    symbols("/system/common/lib/libSceBluetoothHid.sprx",bt,sizeof(bt)/sizeof(bt[0]),1);
    /* Pad library may depend on MBus before loading; only inspect exports. */
    int mbus=d4h_module("/system/common/lib/libSceMbus.sprx","sceMbusAcquireControl");
    if (mbus>=0) {
        static const char *const pad[]={"scePadInit","scePadSetProcessPrivilege",
            "scePadVirtualDeviceAddDevice","scePadVirtualDeviceInsertData",
            "scePadVirtualDeviceDeleteDevice"};
        symbols("/system/common/lib/libScePad.sprx",pad,sizeof(pad)/sizeof(pad[0]),0);
        d4h_log("release MBus=0x%x\n",d4h_release_module(mbus));
    }
#ifdef D4H_BT_OBSERVER
    return d4h_bt_observe();
#elif defined(D4H_USB_PROBE)
    return d4h_usb_probe();
#else
    d4h_log("Bluetooth ABI unverified: no init, pairing, callbacks or read calls attempted\n");
    return 0;
#endif
}
