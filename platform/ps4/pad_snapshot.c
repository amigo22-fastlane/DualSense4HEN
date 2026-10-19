/* SPDX-License-Identifier: GPL-3.0-only
 * Ordinary read client, not VirtualDevice registration or input injection. */
#include "runtime.h"
#include "pad_snapshot.h"
#include <string.h>
_Static_assert(sizeof(OrbisPadData)==120 && sizeof(OrbisPadInformation)==28,"OpenOrbis pad ABI");
int d4h_pad_read_open(const d4h_pad_read_api *api,d4h_pad_read_slot *slot,int32_t user)
{
    memset(slot,0,sizeof(*slot)); slot->handle=-1;
    int h=api->get_handle(user,ORBIS_PAD_PORT_TYPE_STANDARD,0);
    if (h<0) { h=api->open(user,ORBIS_PAD_PORT_TYPE_STANDARD,0,0); slot->owned=h>=0; }
    slot->handle=h; return h;
}
void d4h_pad_read_tick(const d4h_pad_read_api *api,d4h_pad_read_slot *slot,unsigned index,unsigned phase)
{
    if (slot->handle<0 || phase>2) return;
    OrbisPadData state; memset(&state,0,sizeof(state));
    int ret=api->read_state(slot->handle,&state); ++slot->reads;
    if (ret) { if (slot->errors++<3) d4h_log("PAD read user_slot=%u ret=0x%x\n",index,ret); return; }
    int x=(state.buttons&ORBIS_PAD_BUTTON_CROSS)!=0;
    int l2=state.analogButtons.l2>32;
    if (state.connected) {
        ++slot->connected; slot->x_down[phase]+=(unsigned)x;
        slot->l2_active[phase]+=(unsigned)l2;
        if (slot->seen_x && x!=slot->previous_x) ++slot->x_changes[phase];
    }
    if (!slot->seen_x || x!=slot->previous_x || l2!=slot->previous_l2 || state.connected!=slot->previous_connected) {
        if (slot->x_changes[0]+slot->x_changes[1]+slot->x_changes[2]<120)
            d4h_log("PAD snapshot user_slot=%u phase=%u connected=%u X=%d L2=%u buttons=0x%x timestamp=%llu\n",
                index,phase,state.connected,x,state.analogButtons.l2,state.buttons,(unsigned long long)state.timestamp);
    }
    slot->seen_x=1; slot->previous_x=x; slot->previous_l2=l2; slot->previous_connected=state.connected;
}
int d4h_pad_read_close(const d4h_pad_read_api *api,d4h_pad_read_slot *slot)
{
    if (slot->handle<0 || !slot->owned) return 0;
    int ret=api->close(slot->handle);
    if (!ret) { slot->owned=0; slot->handle=-1; }
    return ret;
}
