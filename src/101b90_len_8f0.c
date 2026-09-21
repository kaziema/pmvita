#include "common.h"
#include "sprite.h"
#include "nu/nusys.h"
#include "ld_addrs.h"
#include "sprite/player.h"
#ifdef PORT
#include <stdio.h>
#include <string.h>
#include "../port/endian.h"
#endif

#ifdef SHIFT
#define SPRITE_ROM_START (u32) sprites_ROM_START + 0x10
#elif VERSION_US || VERSION_IQUE
#define SPRITE_ROM_START 0x1943000 + 0x10
#elif VERSION_PAL
#define SPRITE_ROM_START 0x1DF0000 + 0x10
#elif VERSION_JP
#define SPRITE_ROM_START 0x1A40000 + 0x10
#endif

extern bool SpriteUseGeneralHeap;
extern HeapNode heap_generalHead;
extern HeapNode heap_spriteHead;

BSS s32 spr_asset_entry[2];
BSS s32 PlayerRasterLoadDescBuffer[101]; //NOTE: maximum rasters supported for a single player sprite is 101
BSS s32 PlayerRasterLoadDescNumLoaded;
BSS s32 PlayerRasterLoadDescBeginSpriteIndex[SPR_Peach3]; //TODO size linked to number of player sprites
BSS s32 D_802D0084[3]; // unused?
BSS s32 PlayerRasterLoadDesc[0x2E0]; // correct length?

BSS PlayerRastersHeader PlayerRasterHeader;
BSS s32 D_802E0C1C; // unused
BSS s32 PlayerSpriteRasterSets[SPR_Peach3 + 1]; //TODO size linked to number of player sprites
BSS s32 PlayerRasterCacheSize;
BSS s32 PlayerRasterMaxSize;
BSS s32 SpriteDataHeader[3];
BSS s32 D_802E0C6C; // unused?
BSS PlayerSpriteCacheEntry PlayerRasterCache[18];

#ifdef PORT
#define ALIGN4(v) (((uintptr_t)(v) >> 2) << 2)
#define SPR_SWIZZLE(base,offset) ((void*)((intptr_t)(offset) + (intptr_t)(base)))
#else
#define ALIGN4(v) (((u32)(v) >> 2) << 2)
#define SPR_SWIZZLE(base,offset) ((void*)((s32)(offset) + (s32)(base)))
#endif

void spr_swizzle_anim_offsets(s32 arg0, s32 base, void* spriteData) {
    u8* buffer;
    SpriteAnimComponent*** animList;
    SpriteAnimComponent** compList;
    SpriteAnimComponent* comp;
    s32 animOffset;
    s32 compOffset;
    s32 temp;

    // required to match, spriteData->animList would be nicer
    animList = (SpriteAnimComponent***) spriteData;
    animList += 4;

    while (true) {
        if (*animList == PTR_LIST_END) {
            break;
        }
        compList = (SpriteAnimComponent**) ((s32)*animList - ALIGN4(base));
        compList = SPR_SWIZZLE(ALIGN4(spriteData), compList);
        *animList = compList;
        while (true) {
            if (*compList == PTR_LIST_END) {
                break;
            }
            *compList = comp = SPR_SWIZZLE(ALIGN4(spriteData), (s32)*compList - ALIGN4(base));
            comp->cmdList = SPR_SWIZZLE(ALIGN4(spriteData), (s32)comp->cmdList - ALIGN4(base));
            compList++;
        }
        animList++;
    }
}

SpriteAnimData* spr_load_sprite(s32 idx, s32 isPlayerSprite, s32 useTailAlloc) {
    SpriteAnimData* animData;
    s32 base;
    s32 i;
    s32 compressedSize;
    s32* ptr1;
    IMG_PTR image;
    s32 count;
    s32** data;
    s32** palettes;

    if (isPlayerSprite) {
        base = SpriteDataHeader[1];
    } else {
        base = SpriteDataHeader[2];
    }

    // read current and next sprite offsets, so we can find the difference
    nuPiReadRom(base + idx * 4, &spr_asset_entry, sizeof(spr_asset_entry));
#ifdef PORT
    bswap32_array(&spr_asset_entry, sizeof(spr_asset_entry) / 4);
#endif

    compressedSize = ALIGN8(spr_asset_entry[1] - spr_asset_entry[0]);
#ifdef PORT
    if (compressedSize <= 0 || compressedSize > 0x100000) {
        fprintf(stderr, "[spr_load_sprite] ERROR: bad compressedSize=%d (entry[0]=0x%X entry[1]=0x%X)\n",
                compressedSize, spr_asset_entry[0], spr_asset_entry[1]);
        return NULL;
    }
#endif
    data = general_heap_malloc(compressedSize);
    nuPiReadRom(base + spr_asset_entry[0], data, compressedSize);

    ptr1 = (s32*)data;
#ifdef PORT
    // Verify YAY0 signature
    {
        u32 magic = read_be_u32(ptr1);
        if (magic != 0x59617930) {
            fprintf(stderr, "[spr_load_sprite] ERROR: bad YAY0 magic=0x%08X at ROM 0x%X\n",
                    magic, base + spr_asset_entry[0]);
            general_heap_free(data);
            return NULL;
        }
    }
#endif
    // skip 4 bytes: 'YAY0' signature
    ptr1++;

#ifdef PORT
    // YAY0 data is raw big-endian from ROM. Read decompressed size in big-endian.
    {
        s32 decompSize = read_be_s32(ptr1);
        if (decompSize <= 0 || decompSize > 0x100000) {
            fprintf(stderr, "[spr_load_sprite] ERROR: bad decompSize=%d\n", decompSize);
            general_heap_free(data);
            return NULL;
        }
        if (useTailAlloc) {
            animData = _heap_malloc_tail(&heap_spriteHead, decompSize);
        } else {
            animData = _heap_malloc(&heap_spriteHead, decompSize);
        }
        if (animData == NULL) {
            fprintf(stderr, "[spr_load_sprite] ERROR: sprite heap alloc failed (decompSize=%d)\n", decompSize);
            general_heap_free(data);
            return NULL;
        }
    }
#else
    if (useTailAlloc) {
        animData = _heap_malloc_tail(&heap_spriteHead, *ptr1);
    } else {
        animData = _heap_malloc(&heap_spriteHead, *ptr1);
    }
#endif
    decode_yay0(data, animData);
    general_heap_free(data);

#ifdef PORT
    // === PORT: Convert N64 sprite data (32-bit, big-endian) to 64-bit structs ===
    // The decompressed data has 4-byte pointer fields. On 64-bit, structs need 8-byte pointers.
    // We parse the raw N64 data and build proper 64-bit structures, keeping the raw buffer alive
    // for inline data (cmd lists, palette colors, image pixels).
    {
        u8* raw = (u8*)animData;

        // Read N64 header (4 big-endian u32 values at fixed byte offsets)
        u32 rastersOff  = read_be_u32(raw + 0);
        u32 palettesOff = read_be_u32(raw + 4);
        s32 maxComp     = read_be_s32(raw + 8);
        s32 colorVars   = read_be_s32(raw + 12);

        // Count animations (u32 offsets at byte offset 16, terminated by 0xFFFFFFFF)
        int numAnims = 0;
        while (read_be_u32(raw + 16 + numAnims * 4) != 0xFFFFFFFF)
            numAnims++;

        // Count rasters (u32 offsets at rastersOff, terminated by 0xFFFFFFFF)
        int numRasters = 0;
        while (read_be_u32(raw + rastersOff + numRasters * 4) != 0xFFFFFFFF)
            numRasters++;

        // Count palettes (u32 offsets at palettesOff, terminated by 0xFFFFFFFF)
        int numPalettes = 0;
        while (read_be_u32(raw + palettesOff + numPalettes * 4) != 0xFFFFFFFF)
            numPalettes++;

        // Count components per animation
        int totalComps = 0;
        s32 numCompsPerAnim[256]; // max 256 animations
        for (int a = 0; a < numAnims && a < 256; a++) {
            u32 animOff = read_be_u32(raw + 16 + a * 4);
            numCompsPerAnim[a] = 0;
            while (read_be_u32(raw + animOff + numCompsPerAnim[a] * 4) != 0xFFFFFFFF)
                numCompsPerAnim[a]++;
            totalComps += numCompsPerAnim[a];
        }

        // Calculate allocation size for 64-bit structures
        size_t headerSize = sizeof(SpriteRasterCacheEntry**)  // rastersOffset
                          + sizeof(PAL_PTR*)                   // palettesOffset
                          + sizeof(s32)                        // maxComponents
                          + sizeof(s32)                        // colorVariations
                          + (numAnims + 1) * sizeof(SpriteAnimComponent**); // animListStart + terminator
        size_t rasterListSize = (numRasters + 1) * sizeof(SpriteRasterCacheEntry*);
        size_t rasterEntriesSize = numRasters * sizeof(SpriteRasterCacheEntry);
        size_t paletteListSize = (numPalettes + 1) * sizeof(PAL_PTR);
        size_t compListsSize = 0;
        for (int a = 0; a < numAnims; a++)
            compListsSize += (numCompsPerAnim[a] + 1) * sizeof(SpriteAnimComponent*);
        size_t compEntriesSize = totalComps * sizeof(SpriteAnimComponent);

        size_t totalSize = headerSize + rasterListSize + rasterEntriesSize +
                          paletteListSize + compListsSize + compEntriesSize;

        // Allocate the 64-bit structure block
        SpriteAnimData* result;
        if (useTailAlloc) {
            result = _heap_malloc_tail(&heap_spriteHead, totalSize);
        } else {
            result = _heap_malloc(&heap_spriteHead, totalSize);
        }
        memset(result, 0, totalSize);

        u8* block = (u8*)result;
        size_t off = headerSize; // start placing sub-structures after the header

        // Set header fields
        result->maxComponents = maxComp;
        result->colorVariations = colorVars;

        // Build raster pointer array
        SpriteRasterCacheEntry** rasterList = (SpriteRasterCacheEntry**)(block + off);
        off += rasterListSize;
        SpriteRasterCacheEntry* rasterEntries = (SpriteRasterCacheEntry*)(block + off);
        off += rasterEntriesSize;
        result->rastersOffset = rasterList;

        for (int r = 0; r < numRasters; r++) {
            u32 rasterOff = read_be_u32(raw + rastersOff + r * 4);
            u8* rasterRaw = raw + rasterOff;
            // N64 raster entry: u32 image_offset, u8 w, u8 h, s8 pal, s8 quad = 8 bytes
            u32 imgOff = read_be_u32(rasterRaw);
            rasterEntries[r].width = rasterRaw[4];
            rasterEntries[r].height = rasterRaw[5];
            rasterEntries[r].palette = (s8)rasterRaw[6];
            rasterEntries[r].quadCacheIndex = (s8)rasterRaw[7];
            if (!isPlayerSprite) {
                rasterEntries[r].image = (IMG_PTR)(raw + imgOff);
            } else {
                // For player sprites, image field is used differently (raster cache system)
                rasterEntries[r].image = (IMG_PTR)(uintptr_t)imgOff;
            }
            rasterList[r] = &rasterEntries[r];
        }
        rasterList[numRasters] = PTR_LIST_END;

        // Build palette pointer array
        PAL_PTR* paletteList = (PAL_PTR*)(block + off);
        off += paletteListSize;
        result->palettesOffset = paletteList;

        for (int p = 0; p < numPalettes; p++) {
            u32 palOff = read_be_u32(raw + palettesOff + p * 4);
            paletteList[p] = (PAL_PTR)(raw + palOff);
            // Do NOT byte-swap palette data — Fast3D expects N64 big-endian format
            // and handles endianness internally: (palette[idx*2] << 8) | palette[idx*2+1]
        }
        paletteList[numPalettes] = PTR_LIST_END;

        // Build animation and component structures
        // Track which cmdList offsets have been byte-swapped to avoid double-swapping
        // when multiple components (across different animations) share the same cmdList data
        u32 swappedCmdListOffs[512];
        int numSwapped = 0;

        size_t compListOff = off;
        off += compListsSize;
        size_t compEntryOff = off;

        for (int a = 0; a < numAnims; a++) {
            u32 animOff = read_be_u32(raw + 16 + a * 4);
            SpriteAnimComponent** compPtrList = (SpriteAnimComponent**)(block + compListOff);
            result->animListStart[a] = compPtrList;
            compListOff += (numCompsPerAnim[a] + 1) * sizeof(SpriteAnimComponent*);

            for (int c = 0; c < numCompsPerAnim[a]; c++) {
                u32 compOff = read_be_u32(raw + animOff + c * 4);
                SpriteAnimComponent* comp = (SpriteAnimComponent*)(block + compEntryOff);
                compEntryOff += sizeof(SpriteAnimComponent);

                // N64 component layout: u32 cmdList_offset, s16 cmdListSize, s16 x, s16 y, s16 z = 12 bytes
                u8* compRaw = raw + compOff;
                u32 cmdListOff = read_be_u32(compRaw);
                comp->cmdList = (u16*)(raw + cmdListOff);
                comp->cmdListSize = (s16)read_be_u16(compRaw + 4);
                comp->compOffset.x = (s16)read_be_u16(compRaw + 6);
                comp->compOffset.y = (s16)read_be_u16(compRaw + 8);
                comp->compOffset.z = (s16)read_be_u16(compRaw + 10);

                // Byte-swap command list entries (u16 values, in the raw buffer)
                // Only swap once — multiple components may share the same cmdList data
                int alreadySwapped = 0;
                for (int s = 0; s < numSwapped; s++) {
                    if (swappedCmdListOffs[s] == cmdListOff) { alreadySwapped = 1; break; }
                }
                if (!alreadySwapped) {
                    int numCmds = comp->cmdListSize / 2;
                    u16* cmdData = comp->cmdList;
                    for (int j = 0; j < numCmds; j++) {
                        cmdData[j] = bswap16(cmdData[j]);
                    }
                    if (numSwapped < 512) {
                        swappedCmdListOffs[numSwapped++] = cmdListOff;
                    }
                }

                compPtrList[c] = comp;
            }
            compPtrList[numCompsPerAnim[a]] = PTR_LIST_END;
        }
        result->animListStart[numAnims] = PTR_LIST_END;

        // Handle player sprite raster load descriptors
        if (isPlayerSprite) {
            PlayerRasterLoadDescBeginSpriteIndex[idx] = PlayerRasterLoadDescNumLoaded;
            count = PlayerSpriteRasterSets[idx + 1] - PlayerSpriteRasterSets[idx];
            nuPiReadRom(SpriteDataHeader[0] + PlayerRasterHeader.loadDescriptors + sizeof(u32) * PlayerSpriteRasterSets[idx],
                PlayerRasterLoadDescBuffer, sizeof(PlayerRasterLoadDescBuffer));
            bswap32_array(PlayerRasterLoadDescBuffer, sizeof(PlayerRasterLoadDescBuffer) / 4);
            for (i = 0; i < count; i++) {
                PlayerRasterLoadDesc[PlayerRasterLoadDescNumLoaded++] = PlayerRasterLoadDescBuffer[i];
            }
        }

        // Don't free animData (raw buffer) — inline data (cmd lists, palettes, images) point into it
        return result;
    }
#else
    // swizzle raster array
    data = (s32**)animData->rastersOffset;
    data = SPR_SWIZZLE(ALIGN4(animData), data);
    animData->rastersOffset = (SpriteRasterCacheEntry**)data;

    while (true) {
        ptr1 = *data;
        if (ptr1 == PTR_LIST_END) {
            break;
        }
        // swizzle each raster cache entry
        ptr1 = SPR_SWIZZLE(ALIGN4(animData), ptr1);
        *data++ = ptr1;
        image = ((SpriteRasterCacheEntry*)ptr1)->image;

        if (!isPlayerSprite) {
            // swizzle image pointer in the cache entry
            image = SPR_SWIZZLE(ALIGN4(animData), image);
            *ptr1 = (s32)image;
        }
    }

    if (isPlayerSprite) {
        PlayerRasterLoadDescBeginSpriteIndex[idx] = PlayerRasterLoadDescNumLoaded;
        count = PlayerSpriteRasterSets[idx + 1] - PlayerSpriteRasterSets[idx];
        // load a range of raster loading desciptors to a buffer and copy contents into PlayerRasterLoadDesc
        nuPiReadRom(SpriteDataHeader[0] + PlayerRasterHeader.loadDescriptors + sizeof(u32) * PlayerSpriteRasterSets[idx],
            PlayerRasterLoadDescBuffer, sizeof(PlayerRasterLoadDescBuffer));
        for (i = 0; i < count; i++) {
            PlayerRasterLoadDesc[PlayerRasterLoadDescNumLoaded++] = PlayerRasterLoadDescBuffer[i];
        }
    }

    // swizzle palettes array
    palettes = SPR_SWIZZLE(ALIGN4(animData), animData->palettesOffset);
    animData->palettesOffset = (PAL_PTR*)palettes;
    while (true) {
        ptr1 = *palettes;
        if (ptr1 == PTR_LIST_END) {
            break;
        }
        // swizzle each palette pointer
        ptr1 = SPR_SWIZZLE(ALIGN4(animData), ptr1);
        *palettes++ = ptr1;
    }

    spr_swizzle_anim_offsets(0, 0, animData);
    return animData;
#endif
}

void spr_init_player_raster_cache(s32 cacheSize, s32 maxRasterSize) {
    void* raster;
    s32 i;

#ifdef PORT
    fprintf(stderr, "[spr_init_player_raster_cache] SPRITE_ROM_START=0x%X, reading SpriteDataHeader (%zu bytes)\n",
        (unsigned)SPRITE_ROM_START, sizeof(SpriteDataHeader));
#endif
    nuPiReadRom(SPRITE_ROM_START, &SpriteDataHeader, sizeof(SpriteDataHeader));
#ifdef PORT
    bswap32_array(&SpriteDataHeader, sizeof(SpriteDataHeader) / 4);
    fprintf(stderr, "[spr_init_player_raster_cache] SpriteDataHeader raw: [0]=0x%X [1]=0x%X [2]=0x%X\n",
        (unsigned)SpriteDataHeader[0], (unsigned)SpriteDataHeader[1], (unsigned)SpriteDataHeader[2]);
#endif
    PlayerRasterCacheSize = cacheSize;
    PlayerRasterMaxSize = maxRasterSize;
    SpriteDataHeader[0] += SPRITE_ROM_START;
    SpriteDataHeader[1] += SPRITE_ROM_START;
    SpriteDataHeader[2] += SPRITE_ROM_START;
#ifdef PORT
    fprintf(stderr, "[spr_init_player_raster_cache] SpriteDataHeader adjusted: [0]=0x%X [1]=0x%X [2]=0x%X\n",
        (unsigned)SpriteDataHeader[0], (unsigned)SpriteDataHeader[1], (unsigned)SpriteDataHeader[2]);
#endif
    raster = _heap_malloc(&heap_spriteHead, maxRasterSize * cacheSize);
#ifdef PORT
    fprintf(stderr, "[spr_init_player_raster_cache] _heap_malloc(%u * %u = %u) -> %p\n",
        (unsigned)maxRasterSize, (unsigned)cacheSize, (unsigned)(maxRasterSize * cacheSize), raster);
    if (!raster) {
        fprintf(stderr, "[spr_init_player_raster_cache] WARNING: allocation failed, heap_spriteHead exhausted\n");
    }
#endif

    for (i = 0; i < ARRAY_COUNT(PlayerRasterCache); i++) {
        PlayerRasterCache[i].raster = raster;
        raster += PlayerRasterMaxSize;
        PlayerRasterCache[i].lazyDeleteTime = 0;
        PlayerRasterCache[i].rasterIndex = 0;
        PlayerRasterCache[i].spriteIndex = 0xFF;
    }

    for (i = 0; i < ARRAY_COUNT(PlayerRasterLoadDescBeginSpriteIndex); i++)    {
        PlayerRasterLoadDescBeginSpriteIndex[i] = 0;
    }
    PlayerRasterLoadDescNumLoaded = 0;
    nuPiReadRom(SpriteDataHeader[0], &PlayerRasterHeader, sizeof(PlayerRasterHeader));
#ifdef PORT
    bswap32_array(&PlayerRasterHeader, sizeof(PlayerRasterHeader) / 4);
#endif
    nuPiReadRom(SpriteDataHeader[0] + PlayerRasterHeader.indexRanges, PlayerSpriteRasterSets, sizeof(PlayerSpriteRasterSets));
#ifdef PORT
    bswap32_array(PlayerSpriteRasterSets, sizeof(PlayerSpriteRasterSets) / 4);
#endif
}

IMG_PTR spr_get_player_raster(s32 rasterIndex, s32 playerSpriteID) {
    PlayerSpriteCacheEntry* cacheEntry;
    u32 playerRasterInfo;
    s32 idx = -1;
    s32 i;

    for (i = 0; i < PlayerRasterCacheSize; i++) {
        if (PlayerRasterCache[i].lazyDeleteTime != 0) {
            if (PlayerRasterCache[i].rasterIndex == rasterIndex && PlayerRasterCache[i].spriteIndex == playerSpriteID) {
                PlayerRasterCache[i].lazyDeleteTime = 2;
                return PlayerRasterCache[i].raster;
            }
        } else if (idx == -1) {
            idx = i;
        }
    }

    if (idx == -1) {
        return nullptr;
    }

    cacheEntry = &PlayerRasterCache[idx];
    cacheEntry->rasterIndex = rasterIndex;
    cacheEntry->spriteIndex = playerSpriteID;
    cacheEntry->lazyDeleteTime = 2;

    // each player raster load descriptor has image size (in bytes) and relative offset packed into one word
    // upper three nibbles give size / 16, lower 5 give offset
    playerRasterInfo = PlayerRasterLoadDesc[PlayerRasterLoadDescBeginSpriteIndex[playerSpriteID] + rasterIndex];
    nuPiReadRom(SpriteDataHeader[0] + (playerRasterInfo & 0xFFFFF), cacheEntry->raster, (playerRasterInfo >> 0x10) & 0xFFF0);
    return cacheEntry->raster;
}

void spr_update_player_raster_cache(void) {
    s32 i;

    func_8013A4D0();

    for (i = 0; i < PlayerRasterCacheSize; i++) {
        if (PlayerRasterCache[i].lazyDeleteTime != 0) {
            PlayerRasterCache[i].lazyDeleteTime--;
        }
    }
}

void spr_load_npc_extra_anims(SpriteAnimData* header, u32* extraAnimList) {
#ifdef PORT
    // PORT: On 64-bit, the sprite data structures (raster list, palette list, anim components)
    // are allocated separately from the raw N64 data buffer. The N64 version of this function
    // does in-place compaction assuming everything is inline in one contiguous buffer.
    // This corrupts the 64-bit structures. On PC we have plenty of RAM so skip compaction.
    // The raw raster/palette data in the buffer remains valid and accessible.
    (void)header;
    (void)extraAnimList;
    return;
#else
    u8* src;
    s32 imgID;
    s32 rasterSize;
    s32 remaining;
    u16 animCmd;
    u16* cmdList;
    u32 extraAnimID;
    u8* dataPos;
    void* copyEnd;
    SpriteAnimComponent* comp;
    SpriteAnimComponent** compList;
    void* writePos;
    SpriteRasterCacheEntry** rasterList;
    SpriteRasterCacheEntry* raster;
    u16** oldPalList;
    u16* palette;
    // one extra required for 'done' sentinel
    s32 sawRaster[100 + 1];
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sawRaster) - 1; i++) {
        sawRaster[i] = false;
    }

    while ((extraAnimID = *extraAnimList++) != -1) {
        compList = header->animListStart[extraAnimID & 0xFF];
        while ((comp = *compList++) != PTR_LIST_END) {
            cmdList = comp->cmdList;
            remaining = (s16) comp->cmdListSize / 2;
            while (remaining > 0) {
                animCmd = *cmdList++;
                remaining--;
                switch (animCmd & 0xF000) {
                    case 0x1000:
                        i = animCmd; // required to match
                        imgID = i & 0xFF;
                        if (imgID < ARRAY_COUNT(sawRaster) - 1) {
                            sawRaster[imgID] = true;
                        }
                        break;
                    case 0x3000:
                        cmdList++;
                        remaining--;
                        // fallthrough
                    case 0x4000:
                        cmdList++;
                        remaining--;
                        // fallthrough
                    case 0x7000:
                    case 0x5000:
                        cmdList++;
                        remaining--;
                        // fallthrough
                    case 0x0000:
                    case 0x2000:
                    case 0x6000:
                    case 0x8000:
                        break;
                }
            }
        }
    }

    rasterList = header->rastersOffset;
    raster = *rasterList;
    dataPos = raster->image;

    for (i = 0; i < ARRAY_COUNT(sawRaster) - 1; i++) {
        if ((raster = *rasterList) == PTR_LIST_END) {
            break;
        }
        if (sawRaster[i]) {
            src = raster->image;
            rasterSize = (raster->width * raster->height) / 2;
            copyEnd = &dataPos[rasterSize];
            rasterSize += 8;
            if (dataPos != src) {
                raster->image = dataPos;
                bcopy(src, dataPos, rasterSize);
            }
            *rasterList = copyEnd;
            dataPos += rasterSize;
        }
        rasterList++;
    }
    // sentinel value to mark end of valid data
    sawRaster[i] = true;

    writePos = dataPos;

    // copy raster list
    rasterList = header->rastersOffset;
    header->rastersOffset = writePos;

    for (i = 0; i < ARRAY_COUNT(sawRaster) - 1; i++) {
        raster = *rasterList++;
        if (sawRaster[i]) {
            *(SpriteRasterCacheEntry**) writePos = raster;
        } else {
            *(SpriteRasterCacheEntry**) writePos = (SpriteRasterCacheEntry*) copyEnd;
        }
#ifdef PORT
        writePos += sizeof(SpriteRasterCacheEntry*);
#else
        writePos += 4;
#endif
        if (raster == PTR_LIST_END) {
            break;
        }
    }

    // copy palette list
    oldPalList = header->palettesOffset;
    header->palettesOffset = writePos;

    for (i = 0; i < ARRAY_COUNT(sawRaster) - 1; i++) {
        raster = (SpriteRasterCacheEntry*)*oldPalList++; // required to match
        *(u16**)writePos = (u16*)raster;
#ifdef PORT
        writePos += sizeof(u16*);
#else
        writePos += 4;
#endif
        if (raster == PTR_LIST_END) {
            break;
        }
    }

    _heap_realloc(&heap_spriteHead, header, (s32)writePos - (s32)header);
#endif /* !PORT */
}

SpriteComponent** spr_allocate_components(s32 count) {
    s32 listSize;
    SpriteComponent** listStart;
    SpriteComponent* component;
    SpriteComponent** listPos;
    u32 totalSize;
    s32 i;

    // data will contain a -1 terminated list, followed by the SpriteComponents
    // corresponding to that list
    listSize = (count + 1) * sizeof(SpriteComponent*);
    totalSize = (count * sizeof(SpriteComponent)) + listSize;

    if (SpriteUseGeneralHeap) {
        listStart = _heap_malloc(&heap_generalHead, totalSize);
        listPos = listStart;
        component = (SpriteComponent*) listPos;
    } else {
        listStart = _heap_malloc(&heap_spriteHead, totalSize);
        listPos = listStart;
        component = (SpriteComponent*) listPos;
    }

#ifdef PORT
    component = (SpriteComponent*)((char*)(component) + listSize);
#else
    component = (SpriteComponent*)((s32)(component) + (listSize / 4) * 4);
#endif

    // fill list values
    for (i = 0; i < count; i++) {
        *listPos++ = component;
        component++;
    }
    *listPos = PTR_LIST_END;

    return listStart;
}
