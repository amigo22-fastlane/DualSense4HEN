/* SPDX-License-Identifier: GPL-3.0-only
 * Syscall 591's signature is independently checked against
 * ps4-payload-dev/sdk crt/syscall.h. System function prototypes are checked
 * against OpenOrbis. This runtime does NOT link the SDK's patching CRT. */
#include "runtime.h"
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>

d4h_runtime d4h_rt;
static int log_fd=-1;
static int lock_fd=-1;
static struct { int handle,active,owned; } modules[8];
/* PS4 syscall numbers and console-write operation checked against the SDK's
 * target/include/sys/syscall.h and crt/klog.c, not assumed from Linux. */
static long syscall3(long number,long a,long b,long c)
{
    long result; unsigned char failed;
    register long r10 __asm__("r10")=0;
    register long r8 __asm__("r8")=0;
    register long r9 __asm__("r9")=0;
    __asm__ volatile("syscall; setc %1" : "=a"(result),"=qm"(failed),
                     "+r"(r10),"+r"(r8),"+r"(r9),"+d"(c)
                     : "a"(number),"D"(a),"S"(b)
                     : "rcx","r11","memory");
    return failed ? -result : result;
}
static void bootlog(const char *msg)
{
    (void)syscall3(601,7,(long)msg,0);
}
static void boothex(const char *label,uint64_t value)
{
    char out[100]; size_t n=0;
    while (label[n] && n<65) { out[n]=label[n]; ++n; }
    out[n++]='0'; out[n++]='x';
    for (unsigned i=0;i<16;++i) {
        unsigned digit=(unsigned)(value>>((15-i)*4))&15;
        out[n++]=(char)(digit<10?'0'+digit:'a'+digit-10);
    }
    out[n++]='\n'; out[n]=0; bootlog(out);
}
static void bootfile(const char *msg)
{
    size_t n=0;
    while (msg[n]) ++n;
    bootlog(msg);
    if (log_fd>=0) (void)syscall3(4,log_fd,(long)msg,(long)n);
}
void *memset(void *p,int value,size_t n)
{
    unsigned char *q=p;
    for (size_t i=0;i<n;++i) q[i]=(unsigned char)value;
    return p;
}
void *memcpy(void *p,const void *src,size_t n)
{
    unsigned char *q=p; const unsigned char *r=src;
    for (size_t i=0;i<n;++i) q[i]=r[i];
    return p;
}
static long lookup(int handle,const char *name,void **address)
{
    return syscall3(591,(long)handle,(long)name,(long)address);
}
int d4h_resolve(int handle,const char *symbol,void *dst,size_t size)
{
    void *p=0;
    if (size!=sizeof(p) || lookup(handle,symbol,&p)!=0 || !p) return -1;
    memcpy(dst,&p,sizeof(p));
    return 0;
}
static int kernel_symbol(const char *name,void *dst,size_t n)
{
    if (!d4h_resolve(0x2001,name,dst,n)) return 0;
    int ret=d4h_resolve(1,name,dst,n);
    if (ret) { bootfile("[d4h] unresolved kernel symbol: "); bootfile(name); bootfile("\n"); }
    return ret;
}
#define KSYM(member,name) kernel_symbol(name,&d4h_rt.member,sizeof(d4h_rt.member))
int d4h_module(const char *path,const char *anchor)
{
    int32_t handles[256]; size_t count=0; unsigned slot=0;
    int ret=d4h_rt.get_modules(handles,sizeof(handles),&count);
    if (ret || count>256) { d4h_log("module inventory failed=0x%x count=%zu; abort load\n",ret,count); return -1; }
    for (;slot<8 && modules[slot].active;++slot) {}
    if (slot==8) return -1;
    for (size_t i=0;i<count;++i) {
        void *p=0;
        if (!d4h_resolve(handles[i],anchor,&p,sizeof(p))) {
            modules[slot].handle=handles[i]; modules[slot].active=1; modules[slot].owned=0;
            d4h_log("borrow resident module %s handle=0x%x (will not stop/unload)\n",path,handles[i]);
            return handles[i];
        }
    }
    int result=0;
    int handle=(int32_t)d4h_rt.load_module(path,0,0,0,0,&result);
    d4h_log("load %s handle=0x%x start_result=0x%x\n",path,handle,result);
    if (handle>=0) {
        modules[slot].handle=handle; modules[slot].active=1; modules[slot].owned=1;
        /* Even when the anchor is unavailable, preserve an existing module. */
        for (size_t i=0;i<count;++i) if (handles[i]==handle) modules[slot].owned=0;
    }
    return handle;
}
int d4h_release_module(int h)
{
    for (unsigned i=0;i<8;++i) if (modules[i].active && modules[i].handle==h) {
        if (!modules[i].owned) { modules[i].active=0; return 0; }
        int result=0;
        int ret=d4h_rt.unload_module(h,0,0,0,0,&result);
        d4h_log("unload owned module=0x%x ret=0x%x result=0x%x\n",h,ret,result);
        if (!ret) modules[i].active=0;
        return ret;
    }
    return -1;
}
int d4h_module_owned(int h)
{
    for (unsigned i=0;i<8;++i) if (modules[i].active && modules[i].handle==h) return modules[i].owned;
    return 0;
}
void d4h_pin_module(int h)
{
    for (unsigned i=0;i<8;++i) if (modules[i].active && modules[i].handle==h) {
        modules[i].owned=0;
        d4h_log("module=0x%x pinned until host restart; no public BluetoothHidTerm export\n",h);
    }
}
static void write_all(const char *p,size_t n)
{
    while (n && log_fd>=0) {
        ssize_t done=d4h_rt.write(log_fd,p,n);
        if (done<=0 || (size_t)done>n) break;
        p+=done; n-=(size_t)done;
    }
}
void d4h_log(const char *fmt,...)
{
    char buf[768]; __builtin_va_list args;
    __builtin_va_start(args,fmt);
    int n=d4h_rt.vsnprintf(buf,sizeof(buf),fmt,args);
    __builtin_va_end(args);
    if (n>0) write_all(buf,(size_t)n<sizeof(buf)?(size_t)n:sizeof(buf)-1);
}
void d4h_hex(const char *label,const uint8_t *p,size_t n)
{
    static const char hex[]="0123456789abcdef";
    char line[3*128+1];
    if (n>128) n=128;
    for (size_t i=0;i<n;++i) {
        line[3*i]=hex[p[i]>>4]; line[3*i+1]=hex[p[i]&15]; line[3*i+2]=' ';
    }
    line[3*n]=0; d4h_log("%s %s\n",label,line);
}
int d4h_stopping(void)
{
    int fd=d4h_rt.open("/data/dualsense4hen.stop",O_RDONLY);
    if (fd<0) return 0;
    d4h_rt.close(fd); return 1;
}
/* Local ELF contains only relative relocations, no imports. Relocation is
 * idempotent if the loader already applied it. No process memory is patched
 * outside this ELF's own data segment. Build validation rejects other types. */
typedef struct { int64_t tag; uint64_t value; } dyn;
typedef struct { uint64_t offset,info; int64_t addend; } rela;
extern char __image_start[] __attribute__((visibility("hidden")));
extern char __image_end[] __attribute__((visibility("hidden")));
extern char __image_rw_start[] __attribute__((visibility("hidden")));
extern dyn _DYNAMIC[] __attribute__((visibility("hidden")));
static int relocate(void)
{
    uintptr_t base=(uintptr_t)__image_start;
    uintptr_t image_size=(uintptr_t)__image_end-base;
    uintptr_t rw_offset=(uintptr_t)__image_rw_start-base;
    uint64_t offset=0,size=0,entry=0;
    for (dyn *d=_DYNAMIC;d->tag && d<_DYNAMIC+128;++d) {
        if (d->tag==7) offset=d->value;
        if (d->tag==8) size=d->value;
        if (d->tag==9) entry=d->value;
    }
    boothex("[d4h] image base=",base);
    boothex("[d4h] dynamic=",(uintptr_t)_DYNAMIC);
    boothex("[d4h] DT_RELA=",offset);
    boothex("[d4h] DT_RELASZ=",size);
    boothex("[d4h] DT_RELAENT=",entry);
    if (!size) return 0;
    if (!offset || entry!=sizeof(rela) || size%entry ||
        offset>image_size || size>image_size-offset) return -1;
    rela *r=(rela *)(base+offset);
    for (size_t i=0;i<size/entry;++i) {
        if ((uint32_t)r[i].info!=8 || (r[i].info>>32)!=0) {
            boothex("[d4h] rejected relocation index=",i);
            boothex("[d4h] relocation info=",r[i].info);
            return -1;
        }
        if (r[i].offset<rw_offset || r[i].offset>image_size-8 ||
            r[i].addend<0 || (uint64_t)r[i].addend>=image_size) return -1;
    }
    for (size_t i=0;i<size/entry;++i)
        *(uintptr_t *)(base+r[i].offset)=base+(uintptr_t)r[i].addend;
    return 0;
}
int _start(void)
{
    bootlog("[d4h] bootstrap entered\n");
    if (relocate()) { bootlog("[d4h] relocation rejected\n"); return -1; }
    log_fd=(int)syscall3(5,(long)"/data/dualsense4hen-probe.log",O_WRONLY|O_CREAT|O_APPEND,0600);
    bootfile("[d4h] bootstrap relocated; resolving imports\n");
    memset(&d4h_rt,0,sizeof(d4h_rt));
    memset(modules,0,sizeof(modules));
    if (KSYM(open,"open") || KSYM(write,"write") || KSYM(read,"read") ||
        KSYM(close,"close") || KSYM(flock,"flock") || KSYM(usleep,"usleep") ||
        KSYM(get_modules,"sceKernelGetModuleList") ||
        KSYM(time_us,"sceKernelGetProcessTime") ||
        KSYM(load_module,"sceKernelLoadStartModule") ||
        KSYM(unload_module,"sceKernelStopUnloadModule")) {
        if (log_fd>=0) (void)syscall3(6,log_fd,0,0);
        log_fd=-1; return -2;
    }
    if (d4h_resolve(2,"vsnprintf",&d4h_rt.vsnprintf,sizeof(d4h_rt.vsnprintf))) {
        bootfile("[d4h] vsnprintf unavailable in module 2\n");
        if (log_fd>=0) d4h_rt.close(log_fd);
        log_fd=-1; return -3;
    }
    lock_fd=d4h_rt.open("/data/dualsense4hen-probe.lock",O_RDWR|O_CREAT,0600);
    if (lock_fd<0) { if (log_fd>=0) d4h_rt.close(log_fd); log_fd=-1; return -4; }
    if (d4h_rt.flock(lock_fd,LOCK_EX|LOCK_NB)) {
        d4h_rt.close(lock_fd); lock_fd=-1;
        if (log_fd>=0) d4h_rt.close(log_fd);
        log_fd=-1; return -5;
    }
    if (log_fd<0) { d4h_rt.close(lock_fd); lock_fd=-1; return -6; }
    d4h_log("\nDualSense4HEN diagnostic: userspace bootstrap, no SDK CRT or kernel patches\n");
    int ret=d4h_probe_main();
    d4h_log("probe exit=%d\n",ret);
    for (unsigned i=0;i<8;++i) if (modules[i].active) d4h_release_module(modules[i].handle);
    d4h_rt.close(log_fd); log_fd=-1;
    d4h_rt.close(lock_fd); lock_fd=-1;
    return ret; /* Never exit or kill the host daemon. */
}
