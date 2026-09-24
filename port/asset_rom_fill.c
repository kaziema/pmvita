/**
 * asset_rom_fill.c - fill asset stubs that nothing else loads
 *
 * asset_data_stubs.c declares every INCLUDE_IMG/PAL symbol as a zero-filled array, and
 * ui_texture_loader.c fills most of them from the ROM at boot. These are the ones it never
 * covered, so they drew as blank (fully transparent) textures: the Sleepy Sheep, the action
 * command graphics, the end of chapter window, and others.
 *
 * Offsets come from the decomp's ver/us/splat.yaml, matched by asset path, and each image's
 * byte size was checked against splat's width, height and format before it went in here.
 */
#include "ultra64.h"
#include <string.h>

extern void nuPiReadRom(u32 romAddr, void* dest, u32 size);

extern u8 ResetTilesImg[];
extern u8 battle_action_cmd_three_chances_0_pal[];
extern u8 battle_action_cmd_three_chances_0_png[];
extern u8 battle_action_cmd_three_chances_1_pal[];
extern u8 battle_action_cmd_three_chances_1_png[];
extern u8 battle_action_cmd_three_chances_2_pal[];
extern u8 battle_action_cmd_three_chances_2_png[];
extern u8 battle_action_cmd_three_chances_3_pal[];
extern u8 battle_action_cmd_three_chances_3_png[];
extern u8 battle_action_cmd_three_chances_4_pal[];
extern u8 battle_action_cmd_three_chances_4_png[];
extern u8 battle_action_cmd_three_chances_block_pal[];
extern u8 battle_action_cmd_three_chances_block_png[];
extern u8 battle_action_cmd_three_chances_circle_pal[];
extern u8 battle_action_cmd_three_chances_circle_png[];
extern u8 battle_action_cmd_three_chances_cloud_pal[];
extern u8 battle_action_cmd_three_chances_cloud_png[];
extern u8 battle_action_cmd_whirlwind_1_pal[];
extern u8 battle_action_cmd_whirlwind_1_png[];
extern u8 battle_action_cmd_whirlwind_2_pal[];
extern u8 battle_action_cmd_whirlwind_2_png[];
extern u8 battle_action_cmd_whirlwind_3_pal[];
extern u8 battle_action_cmd_whirlwind_3_png[];
extern u8 battle_action_cmd_whirlwind_4_pal[];
extern u8 battle_action_cmd_whirlwind_4_png[];
extern u8 battle_action_cmd_whirlwind_5_pal[];
extern u8 battle_action_cmd_whirlwind_5_png[];
extern u8 battle_action_cmd_whirlwind_6_pal[];
extern u8 battle_action_cmd_whirlwind_6_png[];
extern u8 battle_action_cmd_whirlwind_7_pal[];
extern u8 battle_action_cmd_whirlwind_7_png[];
extern u8 battle_action_cmd_whirlwind_bubble_pal[];
extern u8 battle_action_cmd_whirlwind_bubble_png[];
extern u8 battle_area_sam2_actor_img_pal[];
extern u8 battle_area_sam2_actor_img_png[];
extern u8 battle_item_coconut_pal[];
extern u8 battle_item_coconut_png[];
extern u8 battle_item_dusty_hammer_pal[];
extern u8 battle_item_dusty_hammer_png[];
extern u8 battle_item_egg_missile1_pal[];
extern u8 battle_item_egg_missile1_png[];
extern u8 battle_item_egg_missile2_pal[];
extern u8 battle_item_egg_missile2_png[];
extern u8 battle_item_egg_missile3_pal[];
extern u8 battle_item_egg_missile3_png[];
extern u8 battle_item_egg_missile4_pal[];
extern u8 battle_item_egg_missile4_png[];
extern u8 battle_item_insecticide_herb_pal[];
extern u8 battle_item_insecticide_herb_png[];
extern u8 battle_item_mystery_pal[];
extern u8 battle_item_mystery_png[];
extern u8 battle_item_pebble_pal[];
extern u8 battle_item_pebble_png[];
extern u8 battle_item_sleepy_sheep1_pal[];
extern u8 battle_item_sleepy_sheep1_png[];
extern u8 battle_item_sleepy_sheep2_pal[];
extern u8 battle_item_sleepy_sheep2_png[];
extern u8 battle_item_sleepy_sheep3_pal[];
extern u8 battle_item_sleepy_sheep3_png[];
extern u8 battle_item_strange_cake1_pal[];
extern u8 battle_item_strange_cake1_png[];
extern u8 battle_item_strange_cake2_pal[];
extern u8 battle_item_strange_cake2_png[];
extern u8 battle_item_strange_cake3_pal[];
extern u8 battle_item_strange_cake3_png[];
extern u8 battle_move_hammer_throw_basic_hammer_pal[];
extern u8 battle_move_hammer_throw_dusty_hammer_pal[];
extern u8 battle_move_hammer_throw_super_hammer_pal[];
extern u8 battle_move_hammer_throw_ultra_hammer_pal[];
extern u8 kmr_23_window_ul_img[];
extern u8 mgm_01_panel_1_coin_img[];
extern u8 mgm_01_panel_1_coin_pal[];
extern u8 mgm_01_panel_5_coins_img[];
extern u8 mgm_01_panel_5_coins_pal[];
extern u8 mgm_01_panel_bowser_img[];
extern u8 mgm_01_panel_bowser_pal[];
extern u8 mgm_01_panel_times_5_img[];
extern u8 mgm_01_panel_times_5_pal[];
extern u8 mgm_02_panel_peach_img[];
extern u8 mgm_02_panel_peach_pal[];
extern u8 pulse_stone_icon_1_pal[];
extern u8 pulse_stone_icon_2_pal[];
extern u8 sam_05_monstar_pal[];
extern u8 pulse_stone_icon_img[];
extern u8 sam_05_monstar_png[];

typedef struct {
    u8* dest;
    u32 rom;
    u32 size;
} PortAssetFill;

static const PortAssetFill sAssetFills[] = {
    { ResetTilesImg, 0x4F210, 512 }, /* i4 */
    { battle_action_cmd_three_chances_0_pal, 0x42BB30, 32 }, /* palette */
    { battle_action_cmd_three_chances_0_png, 0x42BA10, 288 }, /* ci4 */
    { battle_action_cmd_three_chances_1_pal, 0x42B770, 32 }, /* palette */
    { battle_action_cmd_three_chances_1_png, 0x42B650, 288 }, /* ci4 */
    { battle_action_cmd_three_chances_2_pal, 0x42B8B0, 32 }, /* palette */
    { battle_action_cmd_three_chances_2_png, 0x42B790, 288 }, /* ci4 */
    { battle_action_cmd_three_chances_3_pal, 0x42B9F0, 32 }, /* palette */
    { battle_action_cmd_three_chances_3_png, 0x42B8D0, 288 }, /* ci4 */
    { battle_action_cmd_three_chances_4_pal, 0x42C510, 32 }, /* palette */
    { battle_action_cmd_three_chances_4_png, 0x42C3F0, 288 }, /* ci4 */
    { battle_action_cmd_three_chances_block_pal, 0x42BD50, 32 }, /* palette */
    { battle_action_cmd_three_chances_block_png, 0x42BB50, 512 }, /* ci4 */
    { battle_action_cmd_three_chances_circle_pal, 0x42C090, 32 }, /* palette */
    { battle_action_cmd_three_chances_circle_png, 0x42BD70, 800 }, /* ci4 */
    { battle_action_cmd_three_chances_cloud_pal, 0x42C3D0, 32 }, /* palette */
    { battle_action_cmd_three_chances_cloud_png, 0x42C0B0, 800 }, /* ci4 */
    { battle_action_cmd_whirlwind_1_pal, 0x424670, 32 }, /* palette */
    { battle_action_cmd_whirlwind_1_png, 0x424550, 288 }, /* ci4 */
    { battle_action_cmd_whirlwind_2_pal, 0x4247B0, 32 }, /* palette */
    { battle_action_cmd_whirlwind_2_png, 0x424690, 288 }, /* ci4 */
    { battle_action_cmd_whirlwind_3_pal, 0x4248F0, 32 }, /* palette */
    { battle_action_cmd_whirlwind_3_png, 0x4247D0, 288 }, /* ci4 */
    { battle_action_cmd_whirlwind_4_pal, 0x424A30, 32 }, /* palette */
    { battle_action_cmd_whirlwind_4_png, 0x424910, 288 }, /* ci4 */
    { battle_action_cmd_whirlwind_5_pal, 0x424B70, 32 }, /* palette */
    { battle_action_cmd_whirlwind_5_png, 0x424A50, 288 }, /* ci4 */
    { battle_action_cmd_whirlwind_6_pal, 0x424CB0, 32 }, /* palette */
    { battle_action_cmd_whirlwind_6_png, 0x424B90, 288 }, /* ci4 */
    { battle_action_cmd_whirlwind_7_pal, 0x424DF0, 32 }, /* palette */
    { battle_action_cmd_whirlwind_7_png, 0x424CD0, 288 }, /* ci4 */
    { battle_action_cmd_whirlwind_bubble_pal, 0x425350, 32 }, /* palette */
    { battle_action_cmd_whirlwind_bubble_png, 0x424E10, 1344 }, /* ci4 */
    { battle_area_sam2_actor_img_pal, 0x63F478, 32 }, /* palette */
    { battle_area_sam2_actor_img_png, 0x63F278, 512 }, /* ci4 */
    { battle_item_coconut_pal, 0x730120, 32 }, /* palette */
    { battle_item_coconut_png, 0x72FF20, 512 }, /* ci4 */
    { battle_item_dusty_hammer_pal, 0x7183D0, 32 }, /* palette */
    { battle_item_dusty_hammer_png, 0x7181D0, 512 }, /* ci4 */
    { battle_item_egg_missile1_pal, 0x729990, 32 }, /* palette */
    { battle_item_egg_missile1_png, 0x729790, 512 }, /* ci4 */
    { battle_item_egg_missile2_pal, 0x729BB0, 32 }, /* palette */
    { battle_item_egg_missile2_png, 0x7299B0, 512 }, /* ci4 */
    { battle_item_egg_missile3_pal, 0x729DD0, 32 }, /* palette */
    { battle_item_egg_missile3_png, 0x729BD0, 512 }, /* ci4 */
    { battle_item_egg_missile4_pal, 0x729FF0, 32 }, /* palette */
    { battle_item_egg_missile4_png, 0x729DF0, 512 }, /* ci4 */
    { battle_item_insecticide_herb_pal, 0x72B330, 32 }, /* palette */
    { battle_item_insecticide_herb_png, 0x72B130, 512 }, /* ci4 */
    { battle_item_mystery_pal, 0x72D6C0, 32 }, /* palette */
    { battle_item_mystery_png, 0x72D4C0, 512 }, /* ci4 */
    { battle_item_pebble_pal, 0x71A370, 32 }, /* palette */
    { battle_item_pebble_png, 0x71A170, 512 }, /* ci4 */
    { battle_item_sleepy_sheep1_pal, 0x71FAC0, 32 }, /* palette */
    { battle_item_sleepy_sheep1_png, 0x71F580, 1344 }, /* ci4 */
    { battle_item_sleepy_sheep2_pal, 0x720020, 32 }, /* palette */
    { battle_item_sleepy_sheep2_png, 0x71FAE0, 1344 }, /* ci4 */
    { battle_item_sleepy_sheep3_pal, 0x720580, 32 }, /* palette */
    { battle_item_sleepy_sheep3_png, 0x720040, 1344 }, /* ci4 */
    { battle_item_strange_cake1_pal, 0x732670, 32 }, /* palette */
    { battle_item_strange_cake1_png, 0x732470, 512 }, /* ci4 */
    { battle_item_strange_cake2_pal, 0x732890, 32 }, /* palette */
    { battle_item_strange_cake2_png, 0x732690, 512 }, /* ci4 */
    { battle_item_strange_cake3_pal, 0x732AB0, 32 }, /* palette */
    { battle_item_strange_cake3_png, 0x7328B0, 512 }, /* ci4 */
    { battle_move_hammer_throw_basic_hammer_pal, 0x7548B8, 32 }, /* palette */
    { battle_move_hammer_throw_dusty_hammer_pal, 0x754698, 32 }, /* palette */
    { battle_move_hammer_throw_super_hammer_pal, 0x754AD8, 32 }, /* palette */
    { battle_move_hammer_throw_ultra_hammer_pal, 0x754CF8, 32 }, /* palette */
    { kmr_23_window_ul_img, 0x9090F8, 256 }, /* ia8, all four corners: ul ur ll lr */
    { mgm_01_panel_1_coin_img, 0xE15440, 512 }, /* ci4 */
    { mgm_01_panel_1_coin_pal, 0xE15640, 32 }, /* palette */
    { mgm_01_panel_5_coins_img, 0xE15660, 512 }, /* ci4 */
    { mgm_01_panel_5_coins_pal, 0xE15860, 32 }, /* palette */
    { mgm_01_panel_bowser_img, 0xE15AA0, 512 }, /* ci4 */
    { mgm_01_panel_bowser_pal, 0xE15CA0, 32 }, /* palette */
    { mgm_01_panel_times_5_img, 0xE15880, 512 }, /* ci4 */
    { mgm_01_panel_times_5_pal, 0xE15A80, 32 }, /* palette */
    { mgm_02_panel_peach_img, 0xE1E020, 512 }, /* ci4 */
    { mgm_02_panel_peach_pal, 0xE1E220, 32 }, /* palette */
    { pulse_stone_icon_1_pal, 0xE224D0, 32 }, /* palette */
    { pulse_stone_icon_2_pal, 0xE224F0, 32 }, /* palette */
    { sam_05_monstar_pal, 0xD1B750, 32 }, /* palette */
    { pulse_stone_icon_img, 0xE21EB0, 1568 }, /* ci4 */
    { sam_05_monstar_png, 0xD1AF50, 512 }, /* ci4 */
};

/* Monstar's bubble vertices. N64 vertices are 16 bytes with s16 positions; this build uses
 * float positions, so they have to be converted, and the array has to be the real size. */
#define MONSTAR_BUBBLES_ROM 0x648860
#define MONSTAR_BUBBLES_COUNT 66
Vtx monstar_bubbles[MONSTAR_BUBBLES_COUNT];

static u16 port_be16(const u8* p) {
    return (u16)((p[0] << 8) | p[1]);
}

static void port_fill_monstar_bubbles(void) {
    static u8 raw[MONSTAR_BUBBLES_COUNT * 16];
    s32 i;

    nuPiReadRom(MONSTAR_BUBBLES_ROM, raw, sizeof(raw));
    for (i = 0; i < MONSTAR_BUBBLES_COUNT; i++) {
        const u8* v = raw + i * 16;
        Vtx_t* d = &monstar_bubbles[i].v;

        d->ob[0] = (s16)port_be16(v + 0);
        d->ob[1] = (s16)port_be16(v + 2);
        d->ob[2] = (s16)port_be16(v + 4);
        d->flag = port_be16(v + 6);
        d->tc[0] = (s16)port_be16(v + 8);
        d->tc[1] = (s16)port_be16(v + 10);
        d->cn[0] = v[12];
        d->cn[1] = v[13];
        d->cn[2] = v[14];
        d->cn[3] = v[15];
    }
}

void port_fill_asset_stubs(void) {
    u32 i;

    for (i = 0; i < sizeof(sAssetFills) / sizeof(sAssetFills[0]); i++) {
        nuPiReadRom(sAssetFills[i].rom, sAssetFills[i].dest, sAssetFills[i].size);
    }
    port_fill_monstar_bubbles();
}
