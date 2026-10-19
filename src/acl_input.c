/* SPDX-License-Identifier: GPL-3.0-only
 * Independent protocol implementation, not copied from another stack. */
#include "ds4hen/acl_input.h"
#include <string.h>
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
void d4h_acl_input_reset(d4h_acl_input *rx)
{
    if (rx) { rx->used=0; rx->expected=0; }
}
int d4h_acl_input_init(d4h_acl_input *rx,uint16_t handle,uint16_t cid)
{
    if (!rx || handle>0x0eff || cid<0x0040) return D4H_E_ARGUMENT;
    memset(rx,0,sizeof(*rx)); rx->handle=handle; rx->cid=cid; return 0;
}
static int fail(d4h_acl_input *rx,int error) { d4h_acl_input_reset(rx); return error; }
int d4h_acl_input_feed(d4h_acl_input *rx,const uint8_t *p,size_t n,d4h_state *out)
{
    if (!rx || !p || !out || rx->cid<0x0040 || rx->handle>0x0eff) return D4H_E_ARGUMENT;
    if (rx->used>rx->expected || rx->expected>sizeof(rx->payload)) return fail(rx,D4H_ACL_E_SEQUENCE);
    if (n<4 || le16(p+2)!=n-4) return fail(rx,D4H_ACL_E_FRAME);
    uint16_t flags=le16(p);
    if ((flags&0x0fff)!=rx->handle) return D4H_ACL_IGNORED;
    if (flags&0xc000) return fail(rx,D4H_ACL_E_FRAME); /* No broadcast. */
    unsigned pb=(flags>>12)&3; p+=4; n-=4;
    if (pb==0 || pb==2) {
        d4h_acl_input_reset(rx);
        if (n<4) return D4H_ACL_E_FRAME;
        size_t length=le16(p); uint16_t cid=le16(p+2); p+=4; n-=4;
        if (cid!=rx->cid) return D4H_ACL_IGNORED;
        if (length<2 || length>sizeof(rx->payload) || n>length) return D4H_ACL_E_FRAME;
        rx->expected=length;
    } else if (pb==1) {
        if (!rx->expected) return D4H_ACL_E_SEQUENCE;
    } else return fail(rx,D4H_ACL_E_FRAME);
    if ((!n && pb==1) || rx->used>rx->expected || n>rx->expected-rx->used)
        return fail(rx,D4H_ACL_E_FRAME);
    memcpy(rx->payload+rx->used,p,n); rx->used+=n;
    if (rx->used<rx->expected) return D4H_ACL_MORE;
    size_t length=rx->expected; d4h_acl_input_reset(rx);
    if (rx->payload[0]!=0xa1) return D4H_E_REPORT;
    return d4h_parse(D4H_BLUETOOTH,rx->payload+1,length-1,out);
}
