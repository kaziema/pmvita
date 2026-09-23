#include "common.h"
#include "effects.h"
#include "ld_addrs.h"
#ifdef PORT
#include <stdio.h>
#include "port/rom_offsets.h"
// Forward declaration from port/effect_gfx_swizzle.c
int effect_gfx_load_and_swizzle(u32 romStart, u32 romEnd);
#endif

typedef s8 TlbEntry[0x1000];
typedef TlbEntry TlbMappablePage[15];

#define EFFECT_GLOBALS_TLB_IDX 0x10

BSS EffectSharedData gEffectSharedData[15];
EffectInstance* gEffectInstances[96];

extern TlbMappablePage gEffectDataBuffer;
extern Addr gEffectGlobals;

#define FX_ENTRY(name, gfx_name) { \
    .entryPoint = name##_main, \
    .dmaStart = effect_##name##_ROM_START, \
    .dmaEnd = effect_##name##_ROM_END, \
    .dmaDest = effect_##name##_VRAM, \
    .graphicsDmaStart = gfx_name##_ROM_START, \
    .graphicsDmaEnd = gfx_name##_ROM_END, \
}

#include "effects/effect_table.c"

s32 D_8007FEB8[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 5, 3, 4, 13, 60, 0, 512, 0, 0, 3, 0 };

/// Used for unbound function points in effect structs.
void stub_effect_delegate(EffectInstance* effect) {
}

void set_effect_pos_offset(EffectInstance* effect, f32 x, f32 y, f32 z) {
    s32* data = effect->data.any;

    ((f32*)data)[1] = x;
    ((f32*)data)[2] = y;
    ((f32*)data)[3] = z;
}

void clear_effect_data(void) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gEffectSharedData); i++) {
        gEffectSharedData[i].flags = 0;
    }

    for (i = 0; i < ARRAY_COUNT(gEffectInstances); i++) {
        gEffectInstances[i] = nullptr;
    }

    osUnmapTLBAll();
    osMapTLB(EFFECT_GLOBALS_TLB_IDX, OS_PM_4K, effect_globals_VRAM, (s32)&gEffectGlobals & 0xFFFFFF, -1, -1);
    DMA_COPY_SEGMENT(effect_globals);
}

void func_80059D48(void) {
}

void update_effects(void) {
    if (!(gOverrideFlags & (GLOBAL_OVERRIDES_800 | GLOBAL_OVERRIDES_400))) {
        EffectSharedData* sharedData;
        s32 i;

        // reset free delay for each EffectSharedData touched in previous update
        for (i = 0, sharedData = gEffectSharedData; i < ARRAY_COUNT(gEffectSharedData); i++, sharedData++) {
            if (sharedData->flags & FX_SHARED_DATA_LOADED) {
                if (!(sharedData->flags & FX_SHARED_DATA_CAN_FREE)) {
                    sharedData->flags |= FX_SHARED_DATA_CAN_FREE;
                    sharedData->freeDelay = 3;
                }
            }
        }

        // update each EffectInstances
        for (i = 0; i < ARRAY_COUNT(gEffectInstances); i++) {
            EffectInstance* effectInstance = gEffectInstances[i];

            if (effectInstance != nullptr && (effectInstance->flags & FX_INSTANCE_FLAG_ENABLED)) {
                effectInstance->shared->flags &= ~FX_SHARED_DATA_CAN_FREE;

                if (gGameStatusPtr->context != CONTEXT_WORLD) {
                    if (effectInstance->flags & FX_INSTANCE_FLAG_BATTLE) {
                        effectInstance->shared->update(effectInstance);
                        effectInstance->flags |= FX_INSTANCE_FLAG_HAS_UPDATED;
                    }
                } else {
                    if (!(effectInstance->flags & FX_INSTANCE_FLAG_BATTLE)) {
                        effectInstance->shared->update(effectInstance);
                        effectInstance->flags |= FX_INSTANCE_FLAG_HAS_UPDATED;
                    }
                }
            }
        }

        // free any EffectSharedData which haven't been used recently
        for (i = 0, sharedData = gEffectSharedData; i < ARRAY_COUNT(gEffectSharedData); i++, sharedData++) {
            if (sharedData->flags & FX_SHARED_DATA_LOADED) {
                if (sharedData->flags & FX_SHARED_DATA_CAN_FREE) {
                    if (sharedData->freeDelay != 0) {
                        sharedData->freeDelay--;
                    } else {
#ifdef PORT
                        // On PC, graphics data lives in static C arrays (D_09xxxxxx symbols),
                        // not in a heap allocation. Don't free the sentinel value.
                        sharedData->graphics = nullptr;
#else
                        if (sharedData->graphics != nullptr) {
                            general_heap_free(sharedData->graphics);
                            sharedData->graphics = nullptr;
                        }
#endif
                        sharedData->flags = 0;
                        osUnmapTLB(i);
                    }
                }
            }
        }
    }
}

void render_effects_scene(void) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gEffectInstances); i++) {
        EffectInstance* effectInstance = gEffectInstances[i];

        if (effectInstance != nullptr) {
            if (effectInstance->flags & FX_INSTANCE_FLAG_ENABLED) {
                if (effectInstance->flags & FX_INSTANCE_FLAG_HAS_UPDATED) {
                    if (gGameStatusPtr->context != CONTEXT_WORLD) {
                        if (effectInstance->flags & FX_INSTANCE_FLAG_BATTLE) {
                            effectInstance->shared->renderScene(effectInstance);
                        }
                    } else {
                        if (!(effectInstance->flags & FX_INSTANCE_FLAG_BATTLE)) {
                            effectInstance->shared->renderScene(effectInstance);
                        }
                    }
                }
            }
        }
    }
}

void render_effects_UI(void) {
    s32 cond = true;
    s32 i;

#ifdef PORT
    // Diagnostic: detect corrupted effect instance pointers
    for (int j = 0; j < ARRAY_COUNT(gEffectInstances); j++) {
        if (gEffectInstances[j] != nullptr) {
            uintptr_t addr = (uintptr_t)gEffectInstances[j];
            // Valid heap pointers on macOS are typically in 0x100000000-0x200000000 range
            if (addr < 0x1000 || addr > 0x800000000ULL) {
                fprintf(stderr, "[render_effects_UI] CORRUPTION: gEffectInstances[%d] = %p (invalid pointer!)\n",
                        j, (void*)gEffectInstances[j]);
                fprintf(stderr, "  gEffectInstances array at %p, gEffectSharedData at %p\n",
                        (void*)gEffectInstances, (void*)gEffectSharedData);
                fflush(stderr);
                gEffectInstances[j] = nullptr; // Clear corrupted entry to avoid crash
            }
        }
    }
#endif

    for (i = 0; i < ARRAY_COUNT(gEffectInstances); i++) {
        EffectInstance* effectInstance = gEffectInstances[i];

        if (effectInstance != nullptr) {
            if (effectInstance->flags & FX_INSTANCE_FLAG_ENABLED) {
                if (effectInstance->flags & FX_INSTANCE_FLAG_HAS_UPDATED) {
                    void (*renderUI)(EffectInstance* effect);

                    if (gGameStatusPtr->context != CONTEXT_WORLD && !(effectInstance->flags & FX_INSTANCE_FLAG_BATTLE)) {
                        continue;
                    }

                    if (gGameStatusPtr->context == CONTEXT_WORLD && effectInstance->flags & FX_INSTANCE_FLAG_BATTLE) {
                        continue;
                    }

                    renderUI = effectInstance->shared->renderUI;
                    if (renderUI != stub_effect_delegate) {
#ifdef PORT
                        if (renderUI == nullptr) {
                            fprintf(stderr, "[render_effects_UI] WARN: effect %d instance %d has NULL renderUI, skipping\n",
                                    effectInstance->effectID, i);
                            continue;
                        }
#endif
                        if (cond) {
                            Camera* camera = &gCameras[gCurrentCameraID];

                            gDPPipeSync(gMainGfxPos++);
                            gSPViewport(gMainGfxPos++, &camera->vp);
                            gSPClearGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG |
                                                G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD |
                                                G_SHADING_SMOOTH | G_CLIPPING | 0x40F9FA);
                            gSPSetGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_SHADING_SMOOTH);
                            gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE,
                                              camera->viewportStartX,
                                              camera->viewportStartY,
                                              camera->viewportStartX + camera->viewportW,
                                              camera->viewportStartY + camera->viewportH);
                            gSPClipRatio(gMainGfxPos++, FRUSTRATIO_2);

                            cond = false;
                            if (!(camera->flags & CAMERA_FLAG_ORTHO)) {
                                gSPPerspNormalize(gMainGfxPos++, camera->perspNorm);
                                gSPMatrix(gMainGfxPos++, &gDisplayContext->camPerspMatrix[gCurrentCameraID],
                                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
                            }
                        }

                        renderUI(effectInstance);
                    }
                }
            }
        }
    }
}

#ifdef PORT
// Cleared on every map load so each map reports the effects it actually runs.
u8 gPortEffectSeen[256];

// Bumped on every map load. Diagnostics elsewhere compare against it to log once per map.
s32 gPortMapLogGen = 0;

void port_reset_effect_log(void) {
    s32 i;

    for (i = 0; i < (s32)sizeof(gPortEffectSeen); i++) {
        gPortEffectSeen[i] = 0;
    }
    gPortMapLogGen++;
}
#endif

EffectInstance* create_effect_instance(EffectBlueprint* effectBp) {
    EffectInstance* newEffectInst;
    EffectSharedData* sharedData;
    s32 i;

    // Search for an unused instance
    for (i = 0; i < ARRAY_COUNT(gEffectInstances); i++) {
        if (gEffectInstances[i] == nullptr) {
            break;
        }
    }

    ASSERT(i < ARRAY_COUNT(gEffectInstances));

    // Allocate space for the new instance
    gEffectInstances[i] = newEffectInst = general_heap_malloc(sizeof(*newEffectInst));
    ASSERT(newEffectInst != nullptr);

    sharedData = &gEffectSharedData[0];
    newEffectInst->effectID = effectBp->effectID;
    newEffectInst->flags = FX_INSTANCE_FLAG_ENABLED;

    // Look for a loaded effect of the proper index
    for (i = 0; i < ARRAY_COUNT(gEffectSharedData); i++) {
        if ((sharedData->flags & FX_SHARED_DATA_LOADED) && (sharedData->effectIndex == effectBp->effectID)) {
            break;
        }
        sharedData++;
    }

    ASSERT(i < ARRAY_COUNT(gEffectSharedData));

    // If this is the first new instance of the effect, initialize the function pointers
    if (sharedData->instanceCounter == 0) {
        sharedData->update = effectBp->update;
        if (sharedData->update == nullptr) {
            sharedData->renderScene = stub_effect_delegate;
        }

        sharedData->renderScene = effectBp->renderScene;
        /// @bug? null check for renderUI instead of renderWorld
        if (sharedData->renderUI == nullptr) {
            sharedData->renderUI = stub_effect_delegate;
        }

        sharedData->renderUI = effectBp->renderUI;
        if (sharedData->renderUI == nullptr) {
            sharedData->renderUI = stub_effect_delegate;
        }
    }

    sharedData->instanceCounter++;
    newEffectInst->shared = sharedData;

    if (effectBp->init != nullptr) {
        effectBp->init(newEffectInst);
    }

    if (gGameStatusPtr->context != CONTEXT_WORLD) {
        newEffectInst->flags |= FX_INSTANCE_FLAG_BATTLE;
    }

#ifdef PORT
    // Report each effect id the first time it is created, with its slot and instance pointer,
    // so a crashing data pointer can be traced back to the effect that owns it.
    {
        s32 id = newEffectInst->effectID;

        if (id >= 0 && id < (s32)sizeof(gPortEffectSeen) && !gPortEffectSeen[id]) {
            gPortEffectSeen[id] = 1;
            fprintf(stderr, "[fx+] id=%d slot=%d inst=%p parts=%d ctx=%d\n",
                    id, i, (void*)newEffectInst, newEffectInst->numParts, gGameStatusPtr->context);
        }
    }
#endif

    return newEffectInst;
}

void remove_effect(EffectInstance* effectInstance) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gEffectInstances); i++) {
        if (gEffectInstances[i] == effectInstance) {
            break;
        }
    }

    ASSERT(i < ARRAY_COUNT(gEffectInstances));

#ifdef PORT
    // Pairs with [fx+]. If a pointer shows up here twice, or a freed one is used later, this is
    // where that shows.
    {
        static s32 sReported = 0;

        if (sReported < 40) {
            sReported++;
            fprintf(stderr, "[fx-] id=%d slot=%d inst=%p data=%p\n", effectInstance->effectID, i,
                    (void*)effectInstance, (void*)effectInstance->data.any);
        }
    }
#endif

    if (effectInstance->data.any == nullptr) {
        general_heap_free(effectInstance);
        gEffectInstances[i] = nullptr;
    } else {
        general_heap_free(effectInstance->data.any);
        general_heap_free(effectInstance);
        gEffectInstances[i] = nullptr;
    }
}

void remove_all_effects(void) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gEffectInstances); i++) {
        EffectInstance* effect = gEffectInstances[i];

        if (effect != nullptr && effect->flags & FX_INSTANCE_FLAG_BATTLE) {
            if (effect->data.any != nullptr) {
                general_heap_free(effect->data.any);
            }
            general_heap_free(effect);
            gEffectInstances[i] = nullptr;
        }
    }
}

s32 load_effect(s32 effectIndex) {
    EffectTableEntry* effectEntry = &gEffectTable[effectIndex];
    EffectSharedData* sharedData;
    s32 i;

    // Look for a loaded effect matching the desired index
    for (i = 0, sharedData = &gEffectSharedData[0]; i < ARRAY_COUNT(gEffectSharedData); i++) {
        if (sharedData->flags & FX_SHARED_DATA_LOADED && sharedData->effectIndex == effectIndex) {
            break;
        }
        sharedData++;
    }

    // If an effect was found within the table, initialize it and return
    if (i < ARRAY_COUNT(gEffectSharedData)) {
        sharedData->effectIndex = effectIndex;
        sharedData->instanceCounter = 0;
        sharedData->flags = FX_SHARED_DATA_LOADED;
        return 1;
    }

    // If a loaded effect wasn't found, look for the first empty space
    for (i = 0, sharedData = &gEffectSharedData[0]; i < ARRAY_COUNT(gEffectSharedData); i++) {
        if (!(sharedData->flags & FX_SHARED_DATA_LOADED)) {
            break;
        }
        sharedData++;
    }

    // If no empty space was found, panic
    ASSERT(i < ARRAY_COUNT(gEffectSharedData));

#ifdef PORT
    // On PC, effect code is statically linked. D_09xxxxxx symbols are C arrays
    // that we fill with swizzled data from ROM at load time.
    // The swizzle function loads the ROM blob, converts N64 big-endian data to PC format,
    // and copies into the correctly-sized C arrays.
    if (effectEntry->graphicsDmaStart != nullptr) {
        u32 romStart = resolve_rom_offset(effectEntry->graphicsDmaStart);
        u32 romEnd = resolve_rom_offset(effectEntry->graphicsDmaEnd);
        if (romStart != 0 && romEnd != 0 && romEnd > romStart) {
            int ok = effect_gfx_load_and_swizzle(romStart, romEnd);
            if (!ok) {
                fprintf(stderr, "[load_effect] swizzle FAILED for effect %d (ROM 0x%X-0x%X)\n",
                        effectIndex, romStart, romEnd);
            }
        } else {
            fprintf(stderr, "[load_effect] bad ROM addr for effect %d: start=0x%X end=0x%X\n",
                    effectIndex, romStart, romEnd);
        }
    }
    // Set graphics to a non-null sentinel so the game knows graphics are loaded.
    // The actual data lives in the D_09xxxxxx C arrays, not in this pointer.
    // Using (void*)1 as sentinel - the gSPSegment(0x09, ...) call in effect render
    // functions will set segment 9 to this value, but since all DL commands use
    // direct C pointers (not segment 9 offsets), the segment base is irrelevant.
    sharedData->graphics = (void*)1;
#else
    // Map space for the effect
    osMapTLB(i, OS_PM_4K, effectEntry->dmaDest, (s32)(gEffectDataBuffer[i]) & 0xFFFFFF, -1, -1);

    // Copy the effect into the newly mapped space
    dma_copy(effectEntry->dmaStart, effectEntry->dmaEnd, effectEntry->dmaDest);

    // If there's graphics data for the effect, allocate space and copy into the new space
    if (effectEntry->graphicsDmaStart != nullptr) {
        void* graphics = general_heap_malloc(effectEntry->graphicsDmaEnd - effectEntry->graphicsDmaStart);
        sharedData->graphics = graphics;
        ASSERT(graphics != nullptr);

#if VERSION_PAL
        if (effectEntry->graphicsDmaStart == effect_gfx_attack_result_text_ROM_START) {
            switch (gCurrentLanguage) {
                case LANGUAGE_EN:
                    dma_copy(effectEntry->graphicsDmaStart, effectEntry->graphicsDmaEnd, sharedData->graphics);
                    break;
                case LANGUAGE_DE:
                    dma_copy(effect_gfx_attack_result_text_de_ROM_START, effect_gfx_attack_result_text_de_ROM_END, sharedData->graphics);
                    break;
                case LANGUAGE_FR:
                    dma_copy(effect_gfx_attack_result_text_fr_ROM_START, effect_gfx_attack_result_text_fr_ROM_END, sharedData->graphics);
                    break;
                default:
                    dma_copy(effect_gfx_attack_result_text_es_ROM_START, effect_gfx_attack_result_text_es_ROM_END, sharedData->graphics);
                    break;
            }
        } else if (effectEntry->graphicsDmaStart == effect_gfx_chapter_change_ROM_START) {
            switch (gCurrentLanguage) {
                case LANGUAGE_EN:
                    dma_copy(effectEntry->graphicsDmaStart, effectEntry->graphicsDmaEnd, sharedData->graphics);
                    break;
                case LANGUAGE_DE:
                    dma_copy(effect_chapter_change_gfx_de_ROM_START, effect_chapter_change_gfx_de_ROM_END, sharedData->graphics);
                    break;
                case LANGUAGE_FR:
                    dma_copy(effect_chapter_change_gfx_fr_ROM_START, effect_chapter_change_gfx_fr_ROM_END, sharedData->graphics);
                    break;
                default:
                    dma_copy(effect_chapter_change_gfx_es_ROM_START, effect_chapter_change_gfx_es_ROM_END, sharedData->graphics);
                    break;
            }
        } else
#endif
            dma_copy(effectEntry->graphicsDmaStart, effectEntry->graphicsDmaEnd, sharedData->graphics);
    }
#endif

    // Initialize the newly loaded effect data
    sharedData->effectIndex = effectIndex;
    sharedData->instanceCounter = 0;
    sharedData->flags = FX_SHARED_DATA_LOADED;
    return 1;
}
