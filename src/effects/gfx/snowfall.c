#include "include_asset.h"

INCLUDE_IMG("effects/gfx/D_09000000_38D070.png", D_09000000_38D070);
INCLUDE_IMG("effects/gfx/D_09000080_38D0F0.png", D_09000080_38D0F0);
#include "effects/gfx/D_09000880_38D8F0.vtx.inc.c"
#include "effects/gfx/D_09000A00_38DA70.vtx.inc.c"

#include "effects/gfx/D_09000A80_38DAF0.vtx.inc.c"

#ifndef PORT
// todo wut dis
u8 D_09000C00_38DC70[] = { 0xE7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD7, 0x00, 0x10, 0x02, 0xFF, 0xFF, 0xFF, 0xFF };
#endif

#include "effects/gfx/D_09000C10_38DC80.gfx.inc.c"

#ifdef PORT
// raw N64 bytes can't run as port Gfx; same two commands, then the fall-through into 0xC10
Gfx D_09000C00_38DC70[] = {
    gsDPPipeSync(),
    gsSPTexture(0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON),
    gsSPBranchList(D_09000C10_38DC80),
};
#endif
#include "effects/gfx/D_09000D30_38DDA0.gfx.inc.c"
#include "effects/gfx/D_09000D50_38DDC0.gfx.inc.c"
