/* SPDX-License-Identifier: GPL-3.0-only
 * ABI recovered from the owner's 12.52 library, with explicit permission.
 * See docs/BLUETOOTH_ABI_1252.md. No firmware/kernel memory patches.
 * This observes HID events; it does not implement Bluetooth bonding. */
#include "runtime.h"
#include "ds4hen/hid.h"
#include <stdatomic.h>
#include <string.h>
typedef struct { void *address; uint32_t size; int32_t prot; } segment;
typedef struct {
    size_t size; char name[256]; segment segments[4];
    uint32_t segment_count; uint8_t fingerprint[20];
} module_info;
typedef struct {
    uint64_t device;
    uint16_t vendor,product;
    uint8_t type,status;
    uint16_t size;
    uint64_t reserved;
    uint8_t report[78],pad[2];
} bt_event;
_Static_assert(sizeof(module_info)==352,"module info ABI");
_Static_assert(sizeof(bt_event)==104 && offsetof(bt_event,report)==24,"BT callback ABI");
typedef void (*bt_callback)(const bt_event *,int,void *);
typedef struct {
    int32_t (*init)(void);
    int32_t (*callback)(bt_callback,void *,const void *);
    int32_t (*uncallback)(void);
    int32_t (*register_device)(uint16_t,uint16_t);
    int32_t (*unregister_device)(uint16_t,uint16_t);
    int32_t (*join)(void *,void **);
    int32_t (*info)(int32_t,module_info *);
} bt_api;
typedef struct { atomic_uint callbacks,reports,parsed,invalid; } observer;
static void events(const bt_event *e,int count,void *context)
{
    observer *o=context;
    if (!e || !o || count<0 || count>16) return;
    atomic_fetch_add_explicit(&o->callbacks,1,memory_order_relaxed);
    for (int i=0;i<count;++i) {
        unsigned n=atomic_fetch_add_explicit(&o->reports,1,memory_order_relaxed);
        if (n<64) d4h_log("BT event vendor=%04x product=%04x type=%u status=%u bytes=%u\n",
            e[i].vendor,e[i].product,e[i].type,e[i].status,e[i].size);
        if (e[i].vendor!=0x054c || e[i].product!=0x0ce6 || !e[i].size) continue;
        if (e[i].size>sizeof(e[i].report)) { atomic_fetch_add(&o->invalid,1); continue; }
        d4h_state state;
        int ret=d4h_parse(D4H_BLUETOOTH,e[i].report,e[i].size,&state);
        if (!ret) {
            unsigned good=atomic_fetch_add(&o->parsed,1);
            if (good<64) {
                d4h_hex("BT HID raw",e[i].report,e[i].size);
                d4h_log("BT parsed buttons=0x%x sticks=%u,%u,%u,%u triggers=%u,%u\n",
                    state.buttons,state.lx,state.ly,state.rx,state.ry,state.l2,state.r2);
            }
        } else {
            atomic_fetch_add(&o->invalid,1);
            if (n<64) d4h_log("BT HID rejected=%d\n",ret);
        }
    }
}
static int kernel_fn(const char *name,void *dst,size_t size)
{
    if (!d4h_resolve(0x2001,name,dst,size)) return 0;
    return d4h_resolve(1,name,dst,size);
}
int d4h_bt_observe(void)
{
    bt_api api; module_info info; observer o;
    memset(&api,0,sizeof(api)); memset(&info,0,sizeof(info));
    atomic_init(&o.callbacks,0); atomic_init(&o.reports,0); atomic_init(&o.parsed,0); atomic_init(&o.invalid,0);
    int h=d4h_module("/system/common/lib/libSceBluetoothHid.sprx","sceBluetoothHidInit");
    if (h<0) return -30;
    if (!d4h_module_owned(h)) { d4h_log("BT observer refuses a resident/shared HID context\n"); return -31; }
    if (D4H_RESOLVE(h,api,init,"sceBluetoothHidInit") ||
        D4H_RESOLVE(h,api,callback,"sceBluetoothHidRegisterCallback") ||
        D4H_RESOLVE(h,api,uncallback,"sceBluetoothHidUnregisterCallback") ||
#ifndef D4H_BT_PASSIVE
        D4H_RESOLVE(h,api,register_device,"sceBluetoothHidRegisterDevice") ||
        D4H_RESOLVE(h,api,unregister_device,"sceBluetoothHidUnregisterDevice") ||
#endif
        kernel_fn("scePthreadJoin",&api.join,sizeof(api.join)) ||
        kernel_fn("sceKernelGetModuleInfo",&api.info,sizeof(api.info))) return -32;
    info.size=sizeof(info);
    if (api.info(h,&info) || info.segment_count>4 || !info.segment_count) return -33;
    /* Build-specific READ-ONLY profile, not a patch address. Text hash checked
     * before using the documented worker handle slot, needed for a safe join. */
    uintptr_t base=(uintptr_t)info.segments[0].address;
    if (!base || info.segments[0].size!=16384 ||
        d4h_bt_crc((const uint8_t *)base,16384)!=0xb489ea13u) {
        d4h_log("BT ABI text fingerprint mismatch; refusing calls\n"); return -34;
    }
    uintptr_t worker_slot=base+0x8048;
    int readable=0;
    for (unsigned i=0;i<info.segment_count;++i) {
        uintptr_t start=(uintptr_t)info.segments[i].address;
        if ((info.segments[i].prot&1) && worker_slot>=start &&
            info.segments[i].size>=8 && worker_slot-start<=info.segments[i].size-8) readable=1;
    }
    if (!readable) return -35;
    int ret=api.init();
    d4h_log("sceBluetoothHidInit()=0x%x\n",ret);
    if (ret) return -36;
    /* No public Term export: retain the initialized library instead of
     * unloading a context with an open descriptor/mutex. One run per boot. */
    d4h_pin_module(h);
    ret=api.callback(events,&o,0);
    d4h_log("sceBluetoothHidRegisterCallback=0x%x\n",ret);
    if (ret) return -37;
    void *worker=0;
    memcpy(&worker,(const void *)worker_slot,sizeof(worker));
#ifdef D4H_BT_PASSIVE
    /* Audited callback registration creates its own event watcher before any
     * device filter is registered. Whether the driver routes events in this
     * mode remains a hardware question, not an assumed pairing mechanism. */
    d4h_log("BT passive observation 30s: no RegisterDevice, no pairing call\n");
    uint64_t begin=d4h_rt.time_us();
    for (unsigned i=0;i<300 && !d4h_stopping();++i) {
        uint64_t now=d4h_rt.time_us();
        if (now<begin || now-begin>=30000000u) break;
        d4h_rt.usleep(100000);
    }
#else
    ret=api.register_device(0x054c,0x0ce6);
    d4h_log("sceBluetoothHidRegisterDevice(054c,0ce6)=0x%x\n",ret);
    if (!ret) {
        d4h_log("BT observation window 180s: no pairing API is called; use normal PS4 pairing UI\n");
        uint64_t begin=d4h_rt.time_us();
        for (unsigned i=0;i<1800 && !d4h_stopping();++i) {
            uint64_t now=d4h_rt.time_us();
            if (now<begin || now-begin>=180000000u) break;
            d4h_rt.usleep(100000);
        }
        d4h_log("sceBluetoothHidUnregisterDevice=0x%x\n",api.unregister_device(0x054c,0x0ce6));
    }
#endif
    int unreg=api.uncallback();
    d4h_log("sceBluetoothHidUnregisterCallback=0x%x\n",unreg);
    void *thread_result=0;
    int joined=worker && !unreg ? api.join(worker,&thread_result) : -1;
    d4h_log("BT event worker join=0x%x\n",joined);
    if (joined) {
        d4h_log("BT teardown cannot prove callback lifetime; holding payload resident until console restart\n");
        for (;;) d4h_rt.usleep(1000000);
    }
    d4h_log("BT summary callbacks=%u events=%u parsed=%u invalid=%u\n",
        atomic_load(&o.callbacks),atomic_load(&o.reports),atomic_load(&o.parsed),atomic_load(&o.invalid));
    return ret ? -38 : 0;
}
