/* SPDX-License-Identifier: GPL-3.0-only
 * Synthetic protocol vectors and injected socket API, not PS4 measurements. */
#include "ds4hen/acl_input.h"
#include "ds4hen/socket_gate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"RESEARCH FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static unsigned created,closed;
static int scenario;
static int create_mock(void *context,int domain,int type,int protocol,int *error)
{
    CHECK(context==&scenario && created<D4H_GATE_CASES);
    const d4h_socket_contract *c=&d4h_socket_contracts[created];
    CHECK(domain==c->domain && type==c->type && protocol==c->protocol);
    unsigned index=created++;
    if (scenario==1 && index==0) { *error=13; return -1; }
    if (scenario==2 && index>0) { *error=47; return -1; }
    if (scenario==3 && index==1) { *error=1; return -1; }
    *error=0; return (int)index; /* Descriptor zero is valid. */
}
static int close_mock(void *context,int fd,int *error)
{
    CHECK(context==&scenario && fd>=0 && fd<3); ++closed;
    if (scenario==4) { *error=5; return -1; }
    *error=0; return 0;
}
static void sockets(void)
{
    CHECK(d4h_socket_contracts[0].domain==2 && d4h_socket_contracts[0].type==2 && d4h_socket_contracts[0].protocol==17);
    CHECK(d4h_socket_contracts[1].domain==36 && d4h_socket_contracts[1].type==3 && d4h_socket_contracts[1].protocol==134);
    CHECK(d4h_socket_contracts[2].domain==36 && d4h_socket_contracts[2].type==5 && d4h_socket_contracts[2].protocol==135);
    d4h_socket_result r[3]; d4h_socket_io io={&scenario,create_mock,close_mock};
    for (scenario=0;scenario<=4;++scenario) {
        created=closed=0; int ret=d4h_socket_gate(&io,r);
        if (scenario==1) CHECK(ret==-2 && created==1 && !closed && !r[1].attempted && !r[2].attempted);
        else if (scenario==4) CHECK(ret==-3 && created==1 && closed==1 && r[0].close_errno==5 && !r[1].attempted);
        else {
            CHECK(ret==0 && created==3 && r[0].fd==0);
            CHECK(closed==(scenario==2?1u:scenario==3?2u:3u));
            if (scenario==2) CHECK(r[1].create_errno==47 && r[2].create_errno==47);
            if (scenario==3) CHECK(r[1].create_errno==1 && r[2].fd==2);
        }
    }
    CHECK(d4h_socket_gate(NULL,r)==-1 && d4h_socket_gate(&io,NULL)==-1);
    io.close=NULL; CHECK(d4h_socket_gate(&io,r)==-1);
}
static void u16(uint8_t *p,uint16_t n) { p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8); }
static size_t frame(uint8_t *out,unsigned pb,uint16_t handle,const uint8_t *p,size_t n)
{
    CHECK(n<=83); u16(out,(uint16_t)(handle|(pb<<12))); u16(out+2,(uint16_t)n);
    memcpy(out+4,p,n); return n+4;
}
static size_t fixture(uint8_t *p)
{
    FILE *f=fopen("tests/fixtures/bt-extended.hex","r"); CHECK(f!=NULL);
    size_t n=0; unsigned byte;
    while (fscanf(f,"%x",&byte)==1) { CHECK(n<78 && byte<256); p[n++]=(uint8_t)byte; }
    CHECK(feof(f) && n==78); fclose(f); return n;
}
static void acl(void)
{
    uint8_t pdu[83],packet[87],report[78];
    CHECK(fixture(report)==78);
    u16(pdu,79); u16(pdu+2,0x0041); pdu[4]=0xa1; memcpy(pdu+5,report,78);
    d4h_state expected,out,unchanged;
    CHECK(d4h_parse(D4H_BLUETOOTH,report,78,&expected)==0);
    memset(&unchanged,0x53,sizeof(unchanged));
    d4h_acl_input rx;
    CHECK(d4h_acl_input_init(&rx,0x002a,0x0041)==0);
    CHECK(d4h_acl_input_init(&rx,0x0f00,0x0041)==D4H_E_ARGUMENT);
    CHECK(d4h_acl_input_init(&rx,0x002a,0x0013)==D4H_E_ARGUMENT); /* PSM != CID. */
    for (unsigned pb=0;pb<=2;pb+=2) for (size_t split=0;split<=79;++split) {
        CHECK(d4h_acl_input_init(&rx,0x002a,0x0041)==0); out=unchanged;
        size_t n=frame(packet,pb,0x002a,pdu,4+split);
        int ret=d4h_acl_input_feed(&rx,packet,n,&out);
        if (split<79) {
            CHECK(ret==D4H_ACL_MORE && !memcmp(&out,&unchanged,sizeof(out)));
            n=frame(packet,1,0x002a,pdu+4+split,79-split);
            CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==0);
        } else CHECK(ret==0);
        CHECK(!memcmp(&out,&expected,sizeof(out)) && !rx.expected && !rx.used);
    }
    /* Every byte may arrive in its own ACL continuation. */
    CHECK(d4h_acl_input_init(&rx,0x002a,0x0041)==0);
    size_t n=frame(packet,2,0x002a,pdu,4); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_MORE);
    for (size_t i=0;i<79;++i) {
        n=frame(packet,1,0x002a,pdu+4+i,1);
        CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==(i==78?0:D4H_ACL_MORE));
    }
    CHECK(!memcmp(&out,&expected,sizeof(out)));
    out=unchanged;
    /* A packet from another connection cannot cancel a pending valid frame. */
    n=frame(packet,2,0x002a,pdu,10); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_MORE);
    n=frame(packet,2,0x002b,pdu,83); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_IGNORED && rx.used==6);
    n=frame(packet,1,0x002a,pdu+10,73); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==0);
    CHECK(!memcmp(&out,&expected,sizeof(out))); out=unchanged;
    n=frame(packet,2,0x002a,pdu,10); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_MORE);
    n=frame(packet,1,0x002a,pdu,74); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_E_FRAME && !rx.expected);
    CHECK(!memcmp(&out,&unchanged,sizeof(out)));
    n=frame(packet,2,0x002a,pdu,10); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_MORE);
    n=frame(packet,2,0x002a,pdu,83); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==0); /* Fresh first replaces partial. */
    out=unchanged;
    n=frame(packet,2,0x002a,pdu,10); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_MORE);
    n=frame(packet,1,0x002a,pdu,0); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_E_FRAME && !rx.expected);
    n=frame(packet,2,0x002a,pdu,3); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_E_FRAME);
    n=frame(packet,1,0x002a,pdu+4,1); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_E_SEQUENCE);
    n=frame(packet,2,0x002b,pdu,83); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_IGNORED);
    u16(pdu+2,0x0042); n=frame(packet,2,0x002a,pdu,83); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_IGNORED);
    u16(pdu+2,0x0041); u16(pdu,80); n=frame(packet,2,0x002a,pdu,83);
    CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_E_FRAME);
    u16(pdu,79); n=frame(packet,3,0x002a,pdu,83); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_E_FRAME);
    n=frame(packet,2,0x002a,pdu,83); packet[1]|=0x40; CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_ACL_E_FRAME);
    n=frame(packet,2,0x002a,pdu,83); CHECK(d4h_acl_input_feed(&rx,packet,n-1,&out)==D4H_ACL_E_FRAME);
    pdu[4]=0xa2; n=frame(packet,2,0x002a,pdu,83); CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_E_REPORT);
    pdu[4]=0xa1; pdu[20]^=1; n=frame(packet,2,0x002a,pdu,83);
    CHECK(d4h_acl_input_feed(&rx,packet,n,&out)==D4H_E_CRC);
    CHECK(!memcmp(&out,&unchanged,sizeof(out)));
    CHECK(d4h_acl_input_feed(NULL,packet,n,&out)==D4H_E_ARGUMENT);
    /* Deterministic hostile framing with no output changes on errors. */
    uint32_t random=0x6252u;
    for (unsigned i=0;i<10000;++i) {
        for (size_t j=0;j<sizeof(packet);++j) { random=random*1664525u+1013904223u; packet[j]=(uint8_t)(random>>24); }
        out=unchanged; int ret=d4h_acl_input_feed(&rx,packet,random%sizeof(packet),&out);
        if (ret) CHECK(!memcmp(&out,&unchanged,sizeof(out)));
        CHECK(rx.used<=rx.expected && rx.expected<=sizeof(rx.payload));
    }
}
int main(void)
{
    sockets(); acl();
    printf("PASS transport research: %u checks; mocked contracts and synthetic ACL/HIDP, not hardware\n",checks);
    return 0;
}
