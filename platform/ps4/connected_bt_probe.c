/* SPDX-License-Identifier: GPL-3.0-only
 * Existing-session diagnostic: never pairs, disconnects, registers a HID
 * device, injects input, changes privilege or invokes controller output. */
#include "runtime.h"
#include "pad_snapshot.h"
#include <orbis/libkernel.h>
#include <orbis/_types/user.h>
#include <string.h>
#ifndef D4H_PAD_ONLY
int d4h_bt_observe(void);
extern void (*d4h_bt_tick)(uint64_t);
#endif
typedef struct { size_t size; char text[28]; uint32_t version; } sw_version;
static d4h_pad_read_api pad;
static d4h_pad_read_slot slots[4];
static int phase_previous=-1;
static int32_t (*notification)(int32_t,OrbisNotificationRequest *,size_t,int32_t);
static int kernel_fn(const char *name,void *dst,size_t size)
{
    if (!d4h_resolve(0x2001,name,dst,size)) return 0;
    return d4h_resolve(1,name,dst,size);
}
static void notify(const char *text)
{
    if (!notification) return;
    OrbisNotificationRequest request; memset(&request,0,sizeof(request));
    request.targetId=-1;
    size_t n=0; while (text[n] && n<sizeof(request.message)-1) ++n;
    memcpy(request.message,text,n);
    d4h_log("notification ret=0x%x\n",notification(0,&request,sizeof(request),0));
}
static void tick(uint64_t elapsed)
{
    unsigned phase=elapsed<5000000u?0:elapsed<20000000u?1:2;
    if ((int)phase!=phase_previous) {
        phase_previous=(int)phase;
        d4h_log("connected-bt phase=%u time_ms=%llu\n",phase,(unsigned long long)(elapsed/1000));
        if (phase==0) notify("DualSense4HEN: leave both controllers idle for 5 seconds. Keep Bluetooth connected.");
        if (phase==1) notify("DualSense4HEN: PS5 DualSense - press/release X for 15 seconds. Do not press PS.");
        if (phase==2) notify("DualSense4HEN: PS4 DualShock 4 - squeeze/release L2 for 15 seconds. Do not press X or PS.");
    }
    for (unsigned i=0;i<4;++i) d4h_pad_read_tick(&pad,&slots[i],i,phase);
}
static void prepare_pad(void)
{
    for (unsigned i=0;i<4;++i) slots[i].handle=-1;
    int u=d4h_module("/system/common/lib/libSceUserService.sprx","sceUserServiceGetLoginUserIdList");
    if (u<0) { d4h_log("PAD view UNTESTED: UserService load\n"); return; }
    int32_t (*users)(OrbisUserServiceLoginUserIdList *)=0;
    int32_t (*initialize)(void *)=0;
    int32_t (*terminate)(void)=0;
    int initialized=0,owned=d4h_module_owned(u),ret=-1;
    if (d4h_resolve(u,"sceUserServiceGetLoginUserIdList",&users,sizeof(users))) goto done_users;
    OrbisUserServiceLoginUserIdList list; for (unsigned i=0;i<4;++i) list.userId[i]=-1;
    ret=users(&list);
    d4h_log("UserService login-list before Init ret=0x%x (read-only query)\n",ret);
    if (!ret && owned) d4h_keep_module(u,"existing UserService state observed; not owned initialization");
    if (ret && owned) {
        if (d4h_resolve(u,"sceUserServiceInitialize",&initialize,sizeof(initialize)) ||
            d4h_resolve(u,"sceUserServiceTerminate",&terminate,sizeof(terminate))) goto done_users;
        OrbisUserServiceInitializeParams params={ORBIS_KERNEL_PRIO_FIFO_NORMAL};
        ret=initialize(&params); d4h_log("UserService own Init=0x%x\n",ret);
        if (ret) {
            d4h_keep_module(u,"UserService Init failed; existing state ownership unresolved");
            goto done_users;
        }
        initialized=1;
        ret=users(&list);
    }
    d4h_log("UserService login-list ret=0x%x (no account names queried)\n",ret);
    if (!ret) {
        int mbus=d4h_module("/system/common/lib/libSceMbus.sprx","sceMbusAcquireControl");
        if (mbus<0) goto done_users;
        int h=d4h_module("/system/common/lib/libScePad.sprx","scePadInit");
        if (h<0) goto done_users;
        int32_t (*init_pad)(void)=0;
        if (d4h_resolve(h,"scePadInit",&init_pad,sizeof(init_pad)) ||
            D4H_RESOLVE(h,pad,get_handle,"scePadGetHandle") || D4H_RESOLVE(h,pad,open,"scePadOpen") ||
            D4H_RESOLVE(h,pad,close,"scePadClose") || D4H_RESOLVE(h,pad,read_state,"scePadReadState") ||
            D4H_RESOLVE(h,pad,info,"scePadGetControllerInformation")) goto done_users;
        ret=init_pad(); d4h_log("scePadInit=0x%x; no SetProcessPrivilege or VirtualDevice calls\n",ret);
        if (ret) goto done_users;
        if (d4h_module_owned(h)) d4h_keep_module(h,"initialized Pad client; no audited Term API");
        if (d4h_module_owned(mbus)) d4h_keep_module(mbus,"dependency of initialized Pad client");
        for (unsigned i=0;i<4;++i) if (list.userId[i]!=-1) {
            int handle=d4h_pad_read_open(&pad,&slots[i],list.userId[i]);
            d4h_log("PAD client user_slot=%u handle=0x%x owned=%d; not a global device inventory\n",i,handle,slots[i].owned);
            if (handle>=0) {
                OrbisPadInformation info; memset(&info,0,sizeof(info));
                int status=pad.info(handle,&info);
                d4h_log("PAD info user_slot=%u ret=0x%x connected=%d class=%d connection_type_raw=%u count_raw=%u\n",
                    i,status,info.connected,info.deviceClass,info.connectionType,info.count);
            }
        }
    }
done_users:
    if (initialized) {
        int term=terminate(); d4h_log("UserService own Term=0x%x\n",term);
        if (term) d4h_keep_module(u,"UserService termination failed; preserve initialized code");
    }
    (void)d4h_release_module(u);
}
int d4h_probe_main(void)
{
    int (*get_sw)(sw_version *)=0; sw_version sw;
    memset(&sw,0,sizeof(sw)); sw.size=sizeof(sw);
    if (kernel_fn("sceKernelGetSystemSwVersion",&get_sw,sizeof(get_sw))) return -10;
    int ret=get_sw(&sw); sw.text[27]=0;
    d4h_log("connected-bt revision=2 firmware_status=0x%x firmware=0x%x text=%s\n",ret,sw.version,sw.text);
    if (ret || (sw.version>>16)!=0x1252) return -11;
    if (d4h_stopping()) return -12;
    d4h_log("existing-session only: L2CAP Control/Interrupt UNKNOWN; no pairing/disconnect/RegisterDevice/SetReport/VDA\n");
    (void)kernel_fn("sceKernelSendNotificationRequest",&notification,sizeof(notification));
    prepare_pad();
#ifdef D4H_PAD_ONLY
    ret=0;
    d4h_log("Pad-only measurement: Bluetooth subscription NOT attempted; channel states UNKNOWN\n");
#else
    d4h_bt_tick=tick;
    ret=d4h_bt_observe();
    d4h_bt_tick=0;
#endif
    if (phase_previous<0) {
#ifndef D4H_PAD_ONLY
        d4h_log("BT subscriber unavailable ret=%d; run Pad-only control; BT states remain UNKNOWN\n",ret);
#endif
        uint64_t begin=d4h_rt.time_us();
        for (unsigned i=0;i<350 && !d4h_stopping();++i) {
            uint64_t now=d4h_rt.time_us();
            if (now<begin || now-begin>=35000000u) break;
            tick(now-begin); d4h_rt.usleep(100000);
        }
    }
    for (unsigned i=0;i<4;++i) if (slots[i].handle>=0) {
        d4h_log("PAD summary user_slot=%u reads=%u errors=%u connected_reads=%u idle_X=%u DualSense_phase_X=%u DS4_control_L2=%u\n",
            i,slots[i].reads,slots[i].errors,slots[i].connected,slots[i].x_down[0],slots[i].x_down[1],slots[i].l2_active[2]);
        int closed=d4h_pad_read_close(&pad,&slots[i]);
        d4h_log("PAD close own user_slot=%u ret=0x%x; borrowed handles are preserved\n",i,closed);
        if (closed) d4h_log("PAD close unresolved: own client handle retained; no disconnect requested\n");
    }
    notify("DualSense4HEN: existing Bluetooth session test finished. Keep the DualSense connected.");
    d4h_log("connected-bt complete=%d; absence of client events is not absence of L2CAP or system registration\n",ret);
    return ret;
}
