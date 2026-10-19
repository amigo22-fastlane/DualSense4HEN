/* SPDX-License-Identifier: GPL-3.0-only */
#include "ds4hen/socket_gate.h"
const d4h_socket_contract d4h_socket_contracts[D4H_GATE_CASES]={
    {"IPv4 UDP positive control",2,2,17},
    {"FreeBSD raw HCI contract",36,3,134},
    {"FreeBSD L2CAP SEQPACKET contract",36,5,135}
};
int d4h_socket_gate(const d4h_socket_io *io,d4h_socket_result out[D4H_GATE_CASES])
{
    if (!io || !io->create || !io->close || !out) return -1;
    for (unsigned i=0;i<D4H_GATE_CASES;++i)
        out[i]=(d4h_socket_result){0,-1,0,0,0};
    for (unsigned i=0;i<D4H_GATE_CASES;++i) {
        const d4h_socket_contract *c=&d4h_socket_contracts[i];
        d4h_socket_result *r=&out[i];
        r->attempted=1;
        r->fd=io->create(io->context,c->domain,c->type,c->protocol,&r->create_errno);
        if (r->fd>=0) {
            r->close_status=io->close(io->context,r->fd,&r->close_errno);
            if (r->close_status) return -3;
        } else if (i==0) return -2;
    }
    return 0; /* Experiment completed; not a Bluetooth success code. */
}
