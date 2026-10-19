/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef D4H_ACL_INPUT_H
#define D4H_ACL_INPUT_H
#include "hid.h"
enum { D4H_ACL_IGNORED=1,D4H_ACL_MORE=2,D4H_ACL_E_FRAME=-20,D4H_ACL_E_SEQUENCE=-21 };
typedef struct {
    uint16_t handle,cid;
    size_t used,expected;
    uint8_t payload[79]; /* HIDP A1 + longest supported DualSense report. */
} d4h_acl_input;
/* Caller must supply the real connection handle and negotiated *local*
 * interrupt CID. PSM 0x13 is not a CID. Basic L2CAP mode only, no ERTM,
 * signaling, encryption, SDP, pairing or live PS4 radio backend. */
int d4h_acl_input_init(d4h_acl_input *,uint16_t handle,uint16_t local_cid);
void d4h_acl_input_reset(d4h_acl_input *);
/* HCI ACL header + payload, without H4 packet-type byte. On nonzero return
 * out is unchanged; buffers are bounded; unrelated handles are ignored. */
int d4h_acl_input_feed(d4h_acl_input *,const uint8_t *,size_t,d4h_state *out);
#endif
