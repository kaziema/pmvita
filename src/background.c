#include "common.h"
#include "model.h"
#include "gcc/string.h"
#ifdef PORT
#include "../port/endian.h"
#include <stdio.h>
extern void gfx_texture_cache_clear(void);
extern float GameEngine_GetAspectRatio(void);

static void port_draw_bg_ext_rect(s32 x0, s32 x1, s32 y0, s32 y1, s32 texel) {
    if (x1 < x0) {
        return;
    }
    gSPWideTextureRectangle(gMainGfxPos++, x0 * 4, y0 * 4, x1 * 4, y1 * 4, G_TX_RENDERTILE, texel * 32, 0, 4096, 1024);
}

// Continues the wrapping panorama past both edges so it reaches a wider window.
static void port_draw_bg_extension(s32 bgMinX, s32 bgMaxX, s32 bgXOffset, s32 extra, s32 y0, s32 y1) {
    s32 t0 = (bgMaxX - bgXOffset) % bgMaxX; // texel shown at x = bgMinX
    s32 extL = bgMinX + extra;
    s32 extR = SCREEN_WIDTH + extra - (bgMinX + bgMaxX);
    s32 d1;
    s32 rem;
    s32 la;

    if (extL > 0) {
        d1 = extL < t0 ? extL : t0;
        port_draw_bg_ext_rect(bgMinX - d1, bgMinX - 1, y0, y1, t0 - d1);
        rem = extL - d1;
        if (rem > 0) {
            port_draw_bg_ext_rect(bgMinX - extL, bgMinX - d1 - 1, y0, y1, bgMaxX - rem);
        }
    }
    if (extR > 0) {
        la = extR < bgMaxX - t0 ? extR : bgMaxX - t0;
        port_draw_bg_ext_rect(bgMinX + bgMaxX, bgMinX + bgMaxX + la - 1, y0, y1, t0);
        if (extR - la > 0) {
            port_draw_bg_ext_rect(bgMinX + bgMaxX + la, bgMinX + bgMaxX + extR - 1, y0, y1, 0);
        }
    }
}
#endif

char gCloudyFlowerFieldsBg[] = "fla_bg";
char gSunnyFlowerFieldsBg[] = "flb_bg";
s8 gBackroundWaveEnabled = false;
s16 gBackroundTextureYOffset = 0;
f32 gBackroundWavePhase = 0.0f;

BSS PAL_BIN gBackgroundPalette[256];
BSS f32 gBackroundLastScrollValue;
BSS s32 D_801595A4[3];
#if !VERSION_PAL
BSS s32 D_801595AC;
#endif

void load_map_bg(char* optAssetName) {
    if (optAssetName != nullptr) {
        UNK_PTR compressedData;
        u32 assetSize;
        char* assetName = optAssetName;

        if (evt_get_variable(nullptr, GB_StoryProgress) >= STORY_CH6_DESTROYED_PUFF_PUFF_MACHINE) {
            // Use sunny Flower Fields bg rather than cloudy
            if (strcmp(assetName, gCloudyFlowerFieldsBg) == 0) {
                assetName = gSunnyFlowerFieldsBg;
            }
        }

        compressedData = load_asset_by_name(assetName, &assetSize);
#ifdef PORT
        /*
         * On N64, gBackgroundImage is a 64KB buffer at 0x80200000 where the
         * decoded Yay0 data contains an N64 BackgroundHeader (16 bytes) followed
         * by raster and palette data.
         *
         * On 64-bit PC, BackgroundHeader is 24 bytes (8-byte pointers), so we
         * decode into a SEPARATE buffer and populate the struct from parsed fields.
         * This avoids the struct fields overwriting the start of the raster data.
         */
        if (compressedData == NULL) {
            fprintf(stderr, "[bg] ERROR: load_asset_by_name returned NULL for '%s'\n", assetName);
        } else {
            extern u8 gBackgroundImageBuffer[];
            u8* buf = gBackgroundImageBuffer;
            u32 rasterN64, paletteN64;

            if (assetSize > 0x10000) {
                fprintf(stderr, "[bg] WARNING: decompressed bg size 0x%X exceeds buffer 0x10000!\n", assetSize);
            }

            decode_yay0(compressedData, buf);
            general_heap_free(compressedData);

            /* Parse N64-layout header (16 bytes big-endian) */
            rasterN64  = read_be_u32(buf + 0);
            paletteN64 = read_be_u32(buf + 4);

            u32 rasterOff  = rasterN64  - 0x80200000;
            u32 paletteOff = paletteN64 - 0x80200000;

            /* Populate the separate PC struct — pointers into the buffer */
            gBackgroundImage.raster  = buf + rasterOff;
            gBackgroundImage.palette = (PAL_PTR)(buf + paletteOff);
            gBackgroundImage.startX  = read_be_u16(buf + 8);
            gBackgroundImage.startY  = read_be_u16(buf + 10);
            gBackgroundImage.width   = read_be_u16(buf + 12);
            gBackgroundImage.height  = read_be_u16(buf + 14);

            /* Invalidate Fast3D texture cache — the background buffer is reused
             * at the same addresses, so stale cache entries from the previous
             * background would match the new data's cache key */
            gfx_texture_cache_clear();
        }
#else
        decode_yay0(compressedData, &gBackgroundImage);
        general_heap_free(compressedData);
#endif
    }
}

void reset_background_settings(void) {
    gBackroundLastScrollValue = 0;
    gBackroundWaveEnabled = false;
    gGameStatusPtr->backgroundDarkness = 180;
    gGameStatusPtr->backgroundFlags &= BACKGROUND_RENDER_STATE_MASK;
}

void set_background(BackgroundHeader* bg) {
    gGameStatusPtr->backgroundMaxX = bg->width;
    gGameStatusPtr->backgroundMaxY = bg->height;
    gGameStatusPtr->backgroundMinX = bg->startX;
    gGameStatusPtr->backgroundMinY = bg->startY;
    gGameStatusPtr->backgroundRaster = bg->raster;
    gGameStatusPtr->backgroundPalette = bg->palette;
    gGameStatusPtr->backgroundFlags |= BACKGROUND_FLAG_TEXTURE;
#ifdef PORT

#endif
}

void set_background_size(s16 startX, s16 startY, s16 sizeX, s16 sizeY) {
    gGameStatusPtr->backgroundFlags &= ~BACKGROUND_FLAG_TEXTURE;
    gGameStatusPtr->backgroundMaxX = startX;
    gGameStatusPtr->backgroundMaxY = startY;
    gGameStatusPtr->backgroundMinX = sizeX;
    gGameStatusPtr->backgroundMinY = sizeY;
}

u16 blend_background_channel(u16 arg0, s32 arg1, s32 alpha) {
    return arg0 + (arg1 - arg0) * alpha / 256;
}

void appendGfx_background_texture(void) {
    Camera* cam = &gCameras[gCurrentCameraID];
    u16 flags = 0;
    s32 fogR, fogG, fogB, fogA;
    u8 r1, g1, b1, a1;
    u8 r2, g2, b2;
    u16 blendedR, blendedG, blendedB;
    s32 i;

    f32 theta, sinTheta, cosTheta, scrollValue, f5, waveOffset;

    s32 bgMinX;
    s32 bgMinY;
    s32 bgMaxX;
    s32 bgMaxY;
    s32 lineHeight;
    s32 numLines;
    s32 extraHeight;

    s32 bgXOffset;
    s16 texOffsetY;

    u8* newvar;

    enum {
        BG_BLEND_NONE           = 0,
        BG_BLEND_HAS_FOG        = 1,
        BG_BLEND_SHOULD_LERP    = 2,
        BG_BLEND_SHOULD_BLEND   = 4,
    };

    if (is_world_fog_enabled()) {
        get_world_fog_color(&fogR, &fogG, &fogB, &fogA);
        flags = BG_BLEND_HAS_FOG;
        fogA = gGameStatusPtr->backgroundDarkness;
    }

    switch (*gBackgroundTintModePtr) {
        case ENV_TINT_NONE:
        case ENV_TINT_SHROUD:
            mdl_get_shroud_tint_params(&r1, &g1, &b1, &a1);
            if (a1 != 0) {
                flags |= BG_BLEND_SHOULD_LERP;
            }
            break;
        case ENV_TINT_DEPTH:
        case ENV_TINT_REMAP:
        default:
            mdl_get_remap_tint_params(&r1, &g1, &b1, &r2, &g2, &b2);
            if (!(r1 == 255 && g1 == 255 && b1 == 255 && r2 == 0 && g2 == 0 && b2 == 0)) {
                flags |= BG_BLEND_SHOULD_BLEND;
            }
            break;
    }

    switch (flags) {
        case BG_BLEND_NONE:
            gGameStatusPtr->backgroundFlags &= ~BACKGROUND_FLAG_FOG;
            break;
        case BG_BLEND_HAS_FOG:
            gGameStatusPtr->backgroundFlags |= BACKGROUND_FLAG_FOG;
            break;
        case BG_BLEND_SHOULD_LERP:
            gGameStatusPtr->backgroundFlags |= BACKGROUND_FLAG_FOG;
            fogR = r1;
            fogG = g1;
            fogB = b1;
            fogA = a1;
            break;
        case BG_BLEND_HAS_FOG | BG_BLEND_SHOULD_LERP:
            gGameStatusPtr->backgroundFlags |= BACKGROUND_FLAG_FOG;
            fogR = (fogR * (255 - a1) + r1 * a1) / 255;
            fogG = (fogG * (255 - a1) + g1 * a1) / 255;
            fogB = (fogB * (255 - a1) + b1 * a1) / 255;
            fogA = (fogA * (255 - a1) + a1 * a1) / 255;
            break;
        case BG_BLEND_SHOULD_BLEND:
            gGameStatusPtr->backgroundFlags |= BACKGROUND_FLAG_FOG;
            break;
    }

    if (gGameStatusPtr->backgroundFlags & BACKGROUND_FLAG_FOG) {
#ifdef PORT
        // The fog path is the only thing that rewrites the sky palette, and it is where the
        // magenta sky shows up. Log its inputs once per distinct set so I can see which is wrong.
        {
            static s32 sLastKey = -1;
            s32 key = (*gBackgroundTintModePtr << 24) | ((fogR & 0xFF) << 16) | ((fogG & 0xFF) << 8) | (fogB & 0xFF);
            if (key != sLastKey) {
                sLastKey = key;
                fprintf(stderr, "[bg-fog] tintMode=%d fog=(%d,%d,%d) fogA=%d pal[0]=0x%04X pal[64]=0x%04X\n",
                        *gBackgroundTintModePtr, fogR, fogG, fogB, fogA,
                        PAL_TO_NATIVE(gGameStatusPtr->backgroundPalette[0]),
                        PAL_TO_NATIVE(gGameStatusPtr->backgroundPalette[64]));
            }
        }
#endif
        switch (*gBackgroundTintModePtr) {
            case ENV_TINT_NONE:
            case ENV_TINT_SHROUD:
                if (fogA == 255) {
                    for (i = 0; i < ARRAY_COUNT(gBackgroundPalette); i++) {
                        gBackgroundPalette[i] = PAL_TO_BE(1);
                    }
                } else {
                    // lerp from background palette color to fog color based on fog alpha
                    for (i = 0; i < ARRAY_COUNT(gBackgroundPalette); i++) {
                        // NOTE: values after UNPACK range from [0,31], so we need to shift fog color into that range
                        u16 palColor = PAL_TO_NATIVE(gGameStatusPtr->backgroundPalette[i]);
                        blendedB = blend_background_channel(UNPACK_PAL_B(palColor), fogB >> 3, fogA);
                        blendedG = blend_background_channel(UNPACK_PAL_G(palColor), fogG >> 3, fogA);
                        blendedR = blend_background_channel(UNPACK_PAL_R(palColor), fogR >> 3, fogA);
                        gBackgroundPalette[i] = PAL_TO_BE(blendedB << 1 | blendedG << 6 | blendedR << 11 | 1);
                    }
                }
                break;
            case ENV_TINT_DEPTH:
            case ENV_TINT_REMAP:
            default:
                // the background color channels are remapped from [0,255] -> [min,max]
                for (i = 0; i < ARRAY_COUNT(gBackgroundPalette); i++) {
                    // NOTE: values after UNPACK range from [0,31], so we need to shift other colors into that range
                    u16 palColor = PAL_TO_NATIVE(gGameStatusPtr->backgroundPalette[i]);
                    blendedB = (b2 >> 3) + ((UNPACK_PAL_B(palColor) * b1 >> 3) >> 5);
                    blendedG = (g2 >> 3) + ((UNPACK_PAL_G(palColor) * g1 >> 3) >> 5);
                    blendedR = (r2 >> 3) + ((UNPACK_PAL_R(palColor) * r1 >> 3) >> 5);

                    if (blendedB > 0x1F) {
                        blendedB = 0x1F;
                    }
                    if (blendedG > 0x1F) {
                        blendedG = 0x1F;
                    }
                    if (blendedR > 0x1F) {
                        blendedR = 0x1F;
                    }
                    gBackgroundPalette[i] = PAL_TO_BE(blendedB << 1 | blendedG << 6 | blendedR << 11 | 1);
                }
                break;
        }
    }

    theta = clamp_angle(-cam->curBoomYaw);
    sinTheta = sin_deg(theta);
    cosTheta = cos_deg(theta);
    f5 = cosTheta * cam->lookAt_obj.x - sinTheta * cam->lookAt_obj.z + cam->leadAmount;
    scrollValue = -f5 * 0.25f;
    scrollValue += gGameStatusPtr->backgroundMaxX * theta * (1 / 90.0f);

    if (fabsf(scrollValue - gBackroundLastScrollValue) < 0.3f) {
        scrollValue = gBackroundLastScrollValue;
    } else {
        gBackroundLastScrollValue = scrollValue;
    }

    while (scrollValue < 0.0f) {
        scrollValue += gGameStatusPtr->backgroundMaxX * 32;
    }

    bgXOffset = gGameStatusPtr->backgroundXOffset = ((s32)scrollValue) % gGameStatusPtr->backgroundMaxX;
    bgMaxX = gGameStatusPtr->backgroundMaxX;
    bgMaxY = gGameStatusPtr->backgroundMaxY;
    bgMinX = gGameStatusPtr->backgroundMinX;
    bgMinY = gGameStatusPtr->backgroundMinY;

#ifdef PORT
    s32 wideExtra = 0;
    {
        f32 aspect = GameEngine_GetAspectRatio();
        if (aspect > (4.0f / 3.0f) + 0.01f) {
            wideExtra = (s32)(120.0f * aspect - 160.0f) + 2;
        }
    }
#endif

    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_COPY);
    gDPSetTexturePersp(gMainGfxPos++, G_TP_NONE);
    gDPSetTextureLUT(gMainGfxPos++, G_TT_RGBA16);
    gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGB, G_CC_DECALRGB);
    gDPSetRenderMode(gMainGfxPos++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetTextureFilter(gMainGfxPos++, G_TF_POINT);
    gDPPipeSync(gMainGfxPos++);

    if (!(gGameStatusPtr->backgroundFlags & BACKGROUND_FLAG_FOG)) {
        gDPLoadTLUT_pal256(gMainGfxPos++, gGameStatusPtr->backgroundPalette);
    } else {
        gDPLoadTLUT_pal256(gMainGfxPos++, gBackgroundPalette);
    }

    if (!gBackroundWaveEnabled) {
        lineHeight = 2048 / gGameStatusPtr->backgroundMaxX;
        numLines = gGameStatusPtr->backgroundMaxY / lineHeight;
        extraHeight = gGameStatusPtr->backgroundMaxY % lineHeight;
        for (i = 0; i < numLines; i++) {
            texOffsetY = gBackroundTextureYOffset + lineHeight * i;
            if (texOffsetY > gGameStatusPtr->backgroundMaxY) {
                texOffsetY -= gGameStatusPtr->backgroundMaxY;
            }
            gDPLoadTextureTile(gMainGfxPos++, gGameStatusPtr->backgroundRaster + bgMaxX * texOffsetY,
                               G_IM_FMT_CI, G_IM_SIZ_8b, bgMaxX, 6,
                               0, 0, 295, 5, 0,
                               G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

            gSPTextureRectangle(gMainGfxPos++, bgMinX * 4, (lineHeight * i + bgMinY) * 4,
                                                 (bgXOffset + bgMinX - 1) * 4, (lineHeight * i + lineHeight - 1 + bgMinY) * 4,
                                                 G_TX_RENDERTILE, (bgMaxX - bgXOffset) * 32, 0, 4096, 1024);
            gSPTextureRectangle(gMainGfxPos++, (bgXOffset + bgMinX) * 4, (lineHeight * i + bgMinY) * 4,
                                                 (bgMaxX + bgMinX - 1) * 4, (lineHeight * i + lineHeight - 1 + bgMinY) * 4,
                                                 G_TX_RENDERTILE, 0, 0, 4096, 1024);
#ifdef PORT
            if (wideExtra > 0) {
                port_draw_bg_extension(bgMinX, bgMaxX, bgXOffset, wideExtra, lineHeight * i + bgMinY,
                                       lineHeight * i + lineHeight - 1 + bgMinY);
            }
#endif
        }
        if (extraHeight != 0) {
            texOffsetY = gBackroundTextureYOffset + lineHeight * i;
            if (texOffsetY > gGameStatusPtr->backgroundMaxY) {
                texOffsetY -= gGameStatusPtr->backgroundMaxY;
            }
            gDPLoadTextureTile(gMainGfxPos++, gGameStatusPtr->backgroundRaster + bgMaxX * texOffsetY,
                               G_IM_FMT_CI, G_IM_SIZ_8b, bgMaxX, extraHeight,
                               0, 0, 295, extraHeight - 1, 0,
                               G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPTextureRectangle(gMainGfxPos++, bgMinX * 4, (lineHeight * i + bgMinY) * 4,
                                                 (bgXOffset + bgMinX - 1) * 4, (bgMaxY - 1 + bgMinY) * 4,
                                                 G_TX_RENDERTILE, (bgMaxX - bgXOffset) * 32, 0, 4096, 1024);
            gSPTextureRectangle(gMainGfxPos++, (bgXOffset + bgMinX) * 4, (lineHeight * i + bgMinY) * 4,
                                                 (bgMaxX + bgMinX - 1) * 4, (bgMaxY - 1 + bgMinY) * 4,
                                                 G_TX_RENDERTILE, 0, 0, 4096, 1024);
#ifdef PORT
            if (wideExtra > 0) {
                port_draw_bg_extension(bgMinX, bgMaxX, bgXOffset, wideExtra, lineHeight * i + bgMinY,
                                       bgMaxY - 1 + bgMinY);
            }
#endif
        }
    } else {
        lineHeight = 6;
        numLines = gGameStatusPtr->backgroundMaxY / lineHeight;
        extraHeight = gGameStatusPtr->backgroundMaxY % lineHeight;
        gBackroundWavePhase += TAU / 60; // 60 frames period
        for (i = 0; i < numLines; i++) {
            waveOffset = sin_rad(gBackroundWavePhase + i * (TAU / 15)) * 3.0f;
            bgXOffset = 2.0f * (gGameStatusPtr->backgroundXOffset + waveOffset);
            texOffsetY = gBackroundTextureYOffset + lineHeight * i;
            if (texOffsetY > gGameStatusPtr->backgroundMaxY) {
                texOffsetY -= gGameStatusPtr->backgroundMaxY;
            }
            gDPLoadTextureTile(gMainGfxPos++, gGameStatusPtr->backgroundRaster + bgMaxX * texOffsetY,
                               G_IM_FMT_CI, G_IM_SIZ_8b, bgMaxX, 6,
                               0, 0, 295, 5, 0,
                               G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

            gSPTextureRectangle(gMainGfxPos++, bgMinX * 4, (lineHeight * i + bgMinY) * 4,
                                                 (2 * bgXOffset + (bgMinX - 1)) * 4, (lineHeight * i + lineHeight - 1 + bgMinY) * 4,
                                                 G_TX_RENDERTILE, bgMaxX * 32 - bgXOffset * 16, 0, 4096, 1024);
            gSPTextureRectangle(gMainGfxPos++, bgXOffset * 2 + bgMinX * 4, (lineHeight * i + bgMinY) * 4,
                                                 (bgMaxX + bgMinX - 1) * 4, (lineHeight * i + lineHeight - 1 + bgMinY) * 4,
                                                 G_TX_RENDERTILE, 0, 0, 4096, 1024);
        }
        if (extraHeight != 0) {
            waveOffset = sin_rad(gBackroundWavePhase + i * (TAU / 15)) * 3.0f;
            bgXOffset = 2.0f * (gGameStatusPtr->backgroundXOffset + waveOffset);
            texOffsetY = gBackroundTextureYOffset + lineHeight * i;
            if (texOffsetY > gGameStatusPtr->backgroundMaxY) {
                texOffsetY -= gGameStatusPtr->backgroundMaxY;
            }
            gDPLoadTextureTile(gMainGfxPos++, gGameStatusPtr->backgroundRaster + bgMaxX * texOffsetY,
                               G_IM_FMT_CI, G_IM_SIZ_8b, bgMaxX, extraHeight,
                               0, 0, 295, extraHeight - 1, 0,
                               G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPTextureRectangle(gMainGfxPos++, bgMinX * 4, (lineHeight * i + bgMinY) * 4,
                                                 (2 * bgXOffset + (bgMinX - 1)) * 4, (bgMaxY - 1 + bgMinY) * 4, /// @bug xh = 2 * bgXOffset + (bgMinX - 1) * 4
                                                 G_TX_RENDERTILE, bgMaxX * 32 - bgXOffset * 16, 0, 4096, 1024);
            gSPTextureRectangle(gMainGfxPos++, bgXOffset * 2  + bgMinX * 4, (lineHeight * i + bgMinY) * 4,
                                                 (bgMaxX + bgMinX - 1) * 4, (bgMaxY - 1 + bgMinY) * 4, /// @bug xh = 2 * bgXOffset + (bgMinX - 1) * 4
                                                 G_TX_RENDERTILE, 0, 0, 4096, 1024);
        }
    }
}

void enable_background_wave(void) {
    gBackroundWaveEnabled = true;
}

void disable_background_wave(void) {
    gBackroundWaveEnabled = false;
}

// TODO figure out why it is needed
static const f32 rodata_padding[] = { 0.0f, 0.0f };
