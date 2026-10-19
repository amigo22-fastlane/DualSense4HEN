/* SPDX-License-Identifier: GPL-3.0-only */
#include "ds4hen/bt_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"MONITOR FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static void fixture(d4h_bt_event *e,const char *name)
{
    char path[160]; snprintf(path,sizeof(path),"tests/fixtures/%s",name);
    FILE *f=fopen(path,"r"); CHECK(f!=NULL); unsigned byte; size_t n=0;
    memset(e,0,sizeof(*e));
    while (fscanf(f,"%x",&byte)==1) { CHECK(n<78 && byte<256); e->report[n++]=(uint8_t)byte; }
    CHECK(feof(f)); fclose(f); e->size=(uint16_t)n;
}
int main(void)
{
    d4h_bt_event e; d4h_state s,old;
    memset(&old,0x53,sizeof(old)); s=old;
    memset(&e,0,sizeof(e)); CHECK(d4h_bt_inspect(&e,&s)==D4H_BT_NO_INPUT && !memcmp(&s,&old,sizeof(s)));
    e.size=79; CHECK(d4h_bt_inspect(&e,&s)==D4H_E_SIZE && !memcmp(&s,&old,sizeof(s)));
    fixture(&e,"bt-extended.hex");
    CHECK(d4h_bt_inspect(&e,&s)==D4H_BT_FORMAT_CANDIDATE && (s.buttons&D4H_CROSS));
    e.vendor=0x054c; e.product=0x0ce6; CHECK(d4h_bt_inspect(&e,&s)==0);
    e.product=0x05c4; CHECK(d4h_bt_inspect(&e,&s)==D4H_BT_FORMAT_CANDIDATE); /* Shape is not identity. */
    e.report[20]^=1; s=old; CHECK(d4h_bt_inspect(&e,&s)==D4H_E_CRC && !memcmp(&s,&old,sizeof(s)));
    fixture(&e,"bt-simple.hex"); CHECK(d4h_bt_inspect(&e,&s)==D4H_BT_COMMON_UNATTRIBUTED);
    e.vendor=0x054c; e.product=0x0ce6; CHECK(d4h_bt_inspect(&e,&s)==0);
    e.size=2; s=old; CHECK(d4h_bt_inspect(&e,&s)<0 && !memcmp(&s,&old,sizeof(s)));
    CHECK(d4h_bt_inspect(NULL,&s)==D4H_E_ARGUMENT && d4h_bt_inspect(&e,NULL)==D4H_E_ARGUMENT);
    printf("PASS BT monitor: %u checks; metadata attribution, lengths, CRC and unchanged errors\n",checks);
    return 0;
}
