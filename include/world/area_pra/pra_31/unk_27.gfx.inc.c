// Extracted from ROM by extract_gfx.py
Gfx N(unk_27_gfx)[] = {
    {(uintptr_t)0xDB020000, (uintptr_t)0x00000018},  // gsMoveWd
    {(uintptr_t)0xDC08060A, (uintptr_t)((u8*)&N(stairs_lights) + 0x8)},  // gsSPMoveMem -> N(stairs_lights) + 0x8
    {(uintptr_t)0xDC08090A, (uintptr_t)&N(stairs_lights)},  // gsSPMoveMem -> N(stairs_lights)
    {(uintptr_t)0xE7000000, (uintptr_t)0x00000000},  // gsDPPipeSync
    {(uintptr_t)0xE3000A01, (uintptr_t)0x00000000},  // gsDPSetOtherMode_L
    {(uintptr_t)0xE200001C, (uintptr_t)0x00552078},  // gsDPSetOtherMode_H
    {(uintptr_t)0xDE000000, (uintptr_t)N(texture_2_gfx)},  // gsSPDisplayList -> N(texture_2_gfx)
    {(uintptr_t)0xD9FDFFFF, (uintptr_t)0x00000000},  // gsSPGeometryMode
    {(uintptr_t)0xD9FFFFFF, (uintptr_t)0x00200400},  // gsSPGeometryMode
    {(uintptr_t)0x01001002, (uintptr_t)&N(stairs_vtx)[6]},  // gsSPVertex -> N(stairs_vtx) + 0x60
    {(uintptr_t)0x01003008, (uintptr_t)&N(stairs_vtx)[10]},  // gsSPVertex -> N(stairs_vtx) + 0xA0
    {(uintptr_t)0x06020406, (uintptr_t)0x00020600},  // gsSP2Triangles
    {(uintptr_t)0xDF000000, (uintptr_t)0x00000000},  // gsSPEndDisplayList
};
