#include "common.h"
#include "effects_internal.h"
#include "nu/nusys.h"

extern Vtx D_09000428_3B9E98[];
extern Gfx D_09000528_3B9F98[];
extern Gfx D_09000570_3B9FE0[];

void underwater_init(EffectInstance* effect);
void underwater_update(EffectInstance* effect);
void underwater_render(EffectInstance* effect);
void underwater_appendGfx(void* effect);

EffectInstance* underwater_main(s32 arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, s32 arg5) {
    EffectBlueprint bp;
    EffectInstance* effect;
    UnderwaterFXData* data;
    s32 numParts = 1;
    s32 i;
    s32 j;

    bp.init = underwater_init;
    bp.update = underwater_update;
    bp.renderScene = underwater_render;
    bp.unk_00 = 0;
    bp.renderUI = nullptr;
    bp.effectID = EFFECT_UNDERWATER;

    effect = create_effect_instance(&bp);
    effect->numParts = numParts;
    data = effect->data.underwater = general_heap_malloc(numParts * sizeof(*data));
    ASSERT(effect->data.underwater != nullptr);

    data->unk_00 = arg0;
    data->lifeTime = 0;
    if (arg5 <= 0) {
        data->timeLeft = 1000;
    } else {
        data->timeLeft = arg5;
    }
    data->waterColor.a = 0;
    data->unk_04 = arg1;
    data->unk_08 = arg2;
    data->unk_0C = arg3;
    data->unk_18 = arg4;
    data->waterColor.r = 10;
    data->waterColor.g = 110;
    data->waterColor.b = 255;
    data->unk_20.r = 190;
    data->unk_20.g = 220;
    data->unk_20.b = 255;

    for (i = 0; i < ARRAY_COUNT(data->unk_23); i++) {
        for (j = 0; j < ARRAY_COUNT(data->unk_23[0]); j++) {
            data->unk_23[i][j] = 0;
            data->unk_11A[i][j] = 0;
        }
    }

    return effect;
}

void underwater_init(EffectInstance* effect) {
}

void underwater_update(EffectInstance* effect) {
    UnderwaterFXData* data = effect->data.underwater;
    s32 timeLeft;
    s32 lifeTime;
    s32 alpha;
    f32 factor;
    s32 i;
    s32 j;

    if (effect->flags & FX_INSTANCE_FLAG_DISMISS) {
        effect->flags &= ~FX_INSTANCE_FLAG_DISMISS;
        data->timeLeft = 32;
    }

    if (data->timeLeft < 1000) {
        data->timeLeft--;
    }

    data->lifeTime++;

    if (data->timeLeft < 0) {
        remove_effect(effect);
        return;
    }

    timeLeft = data->timeLeft;
    lifeTime = data->lifeTime;

    alpha = 255;
    if (timeLeft < 32) {
        alpha = timeLeft * 8;
    }
    if (lifeTime < 16) {
        alpha = lifeTime * 16 + 15;
    }

    data->waterColor.a = (f32) alpha;
    factor = (f32) alpha / 255.0f;

    for (i = 1; i < ARRAY_COUNT(data->unk_23) - 1; i++) {
        data->unk_23[i][6] = sin_deg(-((lifeTime - i) * 20)) * -64.0f * factor;
    }

    for (i = 1; i < ARRAY_COUNT(data->unk_23) - 1; i++) {
        for (j = 1; j < ARRAY_COUNT(data->unk_23[0]) - 1; j++) {
            f32 m11 = data->unk_23[i][j] * 4.0f;
            f32 m21 = data->unk_23[i + 1][j];
            f32 m01 = data->unk_23[i - 1][j];
            f32 m12 = data->unk_23[i][j + 1];
            f32 m10 = data->unk_23[i][j - 1];
            f32 m22 = data->unk_23[i + 1][j + 1];
            f32 m02 = data->unk_23[i - 1][j + 1];
            f32 m20 = data->unk_23[i + 1][j - 1];
            f32 m00 = data->unk_23[i - 1][j - 1];
            f32 temp1 = m21 + m01 + m12 + m10 - m11;
            f32 temp2 = (m22 + m02 + m20 + m00) * 0.5;

            data->unk_11A[i][j] += temp1 + temp2 - m11 * 0.7;
            data->unk_11A[i][j] *= 0.98;

            if (timeLeft < 32) {
                data->unk_23[i][j] *= factor;
            }
        }
    }

    for (i = 1; i < ARRAY_COUNT(data->unk_23) - 1; i++) {
        for (j = 1; j < ARRAY_COUNT(data->unk_23[0]) - 1; j++) {
            data->unk_23[i][j] += data->unk_11A[i][j] * 0.02;
        }
    }
}

void underwater_render(EffectInstance* effect) {
    RenderTask renderTask;
    RenderTask* retTask;

    renderTask.appendGfx = underwater_appendGfx;
    renderTask.appendGfxArg = effect;
    renderTask.dist = 100;
    renderTask.renderMode = RENDER_MODE_CLOUD_NO_ZCMP;

    retTask = queue_render_task(&renderTask);
}

void func_E00BA618(void) {
}

#ifdef PORT
// The N64 copies the CFB into the z-buffer and redraws it warped. Here the frame is
// copied into a GPU framebuffer, bound through a registered sentinel address, and
// redrawn as a grid of screen-space quads.
extern int gfx_create_framebuffer(u32 width, u32 height, u32 nativeWidth, u32 nativeHeight, u8 resize);
extern void gfx_register_fb_texture(const void* cpuAddr, int fbId);
extern s16 OTRGetRectDimensionFromLeftEdge(f32 v);
extern s16 OTRGetRectDimensionFromRightEdge(f32 v);

#define UW_COLS 18
#define UW_ROWS 12
#define UW_CELL 16

static u16 sUnderwaterSentinel[SCREEN_WIDTH * SCREEN_HEIGHT];
static s32 sUnderwaterFbId = -1;
static Vtx sUnderwaterVtx[2][UW_COLS * UW_ROWS * 4];
static Vp sUnderwaterFullVp = { .vp = {
    .vscale = { 640, 480, 511, 0 },
    .vtrans = { 640, 480, 511, 0 },
} };

// screen x (320 space) to S 10.5 across the full mirror, which spans the visible width
static s32 underwater_mirror_s(s32 screenX, s32 visLeft, s32 visWidth) {
    return (s32)(32.0f * SCREEN_WIDTH * (screenX - visLeft) / visWidth);
}

static void port_underwater_appendGfx(void* effect) {
    UnderwaterFXData* data = ((EffectInstance*)effect)->data.underwater;
    Vtx* vtxBuf = sUnderwaterVtx[gCurrentDisplayContextIndex];
    s32 visLeft = OTRGetRectDimensionFromLeftEdge(0);
    s32 visRight = OTRGetRectDimensionFromRightEdge(0);
    s32 visWidth = visRight - visLeft;
    Matrix4f mtx;
    s32 i, j, k;
    s32 vtxPos = 0;

    if (sUnderwaterFbId < 0) {
        sUnderwaterFbId = gfx_create_framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        if (sUnderwaterFbId < 0) {
            return;
        }
        gfx_register_fb_texture(sUnderwaterSentinel, sUnderwaterFbId);
        fprintf(stderr, "[underwater] mirror fb=%d vis=%d..%d\n", sUnderwaterFbId, visLeft, visRight);
    }
    if (visWidth < 1) {
        visLeft = 0;
        visRight = SCREEN_WIDTH;
        visWidth = SCREEN_WIDTH;
    }

    gDPPipeSync(gMainGfxPos++);
    gDPCopyFB(gMainGfxPos++, sUnderwaterFbId, 0, false, NULL);

    gDPSetPrimColor(gMainGfxPos++, 0, 0, data->waterColor.r, data->waterColor.g, data->waterColor.b, data->waterColor.a >> 1);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, PM_CC_48, PM_CC_48);
    gDPSetRenderMode(gMainGfxPos++, CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | G_RM_PASS, CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetTexturePersp(gMainGfxPos++, G_TP_NONE);
    gDPSetTextureFilter(gMainGfxPos++, G_TF_BILERP);
    gDPSetTextureLUT(gMainGfxPos++, G_TT_NONE);
    gDPSetTextureDetail(gMainGfxPos++, G_TD_CLAMP);
    gDPSetTextureLOD(gMainGfxPos++, G_TL_TILE);
    gSPTexture(gMainGfxPos++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);

    // the tile size is what normalises the UVs, so declare the whole frame
    gDPLoadTextureTile(gMainGfxPos++, sUnderwaterSentinel, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, SCREEN_HEIGHT,
                       0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, 0, G_TX_CLAMP, G_TX_CLAMP,
                       G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    gSPViewport(gMainGfxPos++, &sUnderwaterFullVp);
    // screen-space ortho in 10.2 units so the wave offsets keep sub-pixel precision
    guOrthoF(mtx, 0.0f, SCREEN_WIDTH * 4.0f, SCREEN_HEIGHT * 4.0f, 0.0f, -500.0f, 500.0f, 1.0f);
    guMtxF2L(mtx, &gDisplayContext->matrixStack[gMatrixListPos]);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    guMtxIdentF(mtx);
    guMtxF2L(mtx, &gDisplayContext->matrixStack[gMatrixListPos]);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPClearGeometryMode(gMainGfxPos++, G_CULL_BOTH | G_LIGHTING);
    gSPSetGeometryMode(gMainGfxPos++, G_SHADE | G_SHADING_SMOOTH);

    for (j = 0; j < UW_ROWS; j++) {
        s32 y = j * UW_CELL + 24;

        for (i = 0; i < UW_COLS; i++) {
            s32 x = i * UW_CELL + 16;
            // edge cells run out to the screen edge; update leaves the outer ring undisplaced
            s32 srcLeft = (i == 0) ? visLeft : x;
            s32 srcRight = (i == UW_COLS - 1) ? visRight : x + UW_CELL;
            s32 srcTop = (j == 0) ? 0 : y;
            s32 srcBottom = (j == UW_ROWS - 1) ? SCREEN_HEIGHT : y + UW_CELL;
            s32 uLeft = underwater_mirror_s(srcLeft, visLeft, visWidth);
            s32 uRight = underwater_mirror_s(srcRight, visLeft, visWidth);
            Vtx* v = &vtxBuf[vtxPos];

            v[0].v.ob[0] = srcLeft * 4;
            v[0].v.ob[1] = srcTop * 4 + data->unk_23[i][j];
            v[0].v.tc[0] = uLeft;
            v[0].v.tc[1] = srcTop * 32;
            v[1].v.ob[0] = srcRight * 4;
            v[1].v.ob[1] = srcTop * 4 + data->unk_23[i + 1][j];
            v[1].v.tc[0] = uRight;
            v[1].v.tc[1] = srcTop * 32;
            v[2].v.ob[0] = srcLeft * 4;
            v[2].v.ob[1] = srcBottom * 4 + data->unk_23[i][j + 1];
            v[2].v.tc[0] = uLeft;
            v[2].v.tc[1] = srcBottom * 32;
            v[3].v.ob[0] = srcRight * 4;
            v[3].v.ob[1] = srcBottom * 4 + data->unk_23[i + 1][j + 1];
            v[3].v.tc[0] = uRight;
            v[3].v.tc[1] = srcBottom * 32;

            // PM_CC_48 only reads shade alpha; the tint is all prim color
            for (k = 0; k < 4; k++) {
                v[k].v.ob[2] = 0;
                v[k].v.flag = 0;
                v[k].v.cn[0] = 0;
                v[k].v.cn[1] = 0;
                v[k].v.cn[2] = 0;
                v[k].v.cn[3] = 255;
            }

            gSPVertex(gMainGfxPos++, v, 4, 0);
            gSP2Triangles(gMainGfxPos++, 0, 3, 1, 0, 0, 2, 3, 0);
            vtxPos += 4;
        }
    }

    gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
    gDPPipeSync(gMainGfxPos++);
    gSPViewport(gMainGfxPos++, &gCameras[gCurrentCameraID].vp);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->camPerspMatrix[gCurrentCameraID], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gDPPipeSync(gMainGfxPos++);
}
#endif

void underwater_appendGfx(void* effect) {
#ifdef PORT
    port_underwater_appendGfx(effect);
    return;
#endif
    UnderwaterFXData* data = ((EffectInstance*)effect)->data.underwater;
    s32 alpha = data->waterColor.a;
    s32 x, y;
    s32 dxRight, dxLeft, dyTop, dyBottom;
    s32 edgeX, edgeY;
    s32 vtxIdx;
    Matrix4f mtx;
    Matrix4f unused_matrix;
    s32 i, j;

    gDPPipeSync(gMainGfxPos++);
    gSPSegment(gMainGfxPos++, 0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, nuGfxZBuffer);
    gSPDisplayList(gMainGfxPos++, D_09000528_3B9F98);

    // copy image from framebuffer to zbuffer
    for (i = 0; i < 40; i++) {
        gDPLoadTextureTile(
            gMainGfxPos++, nuGfxCfb_ptr + SCREEN_WIDTH * i * 6,
            G_IM_FMT_RGBA, G_IM_SIZ_16b,
            SCREEN_WIDTH, 6,
            0, 0, SCREEN_WIDTH - 1, 5, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(
            gMainGfxPos++,
            0 * 4, (i * 6) * 4,
            (SCREEN_WIDTH - 1) * 4, (i * 6 + 5) * 4,
            G_TX_RENDERTILE, 0, 0, 0x1000, 0x0400);
        gDPPipeSync(gMainGfxPos++);
    }

    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, nuGfxCfb_ptr);
    gDPSetPrimColor(gMainGfxPos++, 0, 0, data->waterColor.r, data->waterColor.g, data->waterColor.b, alpha >> 1);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, PM_CC_48, PM_CC_48);
    gDPSetTextureImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, OS_K0_TO_PHYSICAL(nuGfxZBuffer));
    gDPSetRenderMode(gMainGfxPos++, CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | G_RM_PASS, CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetTexturePersp(gMainGfxPos++, G_TP_PERSP);
    gDPSetTextureFilter(gMainGfxPos++, G_TF_BILERP);

    guFrustumF(mtx, -80.0f, 80.0f, 60.0f, -60.0f, 160.0f, 640.0f, 1.0f);
    guMtxF2L(mtx, &gDisplayContext->matrixStack[gMatrixListPos]);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPClearGeometryMode(gMainGfxPos++, G_CULL_BOTH | G_LIGHTING);
    gSPSetGeometryMode(gMainGfxPos++, G_SHADE | G_SHADING_SMOOTH);
    gSPVertex(gMainGfxPos++, D_09000428_3B9E98, 16, 0);

    for (j = 0; j < 12; j++) {
        y = j * 16 + 24;
        edgeY = false;

        if (j == 0) {
            dyTop = -4;
            edgeY = true;
        } else {
            dyTop = 0;
        }

        if (j == 11) {
            dyBottom = 4;
            edgeY = true;
        } else {
            dyBottom = 0;
        }

        for (i = 0; i < 18; i++) {
            x = i * 16 + 16;
            edgeX = false;

            if (i == 0) {
                dxLeft = -4;
                edgeX = true;
            } else {
                dxLeft = 0;
            }

            if (i == 17) {
                dxRight = 4;
                edgeX = true;
            } else {
                dxRight = 0;
            }

            gDPSetTile(
                gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b,
                (((((x + dxRight + 16) - (x + dxLeft)) + 1) * 2) + 7) >> 3, 0x0000,
                G_TX_LOADTILE, 0,
                G_TX_NOMIRROR | G_TX_CLAMP, 6, G_TX_NOLOD,
                G_TX_NOMIRROR | G_TX_CLAMP, 6, G_TX_NOLOD);
            gDPLoadSync(gMainGfxPos++);
            gDPLoadTile(gMainGfxPos++, G_TX_LOADTILE,
                (x + dxLeft) * 4,
                (y + dyTop) * 4,
                (x + dxRight + 16) * 4,
                (y + dyBottom + 16) * 4);
            gDPPipeSync(gMainGfxPos++);
            gDPSetTile(
                gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b,
                (((((x + dxRight + 16) - (x + dxLeft)) + 1) * 2) + 7) >> 3, 0x0000,
                G_TX_RENDERTILE, 0,
                G_TX_NOMIRROR | G_TX_CLAMP, 6, G_TX_NOLOD,
                G_TX_NOMIRROR | G_TX_CLAMP, 6, G_TX_NOLOD);
            gDPSetTileSize(gMainGfxPos++, G_TX_RENDERTILE,
                0, 0, (x + dxRight + 31) * 4, (y + dyBottom + 31) * 4);

            if (edgeX && edgeY) {
                vtxIdx = 12;
            } else {
                vtxIdx = 4;
                if (!edgeX) {
                    if (!edgeY) {
                        vtxIdx = 0;
                    } else {
                        vtxIdx = 8;
                    }
                }
            }

            gSPModifyVertex(gMainGfxPos++, vtxIdx    , G_MWO_POINT_XYSCREEN, ((x + dxLeft        ) << 0x12) | ((y + dyTop          ) * 4 + data->unk_23[i    ][j    ]));
            gSPModifyVertex(gMainGfxPos++, vtxIdx + 1, G_MWO_POINT_XYSCREEN, ((x + dxRight + 0x10) << 0x12) | ((y + dyTop          ) * 4 + data->unk_23[i + 1][j    ]));
            gSPModifyVertex(gMainGfxPos++, vtxIdx + 2, G_MWO_POINT_XYSCREEN, ((x + dxLeft        ) << 0x12) | ((y + dyBottom + 0x10) * 4 + data->unk_23[i    ][j + 1]));
            gSPModifyVertex(gMainGfxPos++, vtxIdx + 3, G_MWO_POINT_XYSCREEN, ((x + dxRight + 0x10) << 0x12) | ((y + dyBottom + 0x10) * 4 + data->unk_23[i + 1][j + 1]));
            gSP2Triangles(gMainGfxPos++, vtxIdx, vtxIdx + 3, vtxIdx + 1, 0, vtxIdx, vtxIdx + 2, vtxIdx + 3, 0);
            gDPPipeSync(gMainGfxPos++);
        }
    }

    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, nuGfxZBuffer);
    gSPDisplayList(gMainGfxPos++, D_09000570_3B9FE0);
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, nuGfxCfb_ptr);
    gDPPipeSync(gMainGfxPos++);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->camPerspMatrix[gCurrentCameraID], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gDPPipeSync(gMainGfxPos++);
}
