/* SPDX-License-Identifier: GPL-3.0-only
 * Independent architecture experiment; no sceUsbd, sceBluetoothHid, VDA,
 * Control4Free, module hooks, kernel patches or radio writes. */
#include "runtime.h"
#include "ds4hen/socket_gate.h"
#include <sys/socket.h>
#include <bluetooth.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
/* These assertions identify the proposed contract. They do NOT assert that
 * FreeBSD's Bluetooth domain was implemented in this Orbis kernel. */
_Static_assert(AF_INET==2 && SOCK_DGRAM==2 && IPPROTO_UDP==17,"IPv4 contract");
_Static_assert(AF_BLUETOOTH==36 && SOCK_RAW==3 && SOCK_SEQPACKET==5,"BSD BT contract");
_Static_assert(BLUETOOTH_PROTO_HCI==134 && BLUETOOTH_PROTO_L2CAP==135,"BSD protocols");
typedef struct { size_t size; char text[28]; uint32_t version; } sw_version;
_Static_assert(sizeof(sw_version)==40,"firmware query ABI");
typedef struct { int (*socket)(int,int,int); int *error; } native_io;
static int resolve_kernel(const char *name,void *dst,size_t n)
{
    if (!d4h_resolve(0x2001,name,dst,n)) return 0;
    return d4h_resolve(1,name,dst,n);
}
static int create_socket(void *context,int domain,int type,int protocol,int *error)
{
    native_io *io=context; *io->error=0;
    int fd=io->socket(domain,type,protocol);
    *error=*io->error; /* Snapshot before any logging or close. */
    return fd;
}
static int close_socket(void *context,int fd,int *error)
{
    native_io *io=context; *io->error=0;
    int ret=d4h_rt.close(fd); *error=*io->error; return ret;
}
static const char *meaning(const d4h_socket_result *r)
{
    if (!r->attempted) return "NOT_TESTED";
    if (r->fd>=0) return r->close_status?"CLOSE_FAILED":"SOCKET_CREATED_NOT_RADIO_PROVEN";
    if (r->create_errno==EAFNOSUPPORT) return "ADDRESS_FAMILY_UNAVAILABLE_IN_THIS_CONTEXT";
    if (r->create_errno==EPROTONOSUPPORT || r->create_errno==EPROTOTYPE)
        return "PROTOCOL_CONTRACT_UNAVAILABLE_IN_THIS_CONTEXT";
    if (r->create_errno==EPERM || r->create_errno==EACCES) return "PERMISSION_BLOCKED_NOT_ABSENCE_PROOF";
    return "OTHER_ERROR_UNRESOLVED";
}
int d4h_probe_main(void)
{
    int (*version)(sw_version *)=0; sw_version sw;
    memset(&sw,0,sizeof(sw)); sw.size=sizeof(sw);
    if (resolve_kernel("sceKernelGetSystemSwVersion",&version,sizeof(version))) return -10;
    int ret=version(&sw); sw.text[27]=0;
    d4h_log("native-bt-gate revision=1 firmware_status=0x%x firmware=0x%x text=%s\n",ret,sw.version,sw.text);
    if (ret || (sw.version>>16)!=0x1252) return -11;
    if (d4h_stopping()) { d4h_log("gate skipped: stop marker\n"); return -12; }
    native_io io; int *(*error_address)(void)=0;
    memset(&io,0,sizeof(io));
    if (resolve_kernel("socket",&io.socket,sizeof(io.socket)) ||
        resolve_kernel("__error",&error_address,sizeof(error_address)) || !(io.error=error_address())) {
        d4h_log("gate UNTESTED: native socket/errno exports unavailable; no fallback guessing\n"); return -40;
    }
    int saved_errno=*io.error;
    d4h_socket_io ops={&io,create_socket,close_socket};
    d4h_socket_result results[D4H_GATE_CASES];
    ret=d4h_socket_gate(&ops,results);
    for (unsigned i=0;i<D4H_GATE_CASES;++i) {
        const d4h_socket_contract *c=&d4h_socket_contracts[i];
        const d4h_socket_result *r=&results[i];
        d4h_log("gate case=%u name=%s domain=%d type=%d protocol=%d attempted=%d fd=%d create_errno=%d close=%d close_errno=%d outcome=%s\n",
            i,c->name,c->domain,c->type,c->protocol,r->attempted,r->fd,r->create_errno,
            r->close_status,r->close_errno,meaning(r));
    }
    d4h_log("native-bt-gate complete=%d; no bind/connect/ioctl/send/RegisterDevice; no pairing or HCI attachment proven\n",ret);
    *io.error=saved_errno;
    return ret;
}
