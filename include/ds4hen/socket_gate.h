/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef D4H_SOCKET_GATE_H
#define D4H_SOCKET_GATE_H
#include <stddef.h>
enum { D4H_GATE_CASES=3 };
typedef struct { const char *name; int domain,type,protocol; } d4h_socket_contract;
typedef struct { int attempted,fd,create_errno,close_status,close_errno; } d4h_socket_result;
typedef struct {
    void *context;
    int (*create)(void *,int,int,int,int *);
    int (*close)(void *,int,int *);
} d4h_socket_io;
/* Exactly three explicit contracts, not a scan. Values come from the PS4
 * payload SDK's inherited FreeBSD headers; availability on Orbis is UNKNOWN.
 * Creating a socket is not proof of an attached radio or a working channel. */
extern const d4h_socket_contract d4h_socket_contracts[D4H_GATE_CASES];
/* A failed IPv4 control or failed close aborts further attempts. No bind,
 * connect, ioctl, send, radio command or controller registration is exposed. */
int d4h_socket_gate(const d4h_socket_io *,d4h_socket_result out[D4H_GATE_CASES]);
#endif
