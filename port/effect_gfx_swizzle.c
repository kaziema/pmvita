/**
 * effect_gfx_swizzle.c - Load and convert effect graphics from ROM to PC format
 *
 * On N64, effect graphics (display lists, vertices, textures) are DMA'd from ROM
 * into a buffer, then accessed via segment 9 addressing. On PC, the effect code
 * references D_09xxxxxx symbols as direct C pointers to arrays.
 *
 * This module:
 *   1. Loads the effect graphics blob from ROM
 *   2. Copies texture data (no byte-swap - Fast3D handles N64 endian)
 *   3. Byte-swaps vertex data (N64 big-endian → PC native)
 *   4. Converts display lists (N64 8-byte → PC 16-byte Gfx, resolving pointers)
 *   5. Copies everything into the correctly-sized C arrays
 *
 * After swizzle, the C arrays contain valid PC-format data and all internal
 * display list pointers reference the C arrays directly (not the ROM blob).
 */

#ifdef PORT

#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "port/endian.h"
#include "port/effect_gfx_tables.h"

// N64 struct sizes
#define N64_GFX_CMD_SIZE  8
#define N64_VTX_SIZE      16

// F3DEX2 GBI opcodes
#define F3DEX2_G_VTX       0x01
#define F3DEX2_G_TRI1      0x05
#define F3DEX2_G_TRI2      0x06
#define F3DEX2_G_MTX       0xDA
#define F3DEX2_G_MOVEWORD  0xDB
#define F3DEX2_G_MOVEMEM   0xDC
#define F3DEX2_G_DL        0xDE
#define F3DEX2_G_ENDDL     0xDF
#define F3DEX2_G_SETTIMG   0xFD
#define F3DEX2_G_SETCIMG   0xFF
#define F3DEX2_G_LOADTLUT  0xF0

// ROM reading
extern void nuPiReadRom(u32 romAddr, void* dest, u32 size);

/**
 * Resolve a segment 9 address to its corresponding C array pointer.
 * Returns NULL if the address doesn't match any known symbol.
 */
static void* resolve_seg9_addr(u32 seg9Addr, const EffectGfxSymbol* symbols, s32 numSymbols) {
    // Only resolve segment 9 addresses
    u8 segment = (seg9Addr >> 24) & 0xFF;
    if (segment != 0x09) {
        return NULL;
    }
    u32 offset = seg9Addr & 0x00FFFFFF;

    for (s32 i = 0; i < numSymbols; i++) {
        if (symbols[i].seg9Offset == offset) {
            return symbols[i].pcArray;
        }
    }

    // Address doesn't match a symbol exactly — find the containing symbol
    // (for addresses within a symbol's range, like vertex data mid-array)
    for (s32 i = 0; i < numSymbols; i++) {
        u32 symStart = symbols[i].seg9Offset;
        u32 symEnd = symStart + symbols[i].sizeBytes;
        if (offset >= symStart && offset < symEnd) {
            u32 innerOffset = offset - symStart;
            return (u8*)symbols[i].pcArray + innerOffset;
        }
    }

    return NULL;
}

/**
 * Convert N64 big-endian vertices and copy into a PC Vtx array.
 */
static void swizzle_vertices(const u8* blob, const EffectGfxSymbol* sym) {
    s32 numVerts = sym->sizeBytes / N64_VTX_SIZE;
    Vtx* destVtx = (Vtx*)sym->pcArray;
    const u8* src = blob + sym->seg9Offset;

    memset(destVtx, 0, numVerts * sizeof(Vtx));

    for (s32 i = 0; i < numVerts; i++) {
        const u8* v = src + i * N64_VTX_SIZE;

        // Position: s16 big-endian → float (GBI_FLOATS)
        s16 x = (s16)read_be_u16(v + 0);
        s16 y = (s16)read_be_u16(v + 2);
        s16 z = (s16)read_be_u16(v + 4);
        destVtx[i].v.ob[0] = (float)x;
        destVtx[i].v.ob[1] = (float)y;
        destVtx[i].v.ob[2] = (float)z;

        // Flag: u16 big-endian
        destVtx[i].v.flag = read_be_u16(v + 6);

        // Texture coords: s16 big-endian
        destVtx[i].v.tc[0] = (s16)read_be_u16(v + 8);
        destVtx[i].v.tc[1] = (s16)read_be_u16(v + 10);

        // Color/normal: 4 × u8 (no swap needed)
        destVtx[i].v.cn[0] = v[12];
        destVtx[i].v.cn[1] = v[13];
        destVtx[i].v.cn[2] = v[14];
        destVtx[i].v.cn[3] = v[15];
    }
}

/**
 * Convert an N64 display list from the blob and write into a PC Gfx array.
 * Resolves internal segment 9 addresses to C array pointers.
 */
static void swizzle_display_list(const u8* blob, u32 blobSize,
                                  const EffectGfxSymbol* sym,
                                  const EffectGfxSymbol* allSymbols, s32 numSymbols) {
    s32 numCmds = sym->sizeBytes / N64_GFX_CMD_SIZE;
    Gfx* dest = (Gfx*)sym->pcArray;
    const u8* src = blob + sym->seg9Offset;

    memset(dest, 0, numCmds * sizeof(Gfx));

    for (s32 i = 0; i < numCmds; i++) {
        const u8* cmd = src + i * N64_GFX_CMD_SIZE;
        u32 w0 = read_be_u32(cmd);
        u32 w1 = read_be_u32(cmd + 4);
        u8 opcode = (w0 >> 24) & 0xFF;

        switch (opcode) {
            case F3DEX2_G_VTX: {
                // w1: segment 9 vertex address → resolve to C VTX array
                void* pcVtx = resolve_seg9_addr(w1, allSymbols, numSymbols);
                dest[i].words.w0 = (uintptr_t)w0;
                dest[i].words.w1 = pcVtx ? (uintptr_t)pcVtx : (uintptr_t)w1;
                break;
            }

            case F3DEX2_G_DL: {
                // w1: segment 9 sub-DL address → resolve to C GFX array
                void* pcSubDL = resolve_seg9_addr(w1, allSymbols, numSymbols);
                dest[i].words.w0 = (uintptr_t)w0;
                dest[i].words.w1 = pcSubDL ? (uintptr_t)pcSubDL : (uintptr_t)w1;
                break;
            }

            case F3DEX2_G_SETTIMG: {
                // w1: segment 9 texture address → resolve to C IMG array
                void* pcTex = resolve_seg9_addr(w1, allSymbols, numSymbols);
                dest[i].words.w0 = (uintptr_t)w0;
                dest[i].words.w1 = pcTex ? (uintptr_t)pcTex : (uintptr_t)w1;
                break;
            }

            case F3DEX2_G_SETCIMG: {
                // Color image address — resolve if within blob
                void* pcCimg = resolve_seg9_addr(w1, allSymbols, numSymbols);
                dest[i].words.w0 = (uintptr_t)w0;
                dest[i].words.w1 = (uintptr_t)(pcCimg ? pcCimg : (void*)(uintptr_t)w1);
                break;
            }

            case F3DEX2_G_MTX: {
                // Matrix address — shouldn't appear in effect DLs typically
                void* pcMtx = resolve_seg9_addr(w1, allSymbols, numSymbols);
                dest[i].words.w0 = (uintptr_t)w0;
                dest[i].words.w1 = (uintptr_t)(pcMtx ? pcMtx : (void*)(uintptr_t)w1);
                break;
            }

            case F3DEX2_G_MOVEMEM: {
                // Data address (lights, viewport)
                void* pcData = resolve_seg9_addr(w1, allSymbols, numSymbols);
                dest[i].words.w0 = (uintptr_t)w0;
                dest[i].words.w1 = (uintptr_t)(pcData ? pcData : (void*)(uintptr_t)w1);
                break;
            }

            default:
                // All other commands: copy w0 and w1 as-is
                // This handles triangle commands, render mode, combine mode, etc.
                dest[i].words.w0 = (uintptr_t)w0;
                dest[i].words.w1 = (uintptr_t)w1;
                break;
        }
    }

    // Some lists have no G_ENDDL and simply run into the next chunk, which works on N64
    // because segment 9 is one contiguous block. Here every chunk is its own array, so the
    // fall-through has to be restored as an explicit branch into the following list.
    {
        s32 hasEnd = 0;
        for (s32 i = 0; i < numCmds; i++) {
            if ((read_be_u32(src + i * N64_GFX_CMD_SIZE) >> 24) == F3DEX2_G_ENDDL) {
                hasEnd = 1;
                break;
            }
        }

        if (!hasEnd) {
            const EffectGfxSymbol* next = NULL;
            for (s32 i = 0; i < numSymbols; i++) {
                if (allSymbols[i].seg9Offset == sym->seg9Offset + sym->sizeBytes) {
                    next = &allSymbols[i];
                    break;
                }
            }

            if (next != NULL && next->type == EFX_SYM_GFX) {
                dest[numCmds].words.w0 = (uintptr_t)((F3DEX2_G_DL << 24) | (1 << 16)); // branch, no push
                dest[numCmds].words.w1 = (uintptr_t)next->pcArray;
                fprintf(stderr, "[effect_gfx_swizzle] seg9 0x%X falls through -> branch to 0x%X\n",
                        sym->seg9Offset, next->seg9Offset);
            } else {
                dest[numCmds].words.w0 = (uintptr_t)(F3DEX2_G_ENDDL << 24);
                dest[numCmds].words.w1 = 0;
            }
        }
    }
}

/**
 * Look up the EffectGfxSegmentInfo for a given ROM start address.
 * Returns NULL if not found.
 */
const EffectGfxSegmentInfo* effect_gfx_find_segment(u32 romStart) {
    for (s32 i = 0; i < NUM_EFFECT_GFX_SEGMENTS; i++) {
        if (sEffectGfxSegments[i].romStart == romStart) {
            return &sEffectGfxSegments[i];
        }
    }
    return NULL;
}

/**
 * Load and swizzle effect graphics from ROM into the C arrays.
 *
 * Called from load_effect() when an effect is first loaded.
 * The ROM blob is temporary — all data is copied into permanent C arrays.
 *
 * @param romStart  ROM start address of the graphics segment
 * @param romEnd    ROM end address of the graphics segment
 * @return true if successful, false if segment info not found
 */
int effect_gfx_load_and_swizzle(u32 romStart, u32 romEnd) {
    const EffectGfxSegmentInfo* segInfo = effect_gfx_find_segment(romStart);
    if (segInfo == NULL) {
        fprintf(stderr, "[effect_gfx_swizzle] WARNING: no segment info for ROM 0x%X-0x%X\n",
                romStart, romEnd);
        return 0;
    }

    u32 blobSize = segInfo->romEnd - segInfo->romStart;
    if (blobSize == 0 || blobSize > 0x100000) {
        fprintf(stderr, "[effect_gfx_swizzle] ERROR: invalid blob size 0x%X for ROM 0x%X\n",
                blobSize, romStart);
        return 0;
    }

    // Allocate temporary buffer and load ROM data
    u8* blob = malloc(blobSize);
    if (blob == NULL) {
        fprintf(stderr, "[effect_gfx_swizzle] ERROR: failed to alloc %u bytes for blob\n", blobSize);
        return 0;
    }
    nuPiReadRom(segInfo->romStart, blob, blobSize);

    const EffectGfxSymbol* symbols = segInfo->symbols;
    s32 numSymbols = segInfo->numSymbols;

    // Pass 1: Copy texture/image data (no byte-swap — Fast3D handles N64 endian)
    for (s32 i = 0; i < numSymbols; i++) {
        if (symbols[i].type == EFX_SYM_IMG || symbols[i].type == EFX_SYM_PAL) {
            u32 offset = symbols[i].seg9Offset;
            u32 size = symbols[i].sizeBytes;
            if (offset + size <= blobSize && symbols[i].pcArray != NULL) {
                memcpy(symbols[i].pcArray, blob + offset, size);
            }
        }
    }

    // Pass 2: Convert vertex data (byte-swap from big-endian)
    for (s32 i = 0; i < numSymbols; i++) {
        if (symbols[i].type == EFX_SYM_VTX) {
            swizzle_vertices(blob, &symbols[i]);
        }
    }

    // Pass 3: Convert display lists (N64 8-byte → PC 16-byte, resolve pointers)
    for (s32 i = 0; i < numSymbols; i++) {
        if (symbols[i].type == EFX_SYM_GFX) {
            swizzle_display_list(blob, blobSize, &symbols[i], symbols, numSymbols);
        }
    }

    free(blob);

    // Log each segment once. A flat cap hid which effects ever load, which is the
    // first thing I need from a tester's log when an effect renders wrong.
    {
        static u8 sLogged[NUM_EFFECT_GFX_SEGMENTS];
        s32 segIndex = (s32)(segInfo - sEffectGfxSegments);
        if (segIndex >= 0 && segIndex < NUM_EFFECT_GFX_SEGMENTS && !sLogged[segIndex]) {
            sLogged[segIndex] = 1;
            fprintf(stderr, "[effect_gfx_swizzle] Loaded ROM 0x%X-0x%X (%u bytes, %d symbols)\n",
                    segInfo->romStart, segInfo->romEnd, blobSize, numSymbols);
        }
    }

    return 1;
}

#endif // PORT
