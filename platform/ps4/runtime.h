/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef D4H_RUNTIME_H
#define D4H_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
/* Minimal user-space probe runtime. No SDK CRT, kernel read/write, offsets,
 * credential changes or automatic jailbreak calls. */
typedef struct {
    int (*open)(const char *,int,...);
    ssize_t (*write)(int,const void *,size_t);
    ssize_t (*read)(int,void *,size_t);
    int (*close)(int);
    int (*flock)(int,int);
    int32_t (*get_modules)(int32_t *,size_t,size_t *);
    int (*usleep)(unsigned);
    uint64_t (*time_us)(void);
    uint32_t (*load_module)(const char *,size_t,const void *,uint32_t,void *,void *);
    int (*unload_module)(int,size_t,const void *,uint32_t,void *,void *);
    int (*vsnprintf)(char *,size_t,const char *,__builtin_va_list);
} d4h_runtime;
extern d4h_runtime d4h_rt;
int d4h_resolve(int handle,const char *symbol,void *destination,size_t pointer_size);
int d4h_module(const char *path,const char *anchor_symbol);
int d4h_release_module(int handle);
int d4h_module_owned(int handle);
void d4h_pin_module(int handle);
void d4h_log(const char *fmt,...);
void d4h_hex(const char *label,const uint8_t *data,size_t size);
int d4h_stopping(void);
int d4h_probe_main(void);
#define D4H_RESOLVE(h,api,member,name) d4h_resolve(h,name,&(api).member,sizeof((api).member))
#endif
