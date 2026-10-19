/* SPDX-License-Identifier: GPL-3.0-only
 * OpenOrbis type header only; all console calls are injected mocks. */
#include "../platform/ps4/pad_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,opens,closes;
static int mode,pressed,l2,connected,read_error;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"PAD FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
void d4h_log(const char *fmt,...) { CHECK(fmt!=NULL); }
static int32_t get(int32_t user,uint32_t type,uint32_t index)
{ CHECK(user==42 && type==0 && index==0); return mode==0?0:-1; }
static int32_t open_pad(int32_t user,int32_t type,int32_t index,void *p)
{ CHECK(user==42 && type==0 && index==0 && !p); ++opens; return mode==2?-7:5; }
static int32_t close_pad(int32_t h) { CHECK(h==5); ++closes; return mode==3?-1:0; }
static int32_t read_pad(int32_t h,OrbisPadData *p)
{
    CHECK(h==0 || h==5); p->connected=(uint8_t)connected;
    p->buttons=pressed?ORBIS_PAD_BUTTON_CROSS:0; p->analogButtons.l2=(uint8_t)l2;
    return read_error;
}
int main(void)
{
    d4h_pad_read_api api={get,open_pad,close_pad,read_pad,0}; d4h_pad_read_slot s;
    mode=0; CHECK(d4h_pad_read_open(&api,&s,42)==0 && !s.owned && !opens);
    CHECK(d4h_pad_read_close(&api,&s)==0 && !closes); /* Never close borrowed fd/handle zero. */
    mode=1; CHECK(d4h_pad_read_open(&api,&s,42)==5 && s.owned && opens==1);
    connected=1; d4h_pad_read_tick(&api,&s,0,0);
    pressed=1; d4h_pad_read_tick(&api,&s,0,1);
    pressed=0; d4h_pad_read_tick(&api,&s,0,1);
    l2=255; d4h_pad_read_tick(&api,&s,0,2);
    CHECK(s.x_down[0]==0 && s.x_down[1]==1 && s.x_changes[1]==2 && s.l2_active[2]==1);
    connected=0; pressed=1; d4h_pad_read_tick(&api,&s,0,1);
    CHECK(s.x_down[1]==1 && s.connected==4);
    read_error=-1; d4h_pad_read_tick(&api,&s,0,1); CHECK(s.errors==1 && s.x_down[1]==1);
    CHECK(d4h_pad_read_close(&api,&s)==0 && closes==1 && !s.owned && s.handle==-1);
    mode=2; CHECK(d4h_pad_read_open(&api,&s,42)==-7 && !s.owned);
    CHECK(d4h_pad_read_close(&api,&s)==0 && closes==1);
    mode=3; CHECK(d4h_pad_read_open(&api,&s,42)==5);
    CHECK(d4h_pad_read_close(&api,&s)==-1 && s.owned && s.handle==5);
    mode=1; CHECK(d4h_pad_read_close(&api,&s)==0 && !s.owned);
    printf("PASS Pad snapshot: %u checks; own/borrowed handles, phases, errors and cleanup\n",checks);
    return 0;
}
