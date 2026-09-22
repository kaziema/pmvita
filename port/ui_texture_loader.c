/**
 * ui_texture_loader.c - Load UI textures from ROM for PC port
 *
 * On N64, UI textures (window borders, backgrounds, file menu graphics,
 * HUD CI textures) are embedded in CODE/DATA segments via .incbin.
 * On PC, we load them from the ROM file at boot time using known ROM offsets.
 *
 * ROM offsets sourced from papermario-main/ver/us/splat.yaml and symbol_addrs.txt
 */
#include <stdio.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned int u32;
typedef int s32;

// From NuSystemShims (ROM reading)
extern void nuPiReadRom(u32 romAddr, void* dest, u32 size);

// Forward declarations
static void port_load_msg_textures(void);
static void port_load_global_hud_textures(void);
static void port_load_battle_ui_textures(void);
static void port_load_stencil_textures(void);
static void port_load_pause_textures(void);
static void port_load_theater_textures(void);
static void port_load_indicator_icon_textures(void);
static void port_load_level_up_textures(void);
static void port_load_starpoint_textures(void);
static void port_load_entity_model_data(void);
static void port_load_shadow_textures(void);
static void port_load_map_message_images(void);

/* ===== UI box textures (draw_box.c borders/backgrounds) ===== */
extern u8 ui_box_corners9_png[];
extern u8 ui_box_corners8_png[];
extern u8 ui_box_corners6_png[];
extern u8 ui_box_corners7_png[];
extern u8 ui_box_corners3_png[];
extern u8 ui_box_corners5_png[];
extern u8 ui_box_corners4_png[];
extern u8 ui_box_bg_tile_png[];
extern u8 ui_box_corners1_png[];
extern u8 ui_box_corners2_png[];
extern u8 ui_box_bg_flat_png[];

/* ===== Filemenu display list textures (filemenu_gfx.c) ===== */
extern u8 D_8024A200[];  /* copyarrow */
extern u8 D_8024A400[];  /* corners_yellow */
extern u8 D_8024B400[];  /* corners_gray */

/* ===== Menu HUD CI textures (menu_hud_scripts.c) ===== */
/* These are loaded by HUD_ELEMENT_OP_SetCI via direct pointer */
extern u8 ui_pause_unused_badge_points_png[];
extern u8 ui_pause_unused_badge_points_pal[];
extern u8 ui_pause_unused_badge_points_disabled_disabled__pal[];
extern u8 ui_pause_unused_compass_png[];
extern u8 ui_pause_unused_compass_pal[];
extern u8 ui_pause_unused_compass_disabled_disabled__pal[];
extern u8 ui_pause_cursor_hand_png[];
extern u8 ui_pause_cursor_hand_pal[];
extern u8 ui_files_filename_caret_png[];
extern u8 ui_files_filename_caret_pal[];
extern u8 ui_files_filename_space_png[];
extern u8 ui_files_filename_space_pal[];
extern u8 ui_pause_label_stats_png[];
extern u8 ui_pause_label_stats_pal[];
extern u8 ui_pause_label_badges_png[];
extern u8 ui_pause_label_badges_pal[];
extern u8 ui_pause_label_items_png[];
extern u8 ui_pause_label_items_pal[];
extern u8 ui_pause_label_party_png[];
extern u8 ui_pause_label_party_pal[];
extern u8 ui_pause_label_spirits_png[];
extern u8 ui_pause_label_spirits_pal[];
extern u8 ui_pause_label_map_png[];
extern u8 ui_pause_label_map_pal[];
extern u8 ui_pause_small_digit_0_png[];
extern u8 ui_pause_small_digit_0_pal[];
extern u8 ui_pause_small_digit_1_png[];
extern u8 ui_pause_small_digit_1_pal[];
extern u8 ui_pause_small_digit_2_png[];
extern u8 ui_pause_small_digit_2_pal[];
extern u8 ui_pause_small_digit_3_png[];
extern u8 ui_pause_small_digit_3_pal[];
extern u8 ui_pause_small_digit_4_png[];
extern u8 ui_pause_small_digit_4_pal[];
extern u8 ui_pause_small_digit_5_png[];
extern u8 ui_pause_small_digit_5_pal[];
extern u8 ui_pause_small_digit_6_png[];
extern u8 ui_pause_small_digit_6_pal[];
extern u8 ui_pause_small_digit_7_png[];
extern u8 ui_pause_small_digit_7_pal[];
extern u8 ui_pause_small_digit_8_png[];
extern u8 ui_pause_small_digit_8_pal[];
extern u8 ui_pause_small_digit_9_png[];
extern u8 ui_pause_small_digit_9_pal[];
extern u8 ui_pause_small_text_bp_png[];
extern u8 ui_pause_small_text_bp_pal[];
extern u8 ui_pause_label_jp_file_png[];
extern u8 ui_pause_label_jp_file_pal[];
extern u8 ui_pause_label_jp_file_disabled_png[];
extern u8 ui_pause_label_jp_file_disabled_pal[];
extern u8 ui_files_option_mono_on_png[];
extern u8 ui_files_option_mono_on_pal[];
extern u8 ui_files_option_mono_off_png[];
extern u8 ui_files_option_mono_off_pal[];
extern u8 ui_files_option_stereo_on_png[];
extern u8 ui_files_option_stereo_on_pal[];
extern u8 ui_files_option_stereo_off_png[];
extern u8 ui_files_option_stereo_off_pal[];
extern u8 ui_files_eldstar_png[];
extern u8 ui_files_eldstar_pal[];
extern u8 ui_files_eldstar_silhouette_png[];
extern u8 ui_files_eldstar_silhouette_pal[];
extern u8 ui_files_mamar_png[];
extern u8 ui_files_mamar_pal[];
extern u8 ui_files_mamar_silhouette_png[];
extern u8 ui_files_mamar_silhouette_pal[];
extern u8 ui_files_skolar_png[];
extern u8 ui_files_skolar_pal[];
extern u8 ui_files_skolar_silhouette_png[];
extern u8 ui_files_skolar_silhouette_pal[];
extern u8 ui_files_muskular_png[];
extern u8 ui_files_muskular_pal[];
extern u8 ui_files_muskular_silhouette_png[];
extern u8 ui_files_muskular_silhouette_pal[];
extern u8 ui_files_misstar_png[];
extern u8 ui_files_misstar_pal[];
extern u8 ui_files_misstar_silhouette_png[];
extern u8 ui_files_misstar_silhouette_pal[];
extern u8 ui_files_klevar_png[];
extern u8 ui_files_klevar_pal[];
extern u8 ui_files_klevar_silhouette_png[];
extern u8 ui_files_klevar_silhouette_pal[];
extern u8 ui_files_kalmar_png[];
extern u8 ui_files_kalmar_pal[];
extern u8 ui_files_kalmar_silhouette_png[];
extern u8 ui_files_kalmar_silhouette_pal[];
extern u8 ui_pause_map_walk0_png[];
extern u8 ui_pause_map_walk0_pal[];
extern u8 ui_pause_map_walk1_png[];
extern u8 ui_pause_map_walk1_pal[];
extern u8 ui_pause_map_walk2_png[];
extern u8 ui_pause_map_walk2_pal[];
extern u8 ui_pause_jp_super_png[];
extern u8 ui_pause_jp_super_pal[];
extern u8 ui_pause_jp_ultra_png[];
extern u8 ui_pause_jp_ultra_pal[];
extern u8 ui_pause_list_prev_page_png[];
extern u8 ui_pause_list_prev_page_pal[];
extern u8 ui_pause_list_next_page_png[];
extern u8 ui_pause_list_next_page_pal[];
extern u8 ui_pause_unused_z_up_arrow_png[];
extern u8 ui_pause_unused_z_up_arrow_pal[];
extern u8 ui_pause_unused_r_down_arrow_png[];
extern u8 ui_pause_unused_r_down_arrow_pal[];
extern u8 ui_pause_desc_msg_prev_png[];
extern u8 ui_pause_desc_msg_prev_pal[];
extern u8 ui_pause_desc_msg_next_png[];
extern u8 ui_pause_desc_msg_next_pal[];
extern u8 ui_pause_bp_needed_png[];
extern u8 ui_pause_bp_needed_pal[];
extern u8 ui_pause_mario_large_png[];
extern u8 ui_pause_mario_large_pal[];
extern u8 ui_pause_dash_png[];
extern u8 ui_pause_dash_pal[];
extern u8 ui_pause_stat_bp_png[];
extern u8 ui_pause_stat_bp_pal[];
extern u8 ui_pause_stat_mario_png[];
extern u8 ui_pause_stat_mario_pal[];
extern u8 ui_pause_stat_time_png[];
extern u8 ui_pause_stat_time_pal[];
extern u8 ui_pause_stat_star_piece0_png[];
extern u8 ui_pause_stat_star_piece0_pal[];
extern u8 ui_pause_stat_star_piece1_png[];
extern u8 ui_pause_stat_star_piece1_pal[];
extern u8 ui_pause_stat_star_piece2_png[];
extern u8 ui_pause_stat_star_piece2_pal[];
extern u8 ui_pause_stat_fp_png[];
extern u8 ui_pause_stat_fp_pal[];
extern u8 ui_pause_stat_times_png[];
extern u8 ui_pause_stat_times_pal[];
extern u8 ui_pause_stat_boots0_png[];
extern u8 ui_pause_stat_boots0_pal[];
extern u8 ui_pause_stat_boots1_png[];
extern u8 ui_pause_stat_boots1_pal[];
extern u8 ui_pause_stat_boots2_png[];
extern u8 ui_pause_stat_boots2_pal[];
extern u8 ui_pause_stat_boots3_png[];
extern u8 ui_pause_stat_boots3_pal[];
extern u8 ui_pause_stat_hammer0_png[];
extern u8 ui_pause_stat_hammer0_pal[];
extern u8 ui_pause_stat_hammer1_png[];
extern u8 ui_pause_stat_hammer1_pal[];
extern u8 ui_pause_stat_hammer2_png[];
extern u8 ui_pause_stat_hammer2_pal[];
extern u8 ui_pause_stat_hammer3_png[];
extern u8 ui_pause_stat_hammer3_pal[];
extern u8 ui_pause_partner_rank_png[];
extern u8 ui_pause_partner_rank_pal[];
extern u8 ui_pause_unused_bubble_png[];
extern u8 ui_pause_unused_bubble_pal[];

typedef struct {
    u32 romOffset;
    u8* dest;
    u32 size;
    const char* name;
} UITextureEntry;

/* ===== UI box + filemenu DL textures ===== */
static const UITextureEntry sUIBoxTextures[] = {
    { 0xA2E90, NULL, 1024, "ui_box_corners9" },
    { 0xA3290, NULL, 2304, "ui_box_corners8" },
    { 0xA3B90, NULL, 640,  "ui_box_corners6" },
    { 0xA3E10, NULL, 512,  "ui_box_corners7" },
    { 0xA4010, NULL, 1536, "ui_box_corners3" },
    { 0xA4610, NULL, 512,  "ui_box_corners5" },
    { 0xA4810, NULL, 256,  "ui_box_corners4" },
    { 0xA4910, NULL, 128,  "ui_box_bg_tile" },
    { 0xA4990, NULL, 1024, "ui_box_corners1" },
    { 0xA4D90, NULL, 512,  "ui_box_corners2" },
    { 0xA4F90, NULL, 8,    "ui_box_bg_flat" },
    { 0x16AA60, NULL, 512,  "filemenu_copyarrow" },
    { 0x16AC60, NULL, 4096, "filemenu_corners_yellow" },
    { 0x16BC60, NULL, 512,  "filemenu_corners_gray" },
};

void port_load_ui_textures(void) {
    UITextureEntry entries[14];
    memcpy(entries, sUIBoxTextures, sizeof(sUIBoxTextures));

    entries[0].dest = ui_box_corners9_png;
    entries[1].dest = ui_box_corners8_png;
    entries[2].dest = ui_box_corners6_png;
    entries[3].dest = ui_box_corners7_png;
    entries[4].dest = ui_box_corners3_png;
    entries[5].dest = ui_box_corners5_png;
    entries[6].dest = ui_box_corners4_png;
    entries[7].dest = ui_box_bg_tile_png;
    entries[8].dest = ui_box_corners1_png;
    entries[9].dest = ui_box_corners2_png;
    entries[10].dest = ui_box_bg_flat_png;
    entries[11].dest = D_8024A200;
    entries[12].dest = D_8024A400;
    entries[13].dest = D_8024B400;

    fprintf(stderr, "[PORT] Loading %d UI box textures from ROM...\n", 14);
    for (int i = 0; i < 14; i++) {
        nuPiReadRom(entries[i].romOffset, entries[i].dest, entries[i].size);
        /* Compute simple checksum to verify data loaded */
        u32 cksum = 0;
        for (u32 j = 0; j < entries[i].size; j++) cksum += ((u8*)entries[i].dest)[j];
        fprintf(stderr, "[PORT]   %s: ROM 0x%X, %u bytes, cksum=%u\n",
                entries[i].name, entries[i].romOffset, entries[i].size, cksum);
    }

    /* ===== Menu HUD CI textures (ui_images_filemenu_pause segment) =====
     *
     * The menu_hud_scripts.c textures are tightly packed in ROM starting at
     * 0x131340 (VRAM 0x8023E000). We load the entire segment in one read
     * and distribute to each stub array in source-file order.
     *
     * Data layout matches INCLUDE_IMG/INCLUDE_PAL order in menu_hud_scripts.c.
     * CI4 sizes: (width * height) / 2.  All palettes: 32 bytes (16 colors).
     */
    #define MENU_HUD_SEG_ROM  0x131340
    #define MENU_HUD_SEG_SIZE 0x3E00   /* total size of all US textures */
    static u8 segBuf[MENU_HUD_SEG_SIZE];
    nuPiReadRom(MENU_HUD_SEG_ROM, segBuf, MENU_HUD_SEG_SIZE);

    u8* pos = segBuf;
    #define COPY_TEX(dest, sz) do { memcpy(dest, pos, sz); pos += (sz); } while(0)

    /* Lines 6-8: unused_badge_points (24x24 CI4=288) + 2 palettes */
    COPY_TEX(ui_pause_unused_badge_points_png, 288);
    COPY_TEX(ui_pause_unused_badge_points_pal, 32);
    COPY_TEX(ui_pause_unused_badge_points_disabled_disabled__pal, 32);
    /* Lines 9-11: unused_compass (24x24 CI4=288) + 2 palettes */
    COPY_TEX(ui_pause_unused_compass_png, 288);
    COPY_TEX(ui_pause_unused_compass_pal, 32);
    COPY_TEX(ui_pause_unused_compass_disabled_disabled__pal, 32);
    /* Lines 12-13: cursor_hand (16x16 CI4=128) */
    COPY_TEX(ui_pause_cursor_hand_png, 128);
    COPY_TEX(ui_pause_cursor_hand_pal, 32);
    /* Lines 14-17: filename controls */
    COPY_TEX(ui_files_filename_caret_png, 128);
    COPY_TEX(ui_files_filename_caret_pal, 32);
    COPY_TEX(ui_files_filename_space_png, 32);
    COPY_TEX(ui_files_filename_space_pal, 32);
    /* Lines 20-27: pause labels (48x16 CI4=384) — #if !VERSION_JP */
    COPY_TEX(ui_pause_label_stats_png, 384);
    COPY_TEX(ui_pause_label_stats_pal, 32);
    COPY_TEX(ui_pause_label_badges_png, 384);
    COPY_TEX(ui_pause_label_badges_pal, 32);
    COPY_TEX(ui_pause_label_items_png, 384);
    COPY_TEX(ui_pause_label_items_pal, 32);
    COPY_TEX(ui_pause_label_party_png, 384);
    COPY_TEX(ui_pause_label_party_pal, 32);
    /* Lines 30-33: spirits + map labels (48x16) */
    COPY_TEX(ui_pause_label_spirits_png, 384);
    COPY_TEX(ui_pause_label_spirits_pal, 32);
    COPY_TEX(ui_pause_label_map_png, 384);
    COPY_TEX(ui_pause_label_map_pal, 32);
    /* VERSION_PAL labels skipped — not in US ROM */
    /* Lines 76-97: small digits 0-9 + text_bp (8x8 CI4=32) */
    COPY_TEX(ui_pause_small_digit_0_png, 32);
    COPY_TEX(ui_pause_small_digit_0_pal, 32);
    COPY_TEX(ui_pause_small_digit_1_png, 32);
    COPY_TEX(ui_pause_small_digit_1_pal, 32);
    COPY_TEX(ui_pause_small_digit_2_png, 32);
    COPY_TEX(ui_pause_small_digit_2_pal, 32);
    COPY_TEX(ui_pause_small_digit_3_png, 32);
    COPY_TEX(ui_pause_small_digit_3_pal, 32);
    COPY_TEX(ui_pause_small_digit_4_png, 32);
    COPY_TEX(ui_pause_small_digit_4_pal, 32);
    COPY_TEX(ui_pause_small_digit_5_png, 32);
    COPY_TEX(ui_pause_small_digit_5_pal, 32);
    COPY_TEX(ui_pause_small_digit_6_png, 32);
    COPY_TEX(ui_pause_small_digit_6_pal, 32);
    COPY_TEX(ui_pause_small_digit_7_png, 32);
    COPY_TEX(ui_pause_small_digit_7_pal, 32);
    COPY_TEX(ui_pause_small_digit_8_png, 32);
    COPY_TEX(ui_pause_small_digit_8_pal, 32);
    COPY_TEX(ui_pause_small_digit_9_png, 32);
    COPY_TEX(ui_pause_small_digit_9_pal, 32);
    COPY_TEX(ui_pause_small_text_bp_png, 32);
    COPY_TEX(ui_pause_small_text_bp_pal, 32);
    /* Lines 98-101: JP file labels (32x16 CI4=256) */
    COPY_TEX(ui_pause_label_jp_file_png, 256);
    COPY_TEX(ui_pause_label_jp_file_pal, 32);
    COPY_TEX(ui_pause_label_jp_file_disabled_png, 256);
    COPY_TEX(ui_pause_label_jp_file_disabled_pal, 32);
    /* Lines 104-111: file menu options (64x16 CI4=512) */
    COPY_TEX(ui_files_option_mono_on_png, 512);
    COPY_TEX(ui_files_option_mono_on_pal, 32);
    COPY_TEX(ui_files_option_mono_off_png, 512);
    COPY_TEX(ui_files_option_mono_off_pal, 32);
    COPY_TEX(ui_files_option_stereo_on_png, 512);
    COPY_TEX(ui_files_option_stereo_on_pal, 32);
    COPY_TEX(ui_files_option_stereo_off_png, 512);
    COPY_TEX(ui_files_option_stereo_off_pal, 32);
    /* VERSION_PAL options skipped */
    /* Lines 145-172: star spirit icons (16x16 CI4=128) */
    COPY_TEX(ui_files_eldstar_png, 128);
    COPY_TEX(ui_files_eldstar_pal, 32);
    COPY_TEX(ui_files_eldstar_silhouette_png, 128);
    COPY_TEX(ui_files_eldstar_silhouette_pal, 32);
    COPY_TEX(ui_files_mamar_png, 128);
    COPY_TEX(ui_files_mamar_pal, 32);
    COPY_TEX(ui_files_mamar_silhouette_png, 128);
    COPY_TEX(ui_files_mamar_silhouette_pal, 32);
    COPY_TEX(ui_files_skolar_png, 128);
    COPY_TEX(ui_files_skolar_pal, 32);
    COPY_TEX(ui_files_skolar_silhouette_png, 128);
    COPY_TEX(ui_files_skolar_silhouette_pal, 32);
    COPY_TEX(ui_files_muskular_png, 128);
    COPY_TEX(ui_files_muskular_pal, 32);
    COPY_TEX(ui_files_muskular_silhouette_png, 128);
    COPY_TEX(ui_files_muskular_silhouette_pal, 32);
    COPY_TEX(ui_files_misstar_png, 128);
    COPY_TEX(ui_files_misstar_pal, 32);
    COPY_TEX(ui_files_misstar_silhouette_png, 128);
    COPY_TEX(ui_files_misstar_silhouette_pal, 32);
    COPY_TEX(ui_files_klevar_png, 128);
    COPY_TEX(ui_files_klevar_pal, 32);
    COPY_TEX(ui_files_klevar_silhouette_png, 128);
    COPY_TEX(ui_files_klevar_silhouette_pal, 32);
    COPY_TEX(ui_files_kalmar_png, 128);
    COPY_TEX(ui_files_kalmar_pal, 32);
    COPY_TEX(ui_files_kalmar_silhouette_png, 128);
    COPY_TEX(ui_files_kalmar_silhouette_pal, 32);
    /* Lines 173-194: map walk, JP super/ultra, nav icons */
    COPY_TEX(ui_pause_map_walk0_png, 288);
    COPY_TEX(ui_pause_map_walk0_pal, 32);
    COPY_TEX(ui_pause_map_walk1_png, 288);
    COPY_TEX(ui_pause_map_walk1_pal, 32);
    COPY_TEX(ui_pause_map_walk2_png, 288);
    COPY_TEX(ui_pause_map_walk2_pal, 32);
    COPY_TEX(ui_pause_jp_super_png, 320);
    COPY_TEX(ui_pause_jp_super_pal, 32);
    COPY_TEX(ui_pause_jp_ultra_png, 320);
    COPY_TEX(ui_pause_jp_ultra_pal, 32);
    COPY_TEX(ui_pause_list_prev_page_png, 128);
    COPY_TEX(ui_pause_list_prev_page_pal, 32);
    COPY_TEX(ui_pause_list_next_page_png, 128);
    COPY_TEX(ui_pause_list_next_page_pal, 32);
    COPY_TEX(ui_pause_unused_z_up_arrow_png, 128);
    COPY_TEX(ui_pause_unused_z_up_arrow_pal, 32);
    COPY_TEX(ui_pause_unused_r_down_arrow_png, 128);
    COPY_TEX(ui_pause_unused_r_down_arrow_pal, 32);
    COPY_TEX(ui_pause_desc_msg_prev_png, 128);
    COPY_TEX(ui_pause_desc_msg_prev_pal, 32);
    COPY_TEX(ui_pause_desc_msg_next_png, 128);
    COPY_TEX(ui_pause_desc_msg_next_pal, 32);
    /* Lines 196-197: bp_needed (48x8 CI4=192) */
    COPY_TEX(ui_pause_bp_needed_png, 192);
    COPY_TEX(ui_pause_bp_needed_pal, 32);
    /* VERSION_PAL bp_needed skipped */
    /* Lines 210-249: stats and remaining pause textures */
    COPY_TEX(ui_pause_mario_large_png, 512);
    COPY_TEX(ui_pause_mario_large_pal, 32);
    COPY_TEX(ui_pause_dash_png, 64);
    COPY_TEX(ui_pause_dash_pal, 32);
    COPY_TEX(ui_pause_stat_bp_png, 128);
    COPY_TEX(ui_pause_stat_bp_pal, 32);
    COPY_TEX(ui_pause_stat_mario_png, 128);
    COPY_TEX(ui_pause_stat_mario_pal, 32);
    COPY_TEX(ui_pause_stat_time_png, 128);
    COPY_TEX(ui_pause_stat_time_pal, 32);
    COPY_TEX(ui_pause_stat_star_piece0_png, 128);
    COPY_TEX(ui_pause_stat_star_piece0_pal, 32);
    COPY_TEX(ui_pause_stat_star_piece1_png, 128);
    COPY_TEX(ui_pause_stat_star_piece1_pal, 32);
    COPY_TEX(ui_pause_stat_star_piece2_png, 128);
    COPY_TEX(ui_pause_stat_star_piece2_pal, 32);
    COPY_TEX(ui_pause_stat_fp_png, 128);
    COPY_TEX(ui_pause_stat_fp_pal, 32);
    COPY_TEX(ui_pause_stat_times_png, 32);
    COPY_TEX(ui_pause_stat_times_pal, 32);
    COPY_TEX(ui_pause_stat_boots0_png, 128);
    COPY_TEX(ui_pause_stat_boots0_pal, 32);
    COPY_TEX(ui_pause_stat_boots1_png, 128);
    COPY_TEX(ui_pause_stat_boots1_pal, 32);
    COPY_TEX(ui_pause_stat_boots2_png, 128);
    COPY_TEX(ui_pause_stat_boots2_pal, 32);
    COPY_TEX(ui_pause_stat_boots3_png, 128);
    COPY_TEX(ui_pause_stat_boots3_pal, 32);
    COPY_TEX(ui_pause_stat_hammer0_png, 128);
    COPY_TEX(ui_pause_stat_hammer0_pal, 32);
    COPY_TEX(ui_pause_stat_hammer1_png, 128);
    COPY_TEX(ui_pause_stat_hammer1_pal, 32);
    COPY_TEX(ui_pause_stat_hammer2_png, 128);
    COPY_TEX(ui_pause_stat_hammer2_pal, 32);
    COPY_TEX(ui_pause_stat_hammer3_png, 128);
    COPY_TEX(ui_pause_stat_hammer3_pal, 32);
    COPY_TEX(ui_pause_partner_rank_png, 128);
    COPY_TEX(ui_pause_partner_rank_pal, 32);
    COPY_TEX(ui_pause_unused_bubble_png, 448);
    COPY_TEX(ui_pause_unused_bubble_pal, 32);

    #undef COPY_TEX

    int bytesUsed = (int)(pos - segBuf);
    fprintf(stderr, "[PORT] Menu HUD textures loaded: %d bytes from segment at ROM 0x%X\n",
            bytesUsed, MENU_HUD_SEG_ROM);

    /* ===== Message box / speech bubble / sign textures (msg_img segment) =====
     *
     * ROM layout at 0x10CEB0 matches splat.yaml msg_img .data subsegments:
     * bubble_left(1024), bubble_mid(256), bubble_right(1024), arrow(128),
     * frame_a_* (16x32=512), frame_b_* (16x32=512),
     * palettes(512) — already hardcoded in msg_img.c as ui_msg_palettes[16][32],
     * sign_corner_tl/tr/bl/br(128 each), lamppost_corner_br(128),
     * sign_side_top(256), sign_side_left(320), sign_side_right(320),
     * sign_side_bottom(256), sign_fill(32),
     * sign_pal(32), lamppost_pal(32),
     * background(2048), rewind_arrow(288), rewind_arrow_pal(32),
     * star(576), star_silhouette(160)
     * Total: 8960 bytes = 0x2300
     */
    port_load_msg_textures();

    /* ===== World status HUD + battle status + input icons (global_hud_scripts segment) ===== */
    port_load_global_hud_textures();

    /* ===== Battle UI textures (menus, HP, commands, aim, projector) ===== */
    port_load_battle_ui_textures();

    /* ===== Screen overlay stencils (star wipe, circle wipe, Mario silhouette) ===== */
    port_load_stencil_textures();

    /* ===== Pause menu textures (world map, banners, stats) ===== */
    port_load_pause_textures();

    /* ===== Theater/curtains + no controller ===== */
    port_load_theater_textures();

    /* ===== Entity drop shadows ===== */
    port_load_shadow_textures();

    /* ===== Map message images ===== */
    port_load_map_message_images();

    /* ===== Indicator icons (speech bubble, inspect, i-spy) ===== */
    port_load_indicator_icon_textures();

    /* ===== Level up screen textures ===== */
    port_load_level_up_textures();

    /* ===== Star point display textures ===== */
    port_load_starpoint_textures();

    /* ===== Entity model textures + vertices ===== */
    port_load_entity_model_data();

    fprintf(stderr, "[PORT] UI textures loaded successfully\n");
}

/* ===== msg_img externs ===== */
extern u8 ui_msg_bubble_left_png[];
extern u8 ui_msg_bubble_mid_png[];
extern u8 ui_msg_bubble_right_png[];
extern u8 ui_msg_arrow_png[];
extern u8 ui_msg_frame_a_1_1_png[];
extern u8 ui_msg_frame_a_1_2_png[];
extern u8 ui_msg_frame_a_1_3_png[];
extern u8 ui_msg_frame_a_1_4_png[];
extern u8 ui_msg_frame_a_1_5_png[];
extern u8 ui_msg_frame_a_2_1_png[];
extern u8 ui_msg_frame_a_2_5_png[];
extern u8 ui_msg_frame_a_3_1_png[];
extern u8 ui_msg_frame_a_3_5_png[];
extern u8 ui_msg_frame_a_4_1_png[];
extern u8 ui_msg_frame_a_4_5_png[];
extern u8 ui_msg_frame_a_5_1_png[];
extern u8 ui_msg_frame_a_5_2_png[];
extern u8 ui_msg_frame_a_5_3_png[];
extern u8 ui_msg_frame_a_5_4_png[];
extern u8 ui_msg_frame_a_5_5_png[];
extern u8 ui_msg_frame_b_1_1_png[];
extern u8 ui_msg_frame_b_1_2_png[];
extern u8 ui_msg_frame_b_1_3_png[];
extern u8 ui_msg_frame_b_1_4_png[];
extern u8 ui_msg_frame_b_1_5_png[];
extern u8 ui_msg_frame_b_2_1_png[];
extern u8 ui_msg_frame_b_2_5_png[];
extern u8 ui_msg_frame_b_3_1_png[];
extern u8 ui_msg_frame_b_3_5_png[];
extern u8 ui_msg_frame_b_4_1_png[];
extern u8 ui_msg_frame_b_4_5_png[];
extern u8 ui_msg_frame_b_5_1_png[];
extern u8 ui_msg_frame_b_5_2_png[];
extern u8 ui_msg_frame_b_5_3_png[];
extern u8 ui_msg_frame_b_5_4_png[];
extern u8 ui_msg_frame_b_5_5_png[];
extern u8 ui_msg_sign_corner_topleft_png[];
extern u8 ui_msg_sign_corner_topright_png[];
extern u8 ui_msg_sign_corner_bottomleft_png[];
extern u8 ui_msg_sign_corner_bottomright_png[];
extern u8 ui_msg_lamppost_corner_bottomright_png[];
extern u8 ui_msg_sign_side_top_png[];
extern u8 ui_msg_sign_side_left_png[];
extern u8 ui_msg_sign_side_right_png[];
extern u8 ui_msg_sign_side_bottom_png[];
extern u8 ui_msg_sign_fill_png[];
extern u8 ui_msg_sign_pal[];
extern u8 ui_msg_lamppost_pal[];
extern u8 ui_msg_background_png[];
extern u8 ui_msg_rewind_arrow_png[];
extern u8 ui_msg_rewind_arrow_pal[];
extern u8 ui_msg_star_png[];
extern u8 ui_msg_star_silhouette_png[];

static void port_load_msg_textures(void) {
    #define MSG_IMG_SEG_ROM  0x10CEB0
    #define MSG_IMG_SEG_SIZE 0x2300   /* 8960 bytes total */
    static u8 msgBuf[MSG_IMG_SEG_SIZE];
    nuPiReadRom(MSG_IMG_SEG_ROM, msgBuf, MSG_IMG_SEG_SIZE);

    u8* p = msgBuf;
    #define MCOPY(dest, sz) do { memcpy(dest, p, sz); p += (sz); } while(0)

    /* Bubble textures */
    MCOPY(ui_msg_bubble_left_png, 1024);    /* 32x64 CI4 */
    MCOPY(ui_msg_bubble_mid_png, 256);      /* 8x64 CI4 */
    MCOPY(ui_msg_bubble_right_png, 1024);   /* 32x64 CI4 */
    MCOPY(ui_msg_arrow_png, 128);           /* 16x16 CI4 */

    /* Frame A textures (8x8 CI4 = 32 bytes each) */
    MCOPY(ui_msg_frame_a_1_1_png, 32);
    MCOPY(ui_msg_frame_a_1_2_png, 32);
    MCOPY(ui_msg_frame_a_1_3_png, 32);
    MCOPY(ui_msg_frame_a_1_4_png, 32);
    MCOPY(ui_msg_frame_a_1_5_png, 32);
    MCOPY(ui_msg_frame_a_2_1_png, 32);
    MCOPY(ui_msg_frame_a_2_5_png, 32);
    MCOPY(ui_msg_frame_a_3_1_png, 32);
    MCOPY(ui_msg_frame_a_3_5_png, 32);
    MCOPY(ui_msg_frame_a_4_1_png, 32);
    MCOPY(ui_msg_frame_a_4_5_png, 32);
    MCOPY(ui_msg_frame_a_5_1_png, 32);
    MCOPY(ui_msg_frame_a_5_2_png, 32);
    MCOPY(ui_msg_frame_a_5_3_png, 32);
    MCOPY(ui_msg_frame_a_5_4_png, 32);
    MCOPY(ui_msg_frame_a_5_5_png, 32);

    /* Frame B textures (8x8 CI4 = 32 bytes each) */
    MCOPY(ui_msg_frame_b_1_1_png, 32);
    MCOPY(ui_msg_frame_b_1_2_png, 32);
    MCOPY(ui_msg_frame_b_1_3_png, 32);
    MCOPY(ui_msg_frame_b_1_4_png, 32);
    MCOPY(ui_msg_frame_b_1_5_png, 32);
    MCOPY(ui_msg_frame_b_2_1_png, 32);
    MCOPY(ui_msg_frame_b_2_5_png, 32);
    MCOPY(ui_msg_frame_b_3_1_png, 32);
    MCOPY(ui_msg_frame_b_3_5_png, 32);
    MCOPY(ui_msg_frame_b_4_1_png, 32);
    MCOPY(ui_msg_frame_b_4_5_png, 32);
    MCOPY(ui_msg_frame_b_5_1_png, 32);
    MCOPY(ui_msg_frame_b_5_2_png, 32);
    MCOPY(ui_msg_frame_b_5_3_png, 32);
    MCOPY(ui_msg_frame_b_5_4_png, 32);
    MCOPY(ui_msg_frame_b_5_5_png, 32);

    /* Skip 512 bytes of palette data — already hardcoded in msg_img.c as ui_msg_palettes[16][32] */
    p += 512;

    /* Sign textures */
    MCOPY(ui_msg_sign_corner_topleft_png, 128);     /* 16x16 CI4 */
    MCOPY(ui_msg_sign_corner_topright_png, 128);
    MCOPY(ui_msg_sign_corner_bottomleft_png, 128);
    MCOPY(ui_msg_sign_corner_bottomright_png, 128);
    MCOPY(ui_msg_lamppost_corner_bottomright_png, 128);
    MCOPY(ui_msg_sign_side_top_png, 256);           /* 32x16 CI4 */
    MCOPY(ui_msg_sign_side_left_png, 320);          /* 16x40 CI4 */
    MCOPY(ui_msg_sign_side_right_png, 320);
    MCOPY(ui_msg_sign_side_bottom_png, 256);
    MCOPY(ui_msg_sign_fill_png, 32);                /* 8x8 CI4 */

    /* Sign and lamppost palettes */
    MCOPY(ui_msg_sign_pal, 32);
    MCOPY(ui_msg_lamppost_pal, 32);

    /* Misc msg textures */
    MCOPY(ui_msg_background_png, 2048);             /* 64x64 I4 */
    MCOPY(ui_msg_rewind_arrow_png, 288);            /* 24x24 CI4 */
    MCOPY(ui_msg_rewind_arrow_pal, 32);
    MCOPY(ui_msg_star_png, 576);                    /* 16x18 RGBA16 */
    MCOPY(ui_msg_star_silhouette_png, 160);         /* 16x20 I4 */

    #undef MCOPY

    int msgUsed = (int)(p - msgBuf);
    /* Quick checksum to verify non-zero data */
    u32 bck = 0, fck = 0;
    for (int i = 0; i < 1024; i++) bck += ui_msg_bubble_left_png[i];
    for (int i = 0; i < 32; i++) fck += ui_msg_frame_a_1_1_png[i];
    fprintf(stderr, "[PORT] msg_img textures loaded: %d bytes from ROM 0x%X (expected %d)\n",
            msgUsed, MSG_IMG_SEG_ROM, MSG_IMG_SEG_SIZE);
    fprintf(stderr, "[PORT]   bubble_left ck=%u, frame_a_1_1 ck=%u (nonzero=good)\n", bck, fck);
}
/* ===== global_hud_scripts externs ===== */
extern u8 ui_input_a_button_unpressed_png[];
extern u8 ui_input_a_button_unpressed_pal[];
extern u8 ui_input_a_button_pressed_png[];
extern u8 ui_input_a_button_pressed_pal[];
extern u8 ui_input_analog_stick_png[];
extern u8 ui_input_analog_stick_pal[];
extern u8 ui_input_analog_stick_left_png[];
extern u8 ui_input_analog_stick_left_pal[];
extern u8 ui_input_analog_stick2_png[];
extern u8 ui_input_analog_stick2_pal[];
extern u8 ui_input_analog_stick_right_png[];
extern u8 ui_input_analog_stick_right_pal[];
extern u8 ui_input_analog_stick3_png[];
extern u8 ui_input_analog_stick3_pal[];
extern u8 ui_input_analog_stick_down_png[];
extern u8 ui_input_analog_stick_down_pal[];
extern u8 ui_input_analog_stick_up_png[];
extern u8 ui_input_analog_stick_up_pal[];
extern u8 ui_input_mash_bar_png[];
extern u8 ui_input_mash_bar_pal[];
extern u8 ui_ok_png[];
extern u8 ui_ok_pal[];
extern u8 ui_input_start_button_png[];
extern u8 ui_input_start_button_pal[];
extern u8 ui_input_start_button2_png[];
extern u8 ui_input_start_button2_pal[];
extern u8 ui_input_start_text_png[];
extern u8 ui_input_start_text_pal[];
extern u8 ui_hammer_png[];
extern u8 ui_hammer_pal[];
extern u8 ui_hammer_disabled_pal[];
extern u8 ui_super_hammer_png[];
extern u8 ui_super_hammer_pal[];
extern u8 ui_super_hammer_disabled_pal[];
extern u8 ui_ultra_hammer_png[];
extern u8 ui_ultra_hammer_pal[];
extern u8 ui_ultra_hammer_disabled_pal[];
extern u8 ui_boots_png[];
extern u8 ui_boots_pal[];
extern u8 ui_boots_disabled_pal[];
extern u8 ui_super_boots_png[];
extern u8 ui_super_boots_pal[];
extern u8 ui_super_boots_disabled_pal[];
extern u8 ui_ultra_boots_png[];
extern u8 ui_ultra_boots_pal[];
extern u8 ui_ultra_boots_disabled_pal[];
extern u8 ui_item_png[];
extern u8 ui_item_pal[];
extern u8 ui_item_disabled_pal[];
extern u8 ui_star_spirit_png[];
extern u8 ui_star_spirit_pal[];
extern u8 ui_star_spirit_disabled_pal[];
extern u8 ui_arrow_left_png[];
extern u8 ui_arrow_left_pal[];
extern u8 ui_arrow_right_png[];
extern u8 ui_arrow_right_pal[];
extern u8 ui_point_right_png[];
extern u8 ui_point_right_pal[];
extern u8 ui_pip_png[];
extern u8 ui_pip_pal[];
extern u8 ui_pip2_png[];
extern u8 ui_pip2_pal[];
extern u8 ui_coin_sparkle_0_png[];
extern u8 ui_coin_sparkle_0_pal[];
extern u8 ui_coin_sparkle_1_png[];
extern u8 ui_coin_sparkle_1_pal[];
extern u8 ui_coin_sparkle_2_png[];
extern u8 ui_coin_sparkle_2_pal[];
extern u8 ui_coin_sparkle_3_png[];
extern u8 ui_coin_sparkle_3_pal[];
extern u8 ui_coin_sparkle_4_png[];
extern u8 ui_coin_sparkle_4_pal[];
extern u8 ui_coin_sparkle_5_png[];
extern u8 ui_coin_sparkle_5_pal[];
extern u8 ui_coin_sparkle_none_png[];
extern u8 ui_coin_sparkle_none_pal[];
extern u8 ui_status_star_point_shine_png[];
extern u8 ui_status_star_piece_0_png[];
extern u8 ui_status_star_piece_0_pal[];
extern u8 ui_status_star_piece_1_png[];
extern u8 ui_status_star_piece_1_pal[];
extern u8 ui_status_star_piece_2_png[];
extern u8 ui_status_star_piece_2_pal[];
extern u8 ui_menu_times_png[];
extern u8 ui_menu_times_pal[];
extern u8 ui_fp_cost_png[];
extern u8 ui_fp_cost_pal[];
extern u8 ui_fp_cost_reduced_pal[];
extern u8 ui_fp_cost_reduced_twice_pal[];
extern u8 ui_fp_cost_notenough_pal[];
extern u8 ui_pow_cost_png[];
extern u8 ui_pow_cost_pal[];
extern u8 ui_pow_cost_reduced_pal[];
extern u8 ui_pow_cost_reduced_twice_pal[];
extern u8 ui_pow_cost_notenough_pal[];
extern u8 ui_green_arrow_down_png[];
extern u8 ui_green_arrow_down_pal[];
extern u8 ui_green_arrow_up_png[];
extern u8 ui_green_arrow_up_pal[];
extern u8 ui_kaime_png[];
extern u8 ui_kaime_pal[];
extern u8 ui_unused_1_png[];
extern u8 ui_unused_1_pal[];
extern u8 ui_unused_2_png[];
extern u8 ui_unused_2_pal[];
extern u8 ui_unused_3_png[];
extern u8 ui_unused_3_pal[];
extern u8 ui_red_bar1_png[];
extern u8 ui_red_bar1_pal[];
extern u8 ui_empty_bar_png[];
extern u8 ui_empty_bar_pal[];
extern u8 ui_red_bar2_png[];
extern u8 ui_red_bar2_pal[];
extern u8 ui_eldstar_png[];
extern u8 ui_eldstar_pal[];
extern u8 ui_eldstar_disabled_pal[];
extern u8 ui_mamar_png[];
extern u8 ui_mamar_pal[];
extern u8 ui_mamar_disabled_pal[];
extern u8 ui_skolar_png[];
extern u8 ui_skolar_pal[];
extern u8 ui_skolar_disabled_pal[];
extern u8 ui_muskular_png[];
extern u8 ui_muskular_pal[];
extern u8 ui_muskular_disabled_pal[];
extern u8 ui_misstar_png[];
extern u8 ui_misstar_pal[];
extern u8 ui_misstar_disabled_pal[];
extern u8 ui_klevar_png[];
extern u8 ui_klevar_pal[];
extern u8 ui_klevar_disabled_pal[];
extern u8 ui_kalmar_png[];
extern u8 ui_kalmar_pal[];
extern u8 ui_kalmar_disabled_pal[];
extern u8 ui_star_beam_png[];
extern u8 ui_star_beam_pal[];
extern u8 ui_star_beam_disabled_pal[];
extern u8 ui_peach_beam_png[];
extern u8 ui_peach_beam_pal[];
extern u8 ui_peach_beam_disabled_pal[];
extern u8 ui_mario_head_png[];
extern u8 ui_mario_head_pal[];
extern u8 ui_partner0_png[];
extern u8 ui_partner0_pal[];
extern u8 ui_partner0_disabled_pal[];
extern u8 ui_goombario_png[];
extern u8 ui_goombario_pal[];
extern u8 ui_goombario_disabled_pal[];
extern u8 ui_kooper_png[];
extern u8 ui_kooper_pal[];
extern u8 ui_kooper_disabled_pal[];
extern u8 ui_bombette_png[];
extern u8 ui_bombette_pal[];
extern u8 ui_bombette_disabled_pal[];
extern u8 ui_parakarry_png[];
extern u8 ui_parakarry_pal[];
extern u8 ui_parakarry_disabled_pal[];
extern u8 ui_bow_png[];
extern u8 ui_bow_pal[];
extern u8 ui_bow_disabled_pal[];
extern u8 ui_watt_png[];
extern u8 ui_watt_pal[];
extern u8 ui_watt_disabled_pal[];
extern u8 ui_sushie_png[];
extern u8 ui_sushie_pal[];
extern u8 ui_sushie_disabled_pal[];
extern u8 ui_lakilester_png[];
extern u8 ui_lakilester_pal[];
extern u8 ui_lakilester_disabled_pal[];
extern u8 ui_partner9_png[];
extern u8 ui_partner9_pal[];
extern u8 ui_partner9_disabled_pal[];
extern u8 ui_partner10_png[];
extern u8 ui_partner10_pal[];
extern u8 ui_partner10_disabled_pal[];
extern u8 ui_partner11_png[];
extern u8 ui_partner11_pal[];
extern u8 ui_partner11_disabled_pal[];
extern u8 ui_status_text_times_png[];
extern u8 ui_status_text_times_pal[];
extern u8 ui_status_text_slash_png[];
extern u8 ui_status_text_slash_pal[];
extern u8 ui_status_text_0_png[];
extern u8 ui_status_text_0_pal[];
extern u8 ui_status_text_1_png[];
extern u8 ui_status_text_1_pal[];
extern u8 ui_status_text_2_png[];
extern u8 ui_status_text_2_pal[];
extern u8 ui_status_text_3_png[];
extern u8 ui_status_text_3_pal[];
extern u8 ui_status_text_4_png[];
extern u8 ui_status_text_4_pal[];
extern u8 ui_status_text_5_png[];
extern u8 ui_status_text_5_pal[];
extern u8 ui_status_text_6_png[];
extern u8 ui_status_text_6_pal[];
extern u8 ui_status_text_7_png[];
extern u8 ui_status_text_7_pal[];
extern u8 ui_status_text_8_png[];
extern u8 ui_status_text_8_pal[];
extern u8 ui_status_text_9_png[];
extern u8 ui_status_text_9_pal[];
extern u8 ui_status_text_hp_png[];
extern u8 ui_status_text_hp_pal[];
extern u8 ui_status_text_fp_png[];
extern u8 ui_status_text_fp_pal[];
extern u8 ui_status_pow_unit_1_png[];
extern u8 ui_status_pow_unit_1_pal[];
extern u8 ui_status_pow_unit_2_png[];
extern u8 ui_status_pow_unit_2_pal[];
extern u8 ui_status_pow_unit_3_png[];
extern u8 ui_status_pow_unit_3_pal[];
extern u8 ui_status_pow_unit_4_png[];
extern u8 ui_status_pow_unit_4_pal[];
extern u8 ui_status_pow_unit_5_png[];
extern u8 ui_status_pow_unit_5_pal[];
extern u8 ui_status_pow_unit_6_png[];
extern u8 ui_status_pow_unit_6_pal[];
extern u8 ui_status_pow_unit_7_png[];
extern u8 ui_status_pow_unit_7_pal[];
extern u8 ui_status_pow_unit_empty_png[];
extern u8 ui_status_pow_unit_empty_pal[];
extern u8 ui_status_pow_star_1_png[];
extern u8 ui_status_pow_star_1_pal[];
extern u8 ui_status_pow_star_2_png[];
extern u8 ui_status_pow_star_2_pal[];
extern u8 ui_status_pow_star_3_png[];
extern u8 ui_status_pow_star_3_pal[];
extern u8 ui_status_pow_star_4_png[];
extern u8 ui_status_pow_star_4_pal[];
extern u8 ui_status_pow_star_5_png[];
extern u8 ui_status_pow_star_5_pal[];
extern u8 ui_status_pow_star_6_png[];
extern u8 ui_status_pow_star_6_pal[];
extern u8 ui_status_pow_star_7_png[];
extern u8 ui_status_pow_star_7_pal[];
extern u8 ui_status_pow_star_empty_png[];
extern u8 ui_status_pow_star_empty_pal[];
extern u8 ui_status_coin_0_png[];
extern u8 ui_status_coin_0_pal[];
extern u8 ui_status_coin_1_png[];
extern u8 ui_status_coin_1_pal[];
extern u8 ui_status_coin_2_png[];
extern u8 ui_status_coin_2_pal[];
extern u8 ui_status_coin_3_png[];
extern u8 ui_status_coin_3_pal[];
extern u8 ui_status_coin_4_png[];
extern u8 ui_status_coin_4_pal[];
extern u8 ui_status_coin_5_png[];
extern u8 ui_status_coin_5_pal[];
extern u8 ui_status_coin_6_png[];
extern u8 ui_status_coin_6_pal[];
extern u8 ui_status_coin_7_png[];
extern u8 ui_status_coin_7_pal[];
extern u8 ui_status_coin_8_png[];
extern u8 ui_status_coin_8_pal[];
extern u8 ui_status_coin_9_png[];
extern u8 ui_status_coin_9_pal[];
extern u8 ui_status_star_point_0_png[];
extern u8 ui_status_star_point_0_pal[];
extern u8 ui_status_star_point_1_png[];
extern u8 ui_status_star_point_1_pal[];
extern u8 ui_status_star_point_2_png[];
extern u8 ui_status_star_point_2_pal[];
extern u8 ui_status_star_point_3_png[];
extern u8 ui_status_star_point_3_pal[];
extern u8 ui_status_star_point_4_png[];
extern u8 ui_status_star_point_4_pal[];
extern u8 ui_status_star_point_5_png[];
extern u8 ui_status_star_point_5_pal[];
extern u8 ui_status_star_point_6_png[];
extern u8 ui_status_star_point_6_pal[];
extern u8 ui_status_star_point_7_png[];
extern u8 ui_status_star_point_7_pal[];
extern u8 ui_bluepip_png[];
extern u8 ui_bluepip_pal[];
extern u8 ui_bluepip2_png[];
extern u8 ui_bluepip2_pal[];
extern u8 ui_battle_status_charge_jump_png[];
extern u8 ui_battle_status_charge_jump_pal[];
extern u8 ui_battle_status_charge_hammer_png[];
extern u8 ui_battle_status_charge_hammer_pal[];
extern u8 ui_battle_status_charge_goombario_png[];
extern u8 ui_battle_status_charge_goombario_pal[];
extern u8 ui_battle_status_exclamation_png[];
extern u8 ui_battle_status_exclamation_pal[];
extern u8 ui_battle_status_sleep_0_png[];
extern u8 ui_battle_status_sleep_0_pal[];
extern u8 ui_battle_status_sleep_1_png[];
extern u8 ui_battle_status_sleep_1_pal[];
extern u8 ui_battle_status_sleep_2_png[];
extern u8 ui_battle_status_sleep_2_pal[];
extern u8 ui_battle_status_static_0_png[];
extern u8 ui_battle_status_static_0_pal[];
extern u8 ui_battle_status_static_1_png[];
extern u8 ui_battle_status_static_1_pal[];
extern u8 ui_battle_status_paralyze_0_png[];
extern u8 ui_battle_status_paralyze_0_pal[];
extern u8 ui_battle_status_paralyze_1_png[];
extern u8 ui_battle_status_paralyze_1_pal[];
extern u8 ui_battle_status_dizzy_0_png[];
extern u8 ui_battle_status_dizzy_0_pal[];
extern u8 ui_battle_status_dizzy_1_png[];
extern u8 ui_battle_status_dizzy_1_pal[];
extern u8 ui_battle_status_dizzy_2_png[];
extern u8 ui_battle_status_dizzy_2_pal[];
extern u8 ui_battle_status_dizzy_3_png[];
extern u8 ui_battle_status_dizzy_3_pal[];
extern u8 ui_battle_status_poison_0_png[];
extern u8 ui_battle_status_poison_0_pal[];
extern u8 ui_battle_status_poison_1_png[];
extern u8 ui_battle_status_poison_1_pal[];
extern u8 ui_battle_status_frozen_0_png[];
extern u8 ui_battle_status_frozen_0_pal[];
extern u8 ui_battle_status_frozen_1_png[];
extern u8 ui_battle_status_frozen_1_pal[];
extern u8 ui_battle_status_frozen_2_png[];
extern u8 ui_battle_status_frozen_2_pal[];
extern u8 ui_battle_status_frozen_3_png[];
extern u8 ui_battle_status_frozen_3_pal[];
extern u8 ui_battle_status_stop_0_png[];
extern u8 ui_battle_status_stop_0_pal[];
extern u8 ui_battle_status_stop_1_png[];
extern u8 ui_battle_status_stop_1_pal[];
extern u8 ui_battle_status_stop_2_png[];
extern u8 ui_battle_status_stop_2_pal[];
extern u8 ui_battle_status_stop_3_png[];
extern u8 ui_battle_status_stop_3_pal[];
extern u8 ui_battle_status_pdown_0_png[];
extern u8 ui_battle_status_pdown_0_pal[];
extern u8 ui_battle_status_pdown_1_png[];
extern u8 ui_battle_status_pdown_1_pal[];
extern u8 ui_battle_status_shrink_0_png[];
extern u8 ui_battle_status_shrink_0_pal[];
extern u8 ui_battle_status_shrink_1_png[];
extern u8 ui_battle_status_shrink_1_pal[];
extern u8 ui_battle_status_shrink_2_png[];
extern u8 ui_battle_status_shrink_2_pal[];
extern u8 ui_battle_status_shrink_3_png[];
extern u8 ui_battle_status_shrink_3_pal[];
extern u8 ui_battle_status_transparent_0_png[];
extern u8 ui_battle_status_transparent_0_pal[];
extern u8 ui_battle_status_transparent_1_png[];
extern u8 ui_battle_status_transparent_1_pal[];
extern u8 ui_battle_status_transparent_2_png[];
extern u8 ui_battle_status_transparent_2_pal[];
extern u8 ui_battle_status_transparent_3_png[];
extern u8 ui_battle_status_transparent_3_pal[];
extern u8 ui_battle_status_transparent_4_png[];
extern u8 ui_battle_status_transparent_4_pal[];
extern u8 ui_battle_status_transparent_5_png[];
extern u8 ui_battle_status_transparent_5_pal[];
extern u8 ui_battle_status_peril_png[];
extern u8 ui_battle_status_peril_pal[];
extern u8 ui_battle_status_danger_png[];
extern u8 ui_battle_status_danger_pal[];
extern u8 ui_battle_status_refund_png[];
extern u8 ui_battle_status_refund_pal[];
extern u8 ui_battle_status_happy_png[];
extern u8 ui_battle_status_happy_pal[];
extern u8 ui_battle_status_hp_drain_png[];
extern u8 ui_battle_status_hp_drain_pal[];
extern u8 ui_move_basic_png[];
extern u8 ui_move_basic_pal[];
extern u8 ui_move_basic_disabled_pal[];
extern u8 ui_move_partner_1_png[];
extern u8 ui_move_partner_1_pal[];
extern u8 ui_move_partner_1_disabled_pal[];
extern u8 ui_move_partner_2_png[];
extern u8 ui_move_partner_2_pal[];
extern u8 ui_move_partner_2_disabled_pal[];
extern u8 ui_move_partner_3_png[];
extern u8 ui_move_partner_3_pal[];
extern u8 ui_move_partner_3_disabled_pal[];
extern u8 ui_stat_heart_png[];
extern u8 ui_stat_flower_png[];

static void port_load_global_hud_textures(void) {
    #define GHS_SEG_ROM  0x919B0
    #define GHS_SEG_SIZE 0xC4B0  /* 50352 bytes */
    static u8 ghsBuf[GHS_SEG_SIZE];
    nuPiReadRom(GHS_SEG_ROM, ghsBuf, GHS_SEG_SIZE);

    u8* p = ghsBuf;
    #define MCOPY(dest, sz) do { memcpy(dest, p, sz); p += (sz); } while(0)

    MCOPY(ui_input_a_button_unpressed_png, 960);  /* 48x40 ci4 */
    MCOPY(ui_input_a_button_unpressed_pal, 32);  /* palette */
    MCOPY(ui_input_a_button_pressed_png, 960);  /* 48x40 ci4 */
    MCOPY(ui_input_a_button_pressed_pal, 32);  /* palette */
    MCOPY(ui_input_analog_stick_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_input_analog_stick_pal, 32);  /* palette */
    MCOPY(ui_input_analog_stick_left_png, 640);  /* 40x32 ci4 */
    MCOPY(ui_input_analog_stick_left_pal, 32);  /* palette */
    MCOPY(ui_input_analog_stick2_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_input_analog_stick2_pal, 32);  /* palette */
    MCOPY(ui_input_analog_stick_right_png, 640);  /* 40x32 ci4 */
    MCOPY(ui_input_analog_stick_right_pal, 32);  /* palette */
    MCOPY(ui_input_analog_stick3_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_input_analog_stick3_pal, 32);  /* palette */
    MCOPY(ui_input_analog_stick_down_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_input_analog_stick_down_pal, 32);  /* palette */
    MCOPY(ui_input_analog_stick_up_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_input_analog_stick_up_pal, 32);  /* palette */
    MCOPY(ui_input_mash_bar_png, 576);  /* 72x16 ci4 */
    MCOPY(ui_input_mash_bar_pal, 32);  /* palette */
    MCOPY(ui_ok_png, 192);  /* 24x16 ci4 */
    MCOPY(ui_ok_pal, 32);  /* palette */
    MCOPY(ui_input_start_button_png, 960);  /* 48x40 ci4 */
    MCOPY(ui_input_start_button_pal, 32);  /* palette */
    MCOPY(ui_input_start_button2_png, 960);  /* 48x40 ci4 */
    MCOPY(ui_input_start_button2_pal, 32);  /* palette */
    MCOPY(ui_input_start_text_png, 96);  /* 24x8 ci4 */
    MCOPY(ui_input_start_text_pal, 32);  /* palette */
    MCOPY(ui_hammer_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_hammer_pal, 32);  /* palette */
    MCOPY(ui_hammer_disabled_pal, 32);  /* palette */
    MCOPY(ui_super_hammer_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_super_hammer_pal, 32);  /* palette */
    MCOPY(ui_super_hammer_disabled_pal, 32);  /* palette */
    MCOPY(ui_ultra_hammer_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_ultra_hammer_pal, 32);  /* palette */
    MCOPY(ui_ultra_hammer_disabled_pal, 32);  /* palette */
    MCOPY(ui_boots_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_boots_pal, 32);  /* palette */
    MCOPY(ui_boots_disabled_pal, 32);  /* palette */
    MCOPY(ui_super_boots_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_super_boots_pal, 32);  /* palette */
    MCOPY(ui_super_boots_disabled_pal, 32);  /* palette */
    MCOPY(ui_ultra_boots_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_ultra_boots_pal, 32);  /* palette */
    MCOPY(ui_ultra_boots_disabled_pal, 32);  /* palette */
    MCOPY(ui_item_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_item_pal, 32);  /* palette */
    MCOPY(ui_item_disabled_pal, 32);  /* palette */
    MCOPY(ui_star_spirit_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_star_spirit_pal, 32);  /* palette */
    MCOPY(ui_star_spirit_disabled_pal, 32);  /* palette */
    MCOPY(ui_arrow_left_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_arrow_left_pal, 32);  /* palette */
    MCOPY(ui_arrow_right_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_arrow_right_pal, 32);  /* palette */
    MCOPY(ui_point_right_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_point_right_pal, 32);  /* palette */
    MCOPY(ui_pip_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_pip_pal, 32);  /* palette */
    MCOPY(ui_pip2_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_pip2_pal, 32);  /* palette */
    MCOPY(ui_coin_sparkle_0_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_coin_sparkle_0_pal, 32);  /* palette */
    MCOPY(ui_coin_sparkle_1_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_coin_sparkle_1_pal, 32);  /* palette */
    MCOPY(ui_coin_sparkle_2_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_coin_sparkle_2_pal, 32);  /* palette */
    MCOPY(ui_coin_sparkle_3_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_coin_sparkle_3_pal, 32);  /* palette */
    MCOPY(ui_coin_sparkle_4_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_coin_sparkle_4_pal, 32);  /* palette */
    MCOPY(ui_coin_sparkle_5_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_coin_sparkle_5_pal, 32);  /* palette */
    MCOPY(ui_coin_sparkle_none_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_coin_sparkle_none_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_shine_png, 576);  /* 24x24 ia8 */
    MCOPY(ui_status_star_piece_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_piece_0_pal, 32);  /* palette */
    MCOPY(ui_status_star_piece_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_piece_1_pal, 32);  /* palette */
    MCOPY(ui_status_star_piece_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_piece_2_pal, 32);  /* palette */
    MCOPY(ui_menu_times_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_menu_times_pal, 32);  /* palette */
    MCOPY(ui_fp_cost_png, 64);  /* 16x8 ci4 */
    MCOPY(ui_fp_cost_pal, 32);  /* palette */
    MCOPY(ui_fp_cost_reduced_pal, 32);  /* palette */
    MCOPY(ui_fp_cost_reduced_twice_pal, 32);  /* palette */
    MCOPY(ui_fp_cost_notenough_pal, 32);  /* palette */
    MCOPY(ui_pow_cost_png, 64);  /* 16x8 ci4 */
    MCOPY(ui_pow_cost_pal, 32);  /* palette */
    MCOPY(ui_pow_cost_reduced_pal, 32);  /* palette */
    MCOPY(ui_pow_cost_reduced_twice_pal, 32);  /* palette */
    MCOPY(ui_pow_cost_notenough_pal, 32);  /* palette */
    MCOPY(ui_green_arrow_down_png, 64);  /* 16x8 ci4 */
    MCOPY(ui_green_arrow_down_pal, 32);  /* palette */
    MCOPY(ui_green_arrow_up_png, 64);  /* 16x8 ci4 */
    MCOPY(ui_green_arrow_up_pal, 32);  /* palette */
    MCOPY(ui_kaime_png, 672);  /* 56x24 ci4 */
    MCOPY(ui_kaime_pal, 32);  /* palette */
    MCOPY(ui_unused_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_unused_1_pal, 32);  /* palette */
    MCOPY(ui_unused_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_unused_2_pal, 32);  /* palette */
    MCOPY(ui_unused_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_unused_3_pal, 32);  /* palette */
    MCOPY(ui_red_bar1_png, 448);  /* 112x8 ci4 */
    MCOPY(ui_red_bar1_pal, 32);  /* palette */
    MCOPY(ui_empty_bar_png, 448);  /* 112x8 ci4 */
    MCOPY(ui_empty_bar_pal, 32);  /* palette */
    MCOPY(ui_red_bar2_png, 416);  /* 104x8 ci4 */
    MCOPY(ui_red_bar2_pal, 32);  /* palette */
    MCOPY(ui_eldstar_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_eldstar_pal, 32);  /* palette */
    MCOPY(ui_eldstar_disabled_pal, 32);  /* palette */
    MCOPY(ui_mamar_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_mamar_pal, 32);  /* palette */
    MCOPY(ui_mamar_disabled_pal, 32);  /* palette */
    MCOPY(ui_skolar_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_skolar_pal, 32);  /* palette */
    MCOPY(ui_skolar_disabled_pal, 32);  /* palette */
    MCOPY(ui_muskular_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_muskular_pal, 32);  /* palette */
    MCOPY(ui_muskular_disabled_pal, 32);  /* palette */
    MCOPY(ui_misstar_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_misstar_pal, 32);  /* palette */
    MCOPY(ui_misstar_disabled_pal, 32);  /* palette */
    MCOPY(ui_klevar_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_klevar_pal, 32);  /* palette */
    MCOPY(ui_klevar_disabled_pal, 32);  /* palette */
    MCOPY(ui_kalmar_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_kalmar_pal, 32);  /* palette */
    MCOPY(ui_kalmar_disabled_pal, 32);  /* palette */
    MCOPY(ui_star_beam_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_star_beam_pal, 32);  /* palette */
    MCOPY(ui_star_beam_disabled_pal, 32);  /* palette */
    MCOPY(ui_peach_beam_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_peach_beam_pal, 32);  /* palette */
    MCOPY(ui_peach_beam_disabled_pal, 32);  /* palette */
    MCOPY(ui_mario_head_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_mario_head_pal, 32);  /* palette */
    MCOPY(ui_partner0_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_partner0_pal, 32);  /* palette */
    MCOPY(ui_partner0_disabled_pal, 32);  /* palette */
    MCOPY(ui_goombario_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_goombario_pal, 32);  /* palette */
    MCOPY(ui_goombario_disabled_pal, 32);  /* palette */
    MCOPY(ui_kooper_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_kooper_pal, 32);  /* palette */
    MCOPY(ui_kooper_disabled_pal, 32);  /* palette */
    MCOPY(ui_bombette_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_bombette_pal, 32);  /* palette */
    MCOPY(ui_bombette_disabled_pal, 32);  /* palette */
    MCOPY(ui_parakarry_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_parakarry_pal, 32);  /* palette */
    MCOPY(ui_parakarry_disabled_pal, 32);  /* palette */
    MCOPY(ui_bow_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_bow_pal, 32);  /* palette */
    MCOPY(ui_bow_disabled_pal, 32);  /* palette */
    MCOPY(ui_watt_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_watt_pal, 32);  /* palette */
    MCOPY(ui_watt_disabled_pal, 32);  /* palette */
    MCOPY(ui_sushie_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_sushie_pal, 32);  /* palette */
    MCOPY(ui_sushie_disabled_pal, 32);  /* palette */
    MCOPY(ui_lakilester_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_lakilester_pal, 32);  /* palette */
    MCOPY(ui_lakilester_disabled_pal, 32);  /* palette */
    MCOPY(ui_partner9_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_partner9_pal, 32);  /* palette */
    MCOPY(ui_partner9_disabled_pal, 32);  /* palette */
    MCOPY(ui_partner10_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_partner10_pal, 32);  /* palette */
    MCOPY(ui_partner10_disabled_pal, 32);  /* palette */
    MCOPY(ui_partner11_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_partner11_pal, 32);  /* palette */
    MCOPY(ui_partner11_disabled_pal, 32);  /* palette */
    MCOPY(ui_status_text_times_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_text_times_pal, 32);  /* palette */
    MCOPY(ui_status_text_slash_png, 64);  /* 8x16 ci4 */
    MCOPY(ui_status_text_slash_pal, 32);  /* palette */
    MCOPY(ui_status_text_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_0_pal, 32);  /* palette */
    MCOPY(ui_status_text_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_1_pal, 32);  /* palette */
    MCOPY(ui_status_text_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_2_pal, 32);  /* palette */
    MCOPY(ui_status_text_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_3_pal, 32);  /* palette */
    MCOPY(ui_status_text_4_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_4_pal, 32);  /* palette */
    MCOPY(ui_status_text_5_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_5_pal, 32);  /* palette */
    MCOPY(ui_status_text_6_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_6_pal, 32);  /* palette */
    MCOPY(ui_status_text_7_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_7_pal, 32);  /* palette */
    MCOPY(ui_status_text_8_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_8_pal, 32);  /* palette */
    MCOPY(ui_status_text_9_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_9_pal, 32);  /* palette */
    MCOPY(ui_status_text_hp_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_hp_pal, 32);  /* palette */
    MCOPY(ui_status_text_fp_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_text_fp_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_1_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_1_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_2_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_2_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_3_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_3_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_4_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_4_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_5_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_5_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_6_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_6_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_7_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_7_pal, 32);  /* palette */
    MCOPY(ui_status_pow_unit_empty_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_unit_empty_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_1_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_1_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_2_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_2_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_3_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_3_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_4_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_4_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_5_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_5_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_6_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_6_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_7_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_7_pal, 32);  /* palette */
    MCOPY(ui_status_pow_star_empty_png, 32);  /* 8x8 ci4 */
    MCOPY(ui_status_pow_star_empty_pal, 32);  /* palette */
    MCOPY(ui_status_coin_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_0_pal, 32);  /* palette */
    MCOPY(ui_status_coin_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_1_pal, 32);  /* palette */
    MCOPY(ui_status_coin_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_2_pal, 32);  /* palette */
    MCOPY(ui_status_coin_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_3_pal, 32);  /* palette */
    MCOPY(ui_status_coin_4_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_4_pal, 32);  /* palette */
    MCOPY(ui_status_coin_5_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_5_pal, 32);  /* palette */
    MCOPY(ui_status_coin_6_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_6_pal, 32);  /* palette */
    MCOPY(ui_status_coin_7_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_7_pal, 32);  /* palette */
    MCOPY(ui_status_coin_8_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_8_pal, 32);  /* palette */
    MCOPY(ui_status_coin_9_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_coin_9_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_0_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_1_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_2_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_3_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_4_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_4_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_5_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_5_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_6_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_6_pal, 32);  /* palette */
    MCOPY(ui_status_star_point_7_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_status_star_point_7_pal, 32);  /* palette */
    MCOPY(ui_bluepip_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_bluepip_pal, 32);  /* palette */
    MCOPY(ui_bluepip2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_bluepip2_pal, 32);  /* palette */
    MCOPY(ui_battle_status_charge_jump_png, 288);  /* 24x24 ci4 */
    MCOPY(ui_battle_status_charge_jump_pal, 32);  /* palette */
    MCOPY(ui_battle_status_charge_hammer_png, 288);  /* 24x24 ci4 */
    MCOPY(ui_battle_status_charge_hammer_pal, 32);  /* palette */
    MCOPY(ui_battle_status_charge_goombario_png, 288);  /* 24x24 ci4 */
    MCOPY(ui_battle_status_charge_goombario_pal, 32);  /* palette */
    MCOPY(ui_battle_status_exclamation_png, 288);  /* 24x24 ci4 */
    MCOPY(ui_battle_status_exclamation_pal, 32);  /* palette */
    MCOPY(ui_battle_status_sleep_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_sleep_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_sleep_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_sleep_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_sleep_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_sleep_2_pal, 32);  /* palette */
    MCOPY(ui_battle_status_static_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_static_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_static_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_static_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_paralyze_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_paralyze_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_paralyze_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_paralyze_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_dizzy_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_dizzy_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_dizzy_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_dizzy_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_dizzy_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_dizzy_2_pal, 32);  /* palette */
    MCOPY(ui_battle_status_dizzy_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_dizzy_3_pal, 32);  /* palette */
    MCOPY(ui_battle_status_poison_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_poison_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_poison_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_poison_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_frozen_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_frozen_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_frozen_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_frozen_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_frozen_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_frozen_2_pal, 32);  /* palette */
    MCOPY(ui_battle_status_frozen_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_frozen_3_pal, 32);  /* palette */
    MCOPY(ui_battle_status_stop_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_stop_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_stop_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_stop_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_stop_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_stop_2_pal, 32);  /* palette */
    MCOPY(ui_battle_status_stop_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_stop_3_pal, 32);  /* palette */
    MCOPY(ui_battle_status_pdown_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_pdown_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_pdown_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_pdown_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_shrink_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_shrink_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_shrink_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_shrink_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_shrink_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_shrink_2_pal, 32);  /* palette */
    MCOPY(ui_battle_status_shrink_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_shrink_3_pal, 32);  /* palette */
    MCOPY(ui_battle_status_transparent_0_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_transparent_0_pal, 32);  /* palette */
    MCOPY(ui_battle_status_transparent_1_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_transparent_1_pal, 32);  /* palette */
    MCOPY(ui_battle_status_transparent_2_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_transparent_2_pal, 32);  /* palette */
    MCOPY(ui_battle_status_transparent_3_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_transparent_3_pal, 32);  /* palette */
    MCOPY(ui_battle_status_transparent_4_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_transparent_4_pal, 32);  /* palette */
    MCOPY(ui_battle_status_transparent_5_png, 128);  /* 16x16 ci4 */
    MCOPY(ui_battle_status_transparent_5_pal, 32);  /* palette */
    MCOPY(ui_battle_status_peril_png, 320);  /* 40x16 ci4 */
    MCOPY(ui_battle_status_peril_pal, 32);  /* palette */
    MCOPY(ui_battle_status_danger_png, 320);  /* 40x16 ci4 */
    MCOPY(ui_battle_status_danger_pal, 32);  /* palette */
    MCOPY(ui_battle_status_refund_png, 480);  /* 40x24 ci4 */
    MCOPY(ui_battle_status_refund_pal, 32);  /* palette */
    MCOPY(ui_battle_status_happy_png, 320);  /* 40x16 ci4 */
    MCOPY(ui_battle_status_happy_pal, 32);  /* palette */
    MCOPY(ui_battle_status_hp_drain_png, 320);  /* 40x16 ci4 */
    MCOPY(ui_battle_status_hp_drain_pal, 32);  /* palette */
    MCOPY(ui_move_basic_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_move_basic_pal, 32);  /* palette */
    MCOPY(ui_move_basic_disabled_pal, 32);  /* palette */
    MCOPY(ui_move_partner_1_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_move_partner_1_pal, 32);  /* palette */
    MCOPY(ui_move_partner_1_disabled_pal, 32);  /* palette */
    MCOPY(ui_move_partner_2_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_move_partner_2_pal, 32);  /* palette */
    MCOPY(ui_move_partner_2_disabled_pal, 32);  /* palette */
    MCOPY(ui_move_partner_3_png, 512);  /* 32x32 ci4 */
    MCOPY(ui_move_partner_3_pal, 32);  /* palette */
    MCOPY(ui_move_partner_3_disabled_pal, 32);  /* palette */
    p += 8;  /* gap/padding */
    MCOPY(ui_stat_heart_png, 1024);  /* 16x16 rgba32 */
    p += 8;  /* gap/padding */
    MCOPY(ui_stat_flower_png, 1024);  /* 16x16 rgba32 */

    #undef MCOPY

    int used = (int)(p - ghsBuf);
    fprintf(stderr, "[PORT] global_hud_scripts textures loaded: %d bytes from ROM 0x%X (expected %d)\n",
            used, GHS_SEG_ROM, GHS_SEG_SIZE);
}
/* battle_ui_gfx .data segment: ROM 0x1B7CE0 - 0x1C0860 (35712 bytes = 0x8B80) */
/* 150 texture/palette entries */

/* ===== battle_ui_gfx externs ===== */
extern u8 ui_battle_menu_flee_png[];
extern u8 ui_battle_menu_flee_pal[];
extern u8 ui_battle_menu_flee_disabled_pal[];
extern u8 ui_battle_menu_defend_png[];
extern u8 ui_battle_menu_defend_pal[];
extern u8 ui_battle_menu_defend_disabled_pal[];
extern u8 ui_battle_menu_strategies_png[];
extern u8 ui_battle_menu_strategies_pal[];
extern u8 ui_battle_menu_strategies_disabled_pal[];
extern u8 ui_battle_menu_nothing_png[];
extern u8 ui_battle_menu_nothing_pal[];
extern u8 ui_battle_menu_nothing_disabled_pal[];
extern u8 ui_battle_menu_pouch_png[];
extern u8 ui_battle_menu_pouch_pal[];
extern u8 ui_battle_menu_pouch_disabled_pal[];
extern u8 ui_battle_swap_Z_png[];
extern u8 ui_battle_swap_Z_pal[];
extern u8 ui_battle_swap_arrow_left_png[];
extern u8 ui_battle_swap_arrow_left_pal[];
extern u8 ui_battle_swap_arrow_right_png[];
extern u8 ui_battle_swap_arrow_right_pal[];
extern u8 ui_battle_menu_spirits_png[];
extern u8 ui_battle_menu_spirits_pal[];
extern u8 ui_battle_yellow_arrow_png[];
extern u8 ui_battle_yellow_arrow_pal[];
extern u8 ui_battle_hp_bar_png[];
extern u8 ui_battle_hp_bar_pal[];
extern u8 ui_battle_hp_0_png[];
extern u8 ui_battle_hp_0_pal[];
extern u8 ui_battle_hp_1_png[];
extern u8 ui_battle_hp_1_pal[];
extern u8 ui_battle_hp_2_png[];
extern u8 ui_battle_hp_2_pal[];
extern u8 ui_battle_hp_3_png[];
extern u8 ui_battle_hp_3_pal[];
extern u8 ui_battle_hp_4_png[];
extern u8 ui_battle_hp_4_pal[];
extern u8 ui_battle_hp_5_png[];
extern u8 ui_battle_hp_5_pal[];
extern u8 ui_battle_hp_6_png[];
extern u8 ui_battle_hp_6_pal[];
extern u8 ui_battle_hp_7_png[];
extern u8 ui_battle_hp_7_pal[];
extern u8 ui_battle_hp_8_png[];
extern u8 ui_battle_hp_8_pal[];
extern u8 ui_battle_hp_9_png[];
extern u8 ui_battle_hp_9_pal[];
extern u8 ui_battle_cont_b_up_png[];
extern u8 ui_battle_cont_b_up_pal[];
extern u8 ui_battle_cont_b_down_png[];
extern u8 ui_battle_cont_b_down_pal[];
extern u8 ui_battle_cont_c_left_up_png[];
extern u8 ui_battle_cont_c_left_up_pal[];
extern u8 ui_battle_cont_c_left_down_png[];
extern u8 ui_battle_cont_c_left_down_pal[];
extern u8 ui_battle_cont_c_right_up_png[];
extern u8 ui_battle_cont_c_right_up_pal[];
extern u8 ui_battle_cont_c_right_down_png[];
extern u8 ui_battle_cont_c_right_down_pal[];
extern u8 ui_battle_cont_c_up_up_png[];
extern u8 ui_battle_cont_c_up_up_pal[];
extern u8 ui_battle_cont_c_up_down_png[];
extern u8 ui_battle_cont_c_up_down_pal[];
extern u8 ui_battle_cont_c_down_up_png[];
extern u8 ui_battle_cont_c_down_up_pal[];
extern u8 ui_battle_cont_c_down_down_png[];
extern u8 ui_battle_cont_c_down_down_pal[];
extern u8 ui_battle_cmd_timing_1_chance_png[];
extern u8 ui_battle_cmd_timing_1_chance_pal[];
extern u8 ui_battle_cmd_timing_ready_png[];
extern u8 ui_battle_cmd_timing_ready_pal[];
extern u8 ui_battle_cmd_timing_wait_png[];
extern u8 ui_battle_cmd_timing_wait_pal[];
extern u8 ui_battle_cmd_charge_1_png[];
extern u8 ui_battle_cmd_charge_1_pal[];
extern u8 ui_battle_cmd_charge_2_png[];
extern u8 ui_battle_cmd_charge_2_pal[];
extern u8 ui_battle_cmd_charge_3_png[];
extern u8 ui_battle_cmd_charge_3_pal[];
extern u8 ui_battle_cmd_charge_4_png[];
extern u8 ui_battle_cmd_charge_4_pal[];
extern u8 ui_battle_cmd_timing_3_chances_png[];
extern u8 ui_battle_cmd_timing_3_chances_pal[];
extern u8 ui_battle_cmd_gauge_result_png[];
extern u8 ui_battle_cmd_gauge_result_pal[];
extern u8 ui_battle_cmd_timing_ok_png[];
extern u8 ui_battle_cmd_timing_ok_pal[];
extern u8 ui_battle_cmd_timing_miss_png[];
extern u8 ui_battle_cmd_timing_miss_pal[];
extern u8 ui_battle_cmd_flee_0_png[];
extern u8 ui_battle_cmd_flee_0_pal[];
extern u8 ui_battle_cmd_flee_1_png[];
extern u8 ui_battle_cmd_flee_1_pal[];
extern u8 ui_battle_cmd_flee_2_png[];
extern u8 ui_battle_cmd_flee_2_pal[];
extern u8 ui_battle_cmd_100pct_png[];
extern u8 ui_battle_cmd_100pct_pal[];
extern u8 ui_battle_cmd_right_on_png[];
extern u8 ui_battle_cmd_right_on_pal[];
extern u8 ui_battle_cmd_fill_result_png[];
extern u8 ui_battle_cmd_fill_result_pal[];
extern u8 ui_battle_cmd_aim_marker_0_png[];
extern u8 ui_battle_cmd_aim_marker_0_pal[];
extern u8 ui_battle_cmd_aim_marker_1_png[];
extern u8 ui_battle_cmd_aim_marker_1_pal[];
extern u8 ui_battle_cmd_aim_marker_2_png[];
extern u8 ui_battle_cmd_aim_marker_2_pal[];
extern u8 ui_battle_cmd_aim_marker_3_png[];
extern u8 ui_battle_cmd_aim_marker_3_pal[];
extern u8 ui_battle_cmd_aim_marker_4_png[];
extern u8 ui_battle_cmd_aim_marker_4_pal[];
extern u8 ui_battle_cmd_aim_marker_5_png[];
extern u8 ui_battle_cmd_aim_marker_5_pal[];
extern u8 ui_battle_cmd_aim_marker_6_png[];
extern u8 ui_battle_cmd_aim_marker_6_pal[];
extern u8 ui_battle_cmd_aim_marker_7_png[];
extern u8 ui_battle_cmd_aim_marker_7_pal[];
extern u8 ui_battle_cmd_aim_shimmer_0_png[];
extern u8 ui_battle_cmd_aim_shimmer_0_pal[];
extern u8 ui_battle_cmd_aim_shimmer_1_png[];
extern u8 ui_battle_cmd_aim_shimmer_1_pal[];
extern u8 ui_battle_cmd_aim_shimmer_2_png[];
extern u8 ui_battle_cmd_aim_shimmer_2_pal[];
extern u8 ui_battle_cmd_aim_shimmer_3_png[];
extern u8 ui_battle_cmd_aim_shimmer_3_pal[];
extern u8 ui_battle_cmd_aim_shimmer_4_png[];
extern u8 ui_battle_cmd_aim_shimmer_4_pal[];
extern u8 ui_battle_cmd_aim_shimmer_5_png[];
extern u8 ui_battle_cmd_aim_shimmer_5_pal[];
extern u8 ui_battle_cmd_aim_shimmer_6_png[];
extern u8 ui_battle_cmd_aim_shimmer_6_pal[];
extern u8 ui_battle_cmd_aim_shimmer_7_png[];
extern u8 ui_battle_cmd_aim_shimmer_7_pal[];
extern u8 ui_battle_cmd_aim_reticle_0_png[];
extern u8 ui_battle_cmd_aim_reticle_0_pal[];
extern u8 ui_battle_cmd_aim_reticle_1_png[];
extern u8 ui_battle_cmd_aim_reticle_1_pal[];
extern u8 ui_battle_cmd_aim_target_0_png[];
extern u8 ui_battle_cmd_aim_target_0_pal[];
extern u8 ui_battle_cmd_aim_target_1_png[];
extern u8 ui_battle_cmd_aim_target_1_pal[];
extern u8 ui_battle_solid_star_png[];
extern u8 ui_battle_solid_star_pal[];
extern u8 ui_battle_projector_spot_png[];
extern u8 ui_battle_projector_beam_png[];
extern u8 ui_battle_projector_reel_png[];
extern u8 ui_battle_swap_background_png[];
extern u8 ui_battle_cursor_hand_png[];
extern u8 ui_battle_cursor_hand_pal[];

static void port_load_battle_ui_textures(void) {
    #define BTL_SEG_ROM  0x1B7CE0
    #define BTL_SEG_SIZE 0x8B80  /* 35712 bytes */
    static u8 btlBuf[BTL_SEG_SIZE];
    nuPiReadRom(BTL_SEG_ROM, btlBuf, BTL_SEG_SIZE);

    #define BCOPY(dest, rom_off, sz) memcpy(dest, btlBuf + ((rom_off) - BTL_SEG_ROM), sz)

    /* skip ui_battle_menu_unused_png (unused) */
    /* skip ui_battle_menu_unused_pal (unused) */
    /* skip ui_battle_menu_unused_disabled_pal (unused) */
    BCOPY(ui_battle_menu_flee_png, 0x1B7F20, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_menu_flee_pal, 0x1B8120, 32);  /* palette */
    BCOPY(ui_battle_menu_flee_disabled_pal, 0x1B8140, 32);  /* palette */
    BCOPY(ui_battle_menu_defend_png, 0x1B8160, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_menu_defend_pal, 0x1B8360, 32);  /* palette */
    BCOPY(ui_battle_menu_defend_disabled_pal, 0x1B8380, 32);  /* palette */
    BCOPY(ui_battle_menu_strategies_png, 0x1B83A0, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_menu_strategies_pal, 0x1B85A0, 32);  /* palette */
    BCOPY(ui_battle_menu_strategies_disabled_pal, 0x1B85C0, 32);  /* palette */
    BCOPY(ui_battle_menu_nothing_png, 0x1B85E0, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_menu_nothing_pal, 0x1B87E0, 32);  /* palette */
    BCOPY(ui_battle_menu_nothing_disabled_pal, 0x1B8800, 32);  /* palette */
    BCOPY(ui_battle_menu_pouch_png, 0x1B8820, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_menu_pouch_pal, 0x1B8A20, 32);  /* palette */
    BCOPY(ui_battle_menu_pouch_disabled_pal, 0x1B8A40, 32);  /* palette */
    BCOPY(ui_battle_swap_Z_png, 0x1B8A60, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_swap_Z_pal, 0x1B8AE0, 32);  /* palette */
    BCOPY(ui_battle_swap_arrow_left_png, 0x1B8B00, 64);  /* 8x16 ci4 */
    BCOPY(ui_battle_swap_arrow_left_pal, 0x1B8B40, 32);  /* palette */
    BCOPY(ui_battle_swap_arrow_right_png, 0x1B8B60, 64);  /* 8x16 ci4 */
    BCOPY(ui_battle_swap_arrow_right_pal, 0x1B8BA0, 400);  /* palette 400 bytes */
    BCOPY(ui_battle_menu_spirits_png, 0x1B8D40, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_menu_spirits_pal, 0x1B8F40, 32);  /* palette */
    BCOPY(ui_battle_yellow_arrow_png, 0x1B8F60, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_yellow_arrow_pal, 0x1B8FE0, 320);  /* palette 320 bytes */
    BCOPY(ui_battle_hp_bar_png, 0x1B9120, 256);  /* 32x16 ci4 */
    BCOPY(ui_battle_hp_bar_pal, 0x1B9220, 32);  /* palette */
    BCOPY(ui_battle_hp_0_png, 0x1B9240, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_0_pal, 0x1B9260, 32);  /* palette */
    BCOPY(ui_battle_hp_1_png, 0x1B9280, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_1_pal, 0x1B92A0, 32);  /* palette */
    BCOPY(ui_battle_hp_2_png, 0x1B92C0, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_2_pal, 0x1B92E0, 32);  /* palette */
    BCOPY(ui_battle_hp_3_png, 0x1B9300, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_3_pal, 0x1B9320, 32);  /* palette */
    BCOPY(ui_battle_hp_4_png, 0x1B9340, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_4_pal, 0x1B9360, 32);  /* palette */
    BCOPY(ui_battle_hp_5_png, 0x1B9380, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_5_pal, 0x1B93A0, 32);  /* palette */
    BCOPY(ui_battle_hp_6_png, 0x1B93C0, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_6_pal, 0x1B93E0, 32);  /* palette */
    BCOPY(ui_battle_hp_7_png, 0x1B9400, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_7_pal, 0x1B9420, 32);  /* palette */
    BCOPY(ui_battle_hp_8_png, 0x1B9440, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_8_pal, 0x1B9460, 32);  /* palette */
    BCOPY(ui_battle_hp_9_png, 0x1B9480, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_hp_9_pal, 0x1B94A0, 32);  /* palette */
    BCOPY(ui_battle_cont_b_up_png, 0x1B94C0, 960);  /* 48x40 ci4 */
    BCOPY(ui_battle_cont_b_up_pal, 0x1B9880, 32);  /* palette */
    BCOPY(ui_battle_cont_b_down_png, 0x1B98A0, 960);  /* 48x40 ci4 */
    BCOPY(ui_battle_cont_b_down_pal, 0x1B9C60, 32);  /* palette */
    BCOPY(ui_battle_cont_c_left_up_png, 0x1B9C80, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_left_up_pal, 0x1B9F28, 32);  /* palette */
    BCOPY(ui_battle_cont_c_left_down_png, 0x1B9F48, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_left_down_pal, 0x1BA1F0, 32);  /* palette */
    BCOPY(ui_battle_cont_c_right_up_png, 0x1BA210, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_right_up_pal, 0x1BA4B8, 32);  /* palette */
    BCOPY(ui_battle_cont_c_right_down_png, 0x1BA4D8, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_right_down_pal, 0x1BA780, 32);  /* palette */
    BCOPY(ui_battle_cont_c_up_up_png, 0x1BA7A0, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_up_up_pal, 0x1BAA48, 32);  /* palette */
    BCOPY(ui_battle_cont_c_up_down_png, 0x1BAA68, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_up_down_pal, 0x1BAD10, 32);  /* palette */
    BCOPY(ui_battle_cont_c_down_up_png, 0x1BAD30, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_down_up_pal, 0x1BAFD8, 32);  /* palette */
    BCOPY(ui_battle_cont_c_down_down_png, 0x1BAFF8, 680);  /* 40x34 ci4 */
    BCOPY(ui_battle_cont_c_down_down_pal, 0x1BB2A0, 32);  /* palette */
    BCOPY(ui_battle_cmd_timing_1_chance_png, 0x1BB2C0, 864);  /* 72x24 ci4 */
    BCOPY(ui_battle_cmd_timing_1_chance_pal, 0x1BB620, 32);  /* palette */
    BCOPY(ui_battle_cmd_timing_ready_png, 0x1BB640, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_timing_ready_pal, 0x1BB6C0, 32);  /* palette */
    BCOPY(ui_battle_cmd_timing_wait_png, 0x1BB6E0, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_timing_wait_pal, 0x1BB760, 32);  /* palette */
    BCOPY(ui_battle_cmd_charge_1_png, 0x1BB780, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_cmd_charge_1_pal, 0x1BB7A0, 32);  /* palette */
    BCOPY(ui_battle_cmd_charge_2_png, 0x1BB7C0, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_cmd_charge_2_pal, 0x1BB7E0, 32);  /* palette */
    BCOPY(ui_battle_cmd_charge_3_png, 0x1BB800, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_cmd_charge_3_pal, 0x1BB820, 32);  /* palette */
    BCOPY(ui_battle_cmd_charge_4_png, 0x1BB840, 32);  /* 8x8 ci4 */
    BCOPY(ui_battle_cmd_charge_4_pal, 0x1BB860, 32);  /* palette */
    BCOPY(ui_battle_cmd_timing_3_chances_png, 0x1BB880, 1152);  /* 96x24 ci4 */
    BCOPY(ui_battle_cmd_timing_3_chances_pal, 0x1BBD00, 32);  /* palette */
    BCOPY(ui_battle_cmd_gauge_result_png, 0x1BBD20, 384);  /* 32x24 ci4 */
    BCOPY(ui_battle_cmd_gauge_result_pal, 0x1BBEA0, 32);  /* palette */
    BCOPY(ui_battle_cmd_timing_ok_png, 0x1BBEC0, 288);  /* 24x24 ci4 */
    BCOPY(ui_battle_cmd_timing_ok_pal, 0x1BBFE0, 32);  /* palette */
    BCOPY(ui_battle_cmd_timing_miss_png, 0x1BC000, 288);  /* 24x24 ci4 */
    BCOPY(ui_battle_cmd_timing_miss_pal, 0x1BC120, 32);  /* palette */
    BCOPY(ui_battle_cmd_flee_0_png, 0x1BC140, 192);  /* 16x24 ci4 */
    BCOPY(ui_battle_cmd_flee_0_pal, 0x1BC200, 32);  /* palette */
    BCOPY(ui_battle_cmd_flee_1_png, 0x1BC220, 192);  /* 16x24 ci4 */
    BCOPY(ui_battle_cmd_flee_1_pal, 0x1BC2E0, 32);  /* palette */
    BCOPY(ui_battle_cmd_flee_2_png, 0x1BC300, 192);  /* 16x24 ci4 */
    BCOPY(ui_battle_cmd_flee_2_pal, 0x1BC3C0, 32);  /* palette */
    BCOPY(ui_battle_cmd_100pct_png, 0x1BC3E0, 1536);  /* 64x48 ci4 */
    BCOPY(ui_battle_cmd_100pct_pal, 0x1BC9E0, 32);  /* palette */
    BCOPY(ui_battle_cmd_right_on_png, 0x1BCA00, 672);  /* 56x24 ci4 */
    BCOPY(ui_battle_cmd_right_on_pal, 0x1BCCA0, 32);  /* palette */
    BCOPY(ui_battle_cmd_fill_result_png, 0x1BCCC0, 384);  /* 32x24 ci4 */
    BCOPY(ui_battle_cmd_fill_result_pal, 0x1BCE40, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_0_png, 0x1BCE60, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_0_pal, 0x1BCEE0, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_1_png, 0x1BCF00, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_1_pal, 0x1BCF80, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_2_png, 0x1BCFA0, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_2_pal, 0x1BD020, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_3_png, 0x1BD040, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_3_pal, 0x1BD0C0, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_4_png, 0x1BD0E0, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_4_pal, 0x1BD160, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_5_png, 0x1BD180, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_5_pal, 0x1BD200, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_6_png, 0x1BD220, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_6_pal, 0x1BD2A0, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_marker_7_png, 0x1BD2C0, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_marker_7_pal, 0x1BD340, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_0_png, 0x1BD360, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_0_pal, 0x1BD3E0, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_1_png, 0x1BD400, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_1_pal, 0x1BD480, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_2_png, 0x1BD4A0, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_2_pal, 0x1BD520, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_3_png, 0x1BD540, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_3_pal, 0x1BD5C0, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_4_png, 0x1BD5E0, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_4_pal, 0x1BD660, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_5_png, 0x1BD680, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_5_pal, 0x1BD700, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_6_png, 0x1BD720, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_6_pal, 0x1BD7A0, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_shimmer_7_png, 0x1BD7C0, 128);  /* 16x16 ci4 */
    BCOPY(ui_battle_cmd_aim_shimmer_7_pal, 0x1BD840, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_reticle_0_png, 0x1BD860, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_cmd_aim_reticle_0_pal, 0x1BDA60, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_reticle_1_png, 0x1BDA80, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_cmd_aim_reticle_1_pal, 0x1BDC80, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_target_0_png, 0x1BDCA0, 288);  /* 24x24 ci4 */
    BCOPY(ui_battle_cmd_aim_target_0_pal, 0x1BDDC0, 32);  /* palette */
    BCOPY(ui_battle_cmd_aim_target_1_png, 0x1BDDE0, 288);  /* 24x24 ci4 */
    BCOPY(ui_battle_cmd_aim_target_1_pal, 0x1BDF00, 32);  /* palette */
    BCOPY(ui_battle_solid_star_png, 0x1BDF20, 512);  /* 32x32 ci4 */
    BCOPY(ui_battle_solid_star_pal, 0x1BE120, 32);  /* palette */
    BCOPY(ui_battle_projector_spot_png, 0x1BE148, 1024);  /* 32x32 ia8 */
    BCOPY(ui_battle_projector_beam_png, 0x1BE550, 4096);  /* 64x64 ia8 */
    BCOPY(ui_battle_projector_reel_png, 0x1BF558, 4096);  /* 64x64 ia8 */
    BCOPY(ui_battle_swap_background_png, 0x1C0560, 768);  /* 48x16 ia8 */

    #undef BCOPY

    fprintf(stderr, "[PORT] battle_ui_gfx textures loaded: %d bytes from ROM 0x%X (expected %d)\n",
            BTL_SEG_SIZE, BTL_SEG_ROM, BTL_SEG_SIZE);

    /* cursor_hand is in popup_messages .data segment, not battle_ui_gfx */
    nuPiReadRom(0x1B21E0, ui_battle_cursor_hand_png, 512);   /* 32x32 ci4 */
    nuPiReadRom(0x1B23E0, ui_battle_cursor_hand_pal, 32);    /* palette */
}
/* ================================================== */
/* Generated loaders for stencil, pause, theater      */
/* ================================================== */

/* ===== stencil textures externs ===== */
extern u8 ui_stencil_star_png[];
extern u8 ui_stencil_mario_png[];
extern u8 ui_stencil_sharp_circle_png[];
extern u8 ui_stencil_blurry_circle_png[];

static void port_load_stencil_textures(void) {
    #define SEG_ROM  0xE2E00
    #define SEG_SIZE 0x1E00  /* 7680 bytes */
    static u8 segBuf[SEG_SIZE];
    nuPiReadRom(SEG_ROM, segBuf, SEG_SIZE);

    #define BCOPY(dest, rom_off, sz) memcpy(dest, segBuf + ((rom_off) - SEG_ROM), sz)

    BCOPY(ui_stencil_star_png, 0xE2E00, 1024);  /* 32x64 i4 */
    BCOPY(ui_stencil_mario_png, 0xE3200, 2048);  /* 64x64 i4 */
    BCOPY(ui_stencil_sharp_circle_png, 0xE3A00, 512);  /* 32x32 i4 */
    BCOPY(ui_stencil_blurry_circle_png, 0xE3C00, 4096);  /* 64x64 i8 */

    #undef BCOPY

    fprintf(stderr, "[PORT] stencil textures loaded: %d bytes from ROM 0x%X\n",
            SEG_SIZE, SEG_ROM);
}

/* ===== pause_gfx textures externs ===== */
extern u8 pause_image_1439F0_png[];
extern u8 pause_image_143BF0_png[];
extern u8 pause_image_143FF0_png[];
extern u8 pause_world_map_png[];
extern u8 pause_world_map_pal[];
extern u8 pause_spirits_bg_png[];
extern u8 pause_spirits_bg_pal[];
extern u8 pause_orbs_png[];
extern u8 pause_map_location_png[];
extern u8 pause_map_path_marker_png[];
extern u8 pause_arrows_png[];
extern u8 pause_banner_hp_png[];
extern u8 pause_banner_hp_pal[];
extern u8 pause_banner_fp_png[];
extern u8 pause_banner_fp_pal[];
extern u8 pause_banner_bp_png[];
extern u8 pause_banner_bp_pal[];
extern u8 pause_banner_boots_png[];
extern u8 pause_banner_boots_pal[];
extern u8 pause_banner_hammer_png[];
extern u8 pause_banner_hammer_pal[];
extern u8 pause_banner_star_energy_png[];
extern u8 pause_banner_star_energy_pal[];
extern u8 pause_available_png[];
extern u8 pause_available_pal[];
extern u8 pause_prompt_check_abilities_png[];
extern u8 pause_prompt_check_abilities_pal[];
extern u8 pause_stats_bar_png[];
extern u8 pause_stats_bar_pal[];

static void port_load_pause_textures(void) {
    #define SEG_ROM  0x1439F0
    #define SEG_SIZE 0x1E2A0  /* 123552 bytes */
    static u8 segBuf[SEG_SIZE];
    nuPiReadRom(SEG_ROM, segBuf, SEG_SIZE);

    #define BCOPY(dest, rom_off, sz) memcpy(dest, segBuf + ((rom_off) - SEG_ROM), sz)

    BCOPY(pause_image_1439F0_png, 0x1439F0, 512);  /* 16x32 ia8 */
    BCOPY(pause_image_143BF0_png, 0x143BF0, 1024);  /* 16x64 ia8 */
    BCOPY(pause_image_143FF0_png, 0x143FF0, 1024);  /* 16x64 ia8 */
    BCOPY(pause_world_map_png, 0x1443F0, 102400);  /* 320x320 ci8 */
    BCOPY(pause_world_map_pal, 0x15D3F0, 512);  /* pal512 */
    BCOPY(pause_spirits_bg_png, 0x15D5F0, 7040);  /* 128x110 ci4 */
    BCOPY(pause_spirits_bg_pal, 0x15F170, 32);  /* pal32 */
    BCOPY(pause_orbs_png, 0x15F370, 512);  /* 8x32 rgba16 */
    BCOPY(pause_map_location_png, 0x15F570, 256);  /* 16x16 ia8 */
    BCOPY(pause_map_path_marker_png, 0x15F670, 256);  /* 16x16 ia8 */
    BCOPY(pause_arrows_png, 0x15F770, 512);  /* 16x64 ia4 */
    BCOPY(pause_banner_hp_png, 0x15F970, 512);  /* 64x16 ci4 */
    BCOPY(pause_banner_hp_pal, 0x15FB70, 32);  /* pal32 */
    BCOPY(pause_banner_fp_png, 0x15FD70, 512);  /* 64x16 ci4 */
    BCOPY(pause_banner_fp_pal, 0x15FF70, 32);  /* pal32 */
    BCOPY(pause_banner_bp_png, 0x160170, 512);  /* 64x16 ci4 */
    BCOPY(pause_banner_bp_pal, 0x160370, 32);  /* pal32 */
    BCOPY(pause_banner_boots_png, 0x160570, 384);  /* 48x16 ci4 */
    BCOPY(pause_banner_boots_pal, 0x1606F0, 32);  /* pal32 */
    BCOPY(pause_banner_hammer_png, 0x1608F0, 384);  /* 48x16 ci4 */
    BCOPY(pause_banner_hammer_pal, 0x160A70, 32);  /* pal32 */
    BCOPY(pause_banner_star_energy_png, 0x160C70, 384);  /* 48x16 ci4 */
    BCOPY(pause_banner_star_energy_pal, 0x160DF0, 32);  /* pal32 */
    BCOPY(pause_available_png, 0x160FF0, 512);  /* 64x16 ci4 */
    BCOPY(pause_available_pal, 0x1611F0, 32);  /* pal32 */
    BCOPY(pause_prompt_check_abilities_png, 0x1613F0, 1024);  /* 128x16 ci4 */
    BCOPY(pause_prompt_check_abilities_pal, 0x1617F0, 32);  /* pal32 */
    BCOPY(pause_stats_bar_png, 0x1619F0, 640);  /* 160x8 ci4 */
    BCOPY(pause_stats_bar_pal, 0x161C70, 32);  /* pal32 */

    #undef BCOPY

    fprintf(stderr, "[PORT] pause_gfx textures loaded: %d bytes from ROM 0x%X\n",
            SEG_SIZE, SEG_ROM);
}

/* ===== theater/curtains textures externs ===== */
extern u8 theater_walls_png[];
extern u8 theater_curtains_png[];
extern u8 theater_floor_png[];
extern u8 theater_floor_pal[];
extern u8 ui_no_controller_png[];

/* Theater vertex arrays (defined as Vtx[] in .vtx.inc.c stubs, accessed as raw bytes here) */
extern u8 theater_left_wall_vtx[];
extern u8 theater_right_wall_vtx[];
extern u8 theater_floor_vtx[];
extern u8 theater_left_inset_shadow_vtx[];
extern u8 theater_right_inset_shadow_vtx[];
extern u8 theater_curtain_vtx[];
extern u8 theater_wall_shadows_vtx[];

/**
 * Convert N64 big-endian vertices to PC format (GBI_FLOATS=1).
 * N64 Vtx_t: s16 ob[3] + u16 flag + s16 tc[2] + u8 cn[4] = 16 bytes
 * PC  Vtx:   float ob[3] + u16 flag + s16 tc[2] + u8 cn[4] + 2 pad = 24 bytes
 */
#define N64_VTX_SIZE  16
#define PC_VTX_STRIDE 24

static void convert_n64_vtx_to_pc(void* dest, const u8* src, s32 count) {
    u8* out = (u8*)dest;
    for (s32 i = 0; i < count; i++) {
        const u8* v = src + i * N64_VTX_SIZE;
        u8* o = out + i * PC_VTX_STRIDE;

        /* Position: s16 big-endian → float */
        s16 x = (s16)((v[0] << 8) | v[1]);
        s16 y = (s16)((v[2] << 8) | v[3]);
        s16 z = (s16)((v[4] << 8) | v[5]);
        float fx = (float)x, fy = (float)y, fz = (float)z;
        memcpy(o + 0,  &fx, 4);
        memcpy(o + 4,  &fy, 4);
        memcpy(o + 8,  &fz, 4);

        /* Flag: u16 (native endian on PC) */
        u16 flag = (u16)((v[6] << 8) | v[7]);
        memcpy(o + 12, &flag, 2);

        /* Texture coords: s16 (native endian on PC) */
        s16 tc0 = (s16)((v[8] << 8) | v[9]);
        s16 tc1 = (s16)((v[10] << 8) | v[11]);
        memcpy(o + 14, &tc0, 2);
        memcpy(o + 16, &tc1, 2);

        /* Color/normal: 4 bytes, no swap needed */
        o[18] = v[12];
        o[19] = v[13];
        o[20] = v[14];
        o[21] = v[15];

        /* Padding */
        o[22] = 0;
        o[23] = 0;
    }
}

/* ===== Entity drop shadows (src/entity/Shadow.c INCLUDE_IMG symbols) ===== */
extern u8 D_802E9170[]; /* entity/shadow/square, i4 16x16 */
extern u8 D_802E91F0[]; /* entity/shadow/circle, i4 16x16 */

/* ===== Map message images (shown inside message boxes) ===== */
extern u8 kmr_04_hammer_block_message_img[]; /* ci4 48x48 */
extern u8 kmr_04_hammer_block_message_pal[];
extern u8 kmr_02_heart_block_img[]; /* ci4 32x32 */
extern u8 kmr_02_heart_block_pal[];

static void port_load_shadow_textures(void) {
    nuPiReadRom(0x10A9F0, D_802E9170, 0x80);
    nuPiReadRom(0x10AA70, D_802E91F0, 0x80);
    fprintf(stderr, "[PORT] Entity shadow textures loaded: square+circle (128 bytes each)\n");
}

static void port_load_map_message_images(void) {
    nuPiReadRom(0x8CC4F0, kmr_04_hammer_block_message_img, 1152);
    nuPiReadRom(0x8CC970, kmr_04_hammer_block_message_pal, 32);
    nuPiReadRom(0x8C7B70, kmr_02_heart_block_img, 512);
    nuPiReadRom(0x8C7D70, kmr_02_heart_block_pal, 32);
    fprintf(stderr, "[PORT] Map message images loaded: kmr_04 hammer block, kmr_02 heart block\n");
}

static void port_load_theater_textures(void) {
    #define SEG_ROM  0x4F9E0
    #define SEG_SIZE 0x2B60  /* 11104 bytes */
    static u8 segBuf[SEG_SIZE];
    nuPiReadRom(SEG_ROM, segBuf, SEG_SIZE);

    #define BCOPY(dest, rom_off, sz) memcpy(dest, segBuf + ((rom_off) - SEG_ROM), sz)

    /* Textures */
    BCOPY(theater_walls_png, 0x4F9E0, 256);  /* 16x32 i4 */
    BCOPY(theater_curtains_png, 0x4FB30, 4096);  /* 32x32 rgba32 */
    BCOPY(theater_floor_png, 0x50B80, 256);  /* 32x16 ci4 */
    BCOPY(theater_floor_pal, 0x50C80, 32);  /* pal32 */
    BCOPY(ui_no_controller_png, 0x51540, 4096);  /* 128x32 ia8 */

    /* Vertex data: convert from N64 big-endian s16 to PC float (GBI_FLOATS) */
    #define VCOPY(dest, rom_off, count) \
        convert_n64_vtx_to_pc(dest, segBuf + ((rom_off) - SEG_ROM), count)

    VCOPY(theater_left_wall_vtx,          0x50D20,  6);
    VCOPY(theater_right_wall_vtx,         0x50D80,  6);
    VCOPY(theater_floor_vtx,              0x50DE0, 25);
    VCOPY(theater_left_inset_shadow_vtx,  0x50F70,  4);
    VCOPY(theater_right_inset_shadow_vtx, 0x50FB0,  4);
    VCOPY(theater_curtain_vtx,            0x50FF0, 31);
    VCOPY(theater_wall_shadows_vtx,       0x511E0,  8);

    #undef VCOPY
    #undef BCOPY

    fprintf(stderr, "[PORT] theater textures + vertices loaded: %d bytes from ROM 0x%X\n",
            SEG_SIZE, SEG_ROM);
}

/* ===== indicator icon textures (speech bubble, inspect, i-spy) ===== */
extern u8 speech_bubble_img[];
extern u8 speech_bubble_pal[];
extern u8 inspect_icon_img[];
extern u8 inspect_icon_pal[];
extern u8 ispy_icon_img[];
extern u8 ispy_icon_1_pal[];
extern u8 ispy_icon_2_pal[];
extern u8 ispy_icon_3_pal[];

static void port_load_indicator_icon_textures(void) {
    /* Speech bubble: ci4 32x32 + pal16 */
    nuPiReadRom(0xE215C0, speech_bubble_img, 512);
    nuPiReadRom(0xE217C0, speech_bubble_pal, 32);

    /* Inspect icon: ci4 32x32 + pal16 */
    nuPiReadRom(0xE20B30, inspect_icon_img, 512);
    nuPiReadRom(0xE20D30, inspect_icon_pal, 32);

    /* I-Spy icon: ci4 56x56 + 3 palettes */
    nuPiReadRom(0xE22B30, ispy_icon_img, 1568);
    nuPiReadRom(0xE23150, ispy_icon_1_pal, 32);
    nuPiReadRom(0xE23170, ispy_icon_2_pal, 32);
    nuPiReadRom(0xE23190, ispy_icon_3_pal, 32);

    fprintf(stderr, "[PORT] indicator icon textures loaded (speech bubble, inspect, i-spy)\n");
}

/* ===== level_up overlay HUD textures ===== */
/* CI4 icons used by HUD element system (HES_TEMPLATE macros in level_up.c) */
extern u8 level_up_heart_png[];
extern u8 level_up_heart_pal[];
extern u8 level_up_flower_png[];
extern u8 level_up_flower_pal[];
extern u8 level_up_leaves_png[];
extern u8 level_up_leaves_pal[];
extern u8 level_up_badge_png[];
extern u8 level_up_badge_pal[];
extern u8 level_up_FP_png[];
extern u8 level_up_FP_pal[];
extern u8 level_up_BP_png[];
extern u8 level_up_BP_pal[];
extern u8 level_up_HP_png[];
extern u8 level_up_HP_pal[];
extern u8 level_up_digit_0_png[];
extern u8 level_up_digit_0_pal[];
extern u8 level_up_digit_1_png[];
extern u8 level_up_digit_1_pal[];
extern u8 level_up_digit_2_png[];
extern u8 level_up_digit_2_pal[];
extern u8 level_up_digit_3_png[];
extern u8 level_up_digit_3_pal[];
extern u8 level_up_digit_4_png[];
extern u8 level_up_digit_4_pal[];
extern u8 level_up_digit_5_png[];
extern u8 level_up_digit_5_pal[];
extern u8 level_up_digit_6_png[];
extern u8 level_up_digit_6_pal[];
extern u8 level_up_digit_7_png[];
extern u8 level_up_digit_7_pal[];
extern u8 level_up_digit_8_png[];
extern u8 level_up_digit_8_pal[];
extern u8 level_up_digit_9_png[];
extern u8 level_up_digit_9_pal[];
extern u8 level_up_small_digit_0_png[];
extern u8 level_up_small_digit_0_pal[];
extern u8 level_up_small_digit_1_png[];
extern u8 level_up_small_digit_1_pal[];
extern u8 level_up_small_digit_2_png[];
extern u8 level_up_small_digit_2_pal[];
extern u8 level_up_small_digit_3_png[];
extern u8 level_up_small_digit_3_pal[];
extern u8 level_up_small_digit_4_png[];
extern u8 level_up_small_digit_4_pal[];
extern u8 level_up_small_digit_5_png[];
extern u8 level_up_small_digit_5_pal[];
extern u8 level_up_small_digit_6_png[];
extern u8 level_up_small_digit_6_pal[];
extern u8 level_up_small_digit_7_png[];
extern u8 level_up_small_digit_7_pal[];
extern u8 level_up_small_digit_8_png[];
extern u8 level_up_small_digit_8_pal[];
extern u8 level_up_small_digit_9_png[];
extern u8 level_up_small_digit_9_pal[];
extern u8 level_up_small_arrow_png[];
extern u8 level_up_small_arrow_pal[];
extern u8 level_up_digit_0_red_pal[];
extern u8 level_up_digit_1_red_pal[];
extern u8 level_up_digit_2_red_pal[];
extern u8 level_up_digit_3_red_pal[];
extern u8 level_up_digit_4_red_pal[];
extern u8 level_up_digit_5_red_pal[];
extern u8 level_up_digit_6_red_pal[];
extern u8 level_up_digit_7_red_pal[];
extern u8 level_up_digit_8_red_pal[];
extern u8 level_up_digit_9_red_pal[];
extern u8 level_up_small_digit_0_red_pal[];
extern u8 level_up_small_digit_1_red_pal[];
extern u8 level_up_small_digit_2_red_pal[];
extern u8 level_up_small_digit_3_red_pal[];
extern u8 level_up_small_digit_4_red_pal[];
extern u8 level_up_small_digit_5_red_pal[];
extern u8 level_up_small_digit_6_red_pal[];
extern u8 level_up_small_digit_7_red_pal[];
extern u8 level_up_small_digit_8_red_pal[];
extern u8 level_up_small_digit_9_red_pal[];
extern u8 level_up_small_arrow_red_pal[];
extern u8 level_up_digit_0_blue_pal[];
extern u8 level_up_digit_1_blue_pal[];
extern u8 level_up_digit_2_blue_pal[];
extern u8 level_up_digit_3_blue_pal[];
extern u8 level_up_digit_4_blue_pal[];
extern u8 level_up_digit_5_blue_pal[];
extern u8 level_up_digit_6_blue_pal[];
extern u8 level_up_digit_7_blue_pal[];
extern u8 level_up_digit_8_blue_pal[];
extern u8 level_up_digit_9_blue_pal[];
extern u8 level_up_small_digit_0_blue_pal[];
extern u8 level_up_small_digit_1_blue_pal[];
extern u8 level_up_small_digit_2_blue_pal[];
extern u8 level_up_small_digit_3_blue_pal[];
extern u8 level_up_small_digit_4_blue_pal[];
extern u8 level_up_small_digit_5_blue_pal[];
extern u8 level_up_small_digit_6_blue_pal[];
extern u8 level_up_small_digit_7_blue_pal[];
extern u8 level_up_small_digit_8_blue_pal[];
extern u8 level_up_small_digit_9_blue_pal[];
extern u8 level_up_small_arrow_blue_pal[];
extern u8 level_up_select_one_to_upgrade_png[];
extern u8 level_up_select_one_to_upgrade_pal[];

/* Level_up vertex arrays (big letter geometry) */
extern u8 D_802A1018_79EF58[];  /* draw_second_E, 8 vtx */
extern u8 D_802A1098_79EFD8[];  /* draw_V, 8 vtx */
extern u8 D_802A1118_79F058[];  /* draw_E, 8 vtx */
extern u8 D_802A1198_79F0D8[];  /* draw_second_L, 8 vtx */
extern u8 D_802A1218_79F158[];  /* draw_L, 8 vtx */
extern u8 D_802A1298_79F1D8[];  /* draw_P, 8 vtx */
extern u8 D_802A1318_79F258[];  /* draw_U, 8 vtx */
extern u8 D_802A1398_79F2D8[];  /* draw_exclamation_mark, 4 vtx */

/* Level_up big letter IA8 textures (64x64 = 4096 bytes each) */
extern u8 D_802A1418_79F358[];  /* big_V */
extern u8 D_802A2490_7A03D0[];  /* big_P */
extern u8 D_802A3508_7A1448[];  /* big_exclamation_mark */
extern u8 D_802A4580_7A24C0[];  /* big_U */
extern u8 D_802A55F8_7A3538[];  /* big_L */
extern u8 D_802A6670_7A45B0[];  /* big_E */

static void port_load_level_up_textures(void) {
    /*
     * Level_up overlay HUD textures are contiguous in ROM from
     * 0x7A59F0 (heart_png) to 0x7A7E30 (select_one_to_upgrade_pal end).
     * Total: 0x2440 = 9280 bytes. Load as one block and distribute.
     */
    #define LU_SEG_ROM  0x7A59F0
    #define LU_SEG_SIZE 0x2440
    static u8 luBuf[LU_SEG_SIZE];
    nuPiReadRom(LU_SEG_ROM, luBuf, LU_SEG_SIZE);

    u8* p = luBuf;
    #define LU_COPY(dest, sz) do { memcpy(dest, p, sz); p += (sz); } while(0)

    /* Icons: 40x40 CI4 = 800 bytes img + 32 bytes pal */
    LU_COPY(level_up_heart_png, 800);
    LU_COPY(level_up_heart_pal, 32);
    LU_COPY(level_up_flower_png, 800);
    LU_COPY(level_up_flower_pal, 32);
    LU_COPY(level_up_leaves_png, 800);
    LU_COPY(level_up_leaves_pal, 32);
    LU_COPY(level_up_badge_png, 800);
    LU_COPY(level_up_badge_pal, 32);

    /* Stat labels: 32x24 CI4 = 384 bytes img + 32 bytes pal */
    LU_COPY(level_up_FP_png, 384);
    LU_COPY(level_up_FP_pal, 32);
    LU_COPY(level_up_BP_png, 384);
    LU_COPY(level_up_BP_pal, 32);
    LU_COPY(level_up_HP_png, 384);
    LU_COPY(level_up_HP_pal, 32);

    /* Digits 0-9: 8x16 CI4 = 64 bytes img + 32 bytes pal (green) */
    LU_COPY(level_up_digit_0_png, 64);  LU_COPY(level_up_digit_0_pal, 32);
    LU_COPY(level_up_digit_1_png, 64);  LU_COPY(level_up_digit_1_pal, 32);
    LU_COPY(level_up_digit_2_png, 64);  LU_COPY(level_up_digit_2_pal, 32);
    LU_COPY(level_up_digit_3_png, 64);  LU_COPY(level_up_digit_3_pal, 32);
    LU_COPY(level_up_digit_4_png, 64);  LU_COPY(level_up_digit_4_pal, 32);
    LU_COPY(level_up_digit_5_png, 64);  LU_COPY(level_up_digit_5_pal, 32);
    LU_COPY(level_up_digit_6_png, 64);  LU_COPY(level_up_digit_6_pal, 32);
    LU_COPY(level_up_digit_7_png, 64);  LU_COPY(level_up_digit_7_pal, 32);
    LU_COPY(level_up_digit_8_png, 64);  LU_COPY(level_up_digit_8_pal, 32);
    LU_COPY(level_up_digit_9_png, 64);  LU_COPY(level_up_digit_9_pal, 32);

    /* Small digits 0-9 + arrow: 8x8 CI4 = 32 bytes img + 32 bytes pal (green) */
    LU_COPY(level_up_small_digit_0_png, 32);  LU_COPY(level_up_small_digit_0_pal, 32);
    LU_COPY(level_up_small_digit_1_png, 32);  LU_COPY(level_up_small_digit_1_pal, 32);
    LU_COPY(level_up_small_digit_2_png, 32);  LU_COPY(level_up_small_digit_2_pal, 32);
    LU_COPY(level_up_small_digit_3_png, 32);  LU_COPY(level_up_small_digit_3_pal, 32);
    LU_COPY(level_up_small_digit_4_png, 32);  LU_COPY(level_up_small_digit_4_pal, 32);
    LU_COPY(level_up_small_digit_5_png, 32);  LU_COPY(level_up_small_digit_5_pal, 32);
    LU_COPY(level_up_small_digit_6_png, 32);  LU_COPY(level_up_small_digit_6_pal, 32);
    LU_COPY(level_up_small_digit_7_png, 32);  LU_COPY(level_up_small_digit_7_pal, 32);
    LU_COPY(level_up_small_digit_8_png, 32);  LU_COPY(level_up_small_digit_8_pal, 32);
    LU_COPY(level_up_small_digit_9_png, 32);  LU_COPY(level_up_small_digit_9_pal, 32);
    LU_COPY(level_up_small_arrow_png, 32);    LU_COPY(level_up_small_arrow_pal, 32);

    /* Red palettes: digits 0-9 */
    LU_COPY(level_up_digit_0_red_pal, 32);
    LU_COPY(level_up_digit_1_red_pal, 32);
    LU_COPY(level_up_digit_2_red_pal, 32);
    LU_COPY(level_up_digit_3_red_pal, 32);
    LU_COPY(level_up_digit_4_red_pal, 32);
    LU_COPY(level_up_digit_5_red_pal, 32);
    LU_COPY(level_up_digit_6_red_pal, 32);
    LU_COPY(level_up_digit_7_red_pal, 32);
    LU_COPY(level_up_digit_8_red_pal, 32);
    LU_COPY(level_up_digit_9_red_pal, 32);

    /* Red palettes: small digits 0-9 + arrow */
    LU_COPY(level_up_small_digit_0_red_pal, 32);
    LU_COPY(level_up_small_digit_1_red_pal, 32);
    LU_COPY(level_up_small_digit_2_red_pal, 32);
    LU_COPY(level_up_small_digit_3_red_pal, 32);
    LU_COPY(level_up_small_digit_4_red_pal, 32);
    LU_COPY(level_up_small_digit_5_red_pal, 32);
    LU_COPY(level_up_small_digit_6_red_pal, 32);
    LU_COPY(level_up_small_digit_7_red_pal, 32);
    LU_COPY(level_up_small_digit_8_red_pal, 32);
    LU_COPY(level_up_small_digit_9_red_pal, 32);
    LU_COPY(level_up_small_arrow_red_pal, 32);

    /* Blue palettes: digits 0-9 */
    LU_COPY(level_up_digit_0_blue_pal, 32);
    LU_COPY(level_up_digit_1_blue_pal, 32);
    LU_COPY(level_up_digit_2_blue_pal, 32);
    LU_COPY(level_up_digit_3_blue_pal, 32);
    LU_COPY(level_up_digit_4_blue_pal, 32);
    LU_COPY(level_up_digit_5_blue_pal, 32);
    LU_COPY(level_up_digit_6_blue_pal, 32);
    LU_COPY(level_up_digit_7_blue_pal, 32);
    LU_COPY(level_up_digit_8_blue_pal, 32);
    LU_COPY(level_up_digit_9_blue_pal, 32);

    /* Blue palettes: small digits 0-9 + arrow */
    LU_COPY(level_up_small_digit_0_blue_pal, 32);
    LU_COPY(level_up_small_digit_1_blue_pal, 32);
    LU_COPY(level_up_small_digit_2_blue_pal, 32);
    LU_COPY(level_up_small_digit_3_blue_pal, 32);
    LU_COPY(level_up_small_digit_4_blue_pal, 32);
    LU_COPY(level_up_small_digit_5_blue_pal, 32);
    LU_COPY(level_up_small_digit_6_blue_pal, 32);
    LU_COPY(level_up_small_digit_7_blue_pal, 32);
    LU_COPY(level_up_small_digit_8_blue_pal, 32);
    LU_COPY(level_up_small_digit_9_blue_pal, 32);
    LU_COPY(level_up_small_arrow_blue_pal, 32);

    /* "Select one to upgrade" banner: 208x16 CI4 = 1664 bytes + 32 pal */
    LU_COPY(level_up_select_one_to_upgrade_png, 1664);
    LU_COPY(level_up_select_one_to_upgrade_pal, 32);

    #undef LU_COPY

    int luUsed = (int)(p - luBuf);
    fprintf(stderr, "[PORT] level_up HUD textures loaded: %d/%d bytes from ROM 0x%X\n",
            luUsed, LU_SEG_SIZE, LU_SEG_ROM);

    /* Load level_up vertex data from ROM and convert N64→PC format */
    {
        /* Vertex data spans ROM 0x79EF58 to 0x79F318 (60 N64 vertices = 960 bytes) */
        #define LU_VTX_ROM  0x79EF58
        #define LU_VTX_SIZE (60 * N64_VTX_SIZE)   /* 8+8+8+8+8+8+8+4 = 60 vtx * 16 = 960 bytes */
        static u8 vtxBuf[LU_VTX_SIZE];
        nuPiReadRom(LU_VTX_ROM, vtxBuf, LU_VTX_SIZE);

        #define LU_VCOPY(dest, off, count) \
            convert_n64_vtx_to_pc(dest, vtxBuf + ((off) - LU_VTX_ROM), count)

        LU_VCOPY(D_802A1018_79EF58, 0x79EF58, 8);  /* draw_second_E */
        LU_VCOPY(D_802A1098_79EFD8, 0x79EFD8, 8);  /* draw_V */
        LU_VCOPY(D_802A1118_79F058, 0x79F058, 8);  /* draw_E */
        LU_VCOPY(D_802A1198_79F0D8, 0x79F0D8, 8);  /* draw_second_L */
        LU_VCOPY(D_802A1218_79F158, 0x79F158, 8);  /* draw_L */
        LU_VCOPY(D_802A1298_79F1D8, 0x79F1D8, 8);  /* draw_P */
        LU_VCOPY(D_802A1318_79F258, 0x79F258, 8);  /* draw_U */
        LU_VCOPY(D_802A1398_79F2D8, 0x79F2D8, 4);  /* draw_exclamation_mark */

        #undef LU_VCOPY
        fprintf(stderr, "[PORT] level_up vertices loaded: 60 vtx from ROM 0x%X\n", LU_VTX_ROM);
    }

    /* Load big letter IA8 textures (64x64 = 4096 bytes each) */
    nuPiReadRom(0x79F358, D_802A1418_79F358, 4096);  /* big_V */
    nuPiReadRom(0x7A03D0, D_802A2490_7A03D0, 4096);  /* big_P */
    nuPiReadRom(0x7A1448, D_802A3508_7A1448, 4096);  /* big_! */
    nuPiReadRom(0x7A24C0, D_802A4580_7A24C0, 4096);  /* big_U */
    nuPiReadRom(0x7A3538, D_802A55F8_7A3538, 4096);  /* big_L */
    nuPiReadRom(0x7A45B0, D_802A6670_7A45B0, 4096);  /* big_E */
    fprintf(stderr, "[PORT] level_up big letter textures loaded: 6 * 4096 bytes\n");
}

/* ===== starpoint overlay textures ===== */
extern u8 D_802A1018_7A89B8[];  /* starpoint.png, 128x32 IA8 = 4096 */
extern u8 D_802A21D8_7A9B78[];  /* starpoints.png, 128x32 IA8 = 4096 */
extern u8 D_802A3340_7AACE0[];  /* digit_0.png, 64x64 IA8 = 4096 */
extern u8 D_802A4460_7ABE00[];  /* digit_1.png */
extern u8 D_802A5580_7ACF20[];  /* digit_2.png */
extern u8 D_802A66A0_7AE040[];  /* digit_3.png */
extern u8 D_802A77C0_7AF160[];  /* digit_4.png */
extern u8 D_802A88E0_7B0280[];  /* digit_5.png */
extern u8 D_802A9A00_7B13A0[];  /* digit_6.png */
extern u8 D_802AAB20_7B24C0[];  /* digit_7.png */
extern u8 D_802ABC40_7B35E0[];  /* digit_8.png */
extern u8 D_802ACD60_7B4700[];  /* digit_9.png */

/* Starpoint vertex arrays */
extern u8 D_802A2090_7A9A30[];  /* starpoint.vtx, 4 vtx */
extern u8 D_802A2198_7A9B38[];  /* starpoints.vtx, 4 vtx */
extern u8 vtx_digit_0[];
extern u8 vtx_digit_1[];
extern u8 vtx_digit_2[];
extern u8 vtx_digit_3[];
extern u8 vtx_digit_4[];
extern u8 vtx_digit_5[];
extern u8 vtx_digit_6[];
extern u8 vtx_digit_7[];
extern u8 vtx_digit_8[];
extern u8 vtx_digit_9[];

static void port_load_starpoint_textures(void) {
    /* Main text textures: starpoint + starpoints (128x32 IA8 = 4096 each) */
    nuPiReadRom(0x7A89B8, D_802A1018_7A89B8, 4096);
    nuPiReadRom(0x7A9B78, D_802A21D8_7A9B78, 4096);
    fprintf(stderr, "[PORT] starpoint text textures loaded\n");

    /* Starpoint/starpoints vertex data */
    {
        u8 vbuf[128];  /* 2 * 4 vtx * 16 bytes = 128 */
        nuPiReadRom(0x7A9A30, vbuf, 64);
        convert_n64_vtx_to_pc(D_802A2090_7A9A30, vbuf, 4);
        nuPiReadRom(0x7A9B38, vbuf, 64);
        convert_n64_vtx_to_pc(D_802A2198_7A9B38, vbuf, 4);
        fprintf(stderr, "[PORT] starpoint text vertices loaded\n");
    }

    /* Digit textures: 64x64 IA8 = 4096 bytes each */
    nuPiReadRom(0x7AACE0, D_802A3340_7AACE0, 4096);  /* digit_0 */
    nuPiReadRom(0x7ABE00, D_802A4460_7ABE00, 4096);  /* digit_1 */
    nuPiReadRom(0x7ACF20, D_802A5580_7ACF20, 4096);  /* digit_2 */
    nuPiReadRom(0x7AE040, D_802A66A0_7AE040, 4096);  /* digit_3 */
    nuPiReadRom(0x7AF160, D_802A77C0_7AF160, 4096);  /* digit_4 */
    nuPiReadRom(0x7B0280, D_802A88E0_7B0280, 4096);  /* digit_5 */
    nuPiReadRom(0x7B13A0, D_802A9A00_7B13A0, 4096);  /* digit_6 */
    nuPiReadRom(0x7B24C0, D_802AAB20_7B24C0, 4096);  /* digit_7 */
    nuPiReadRom(0x7B35E0, D_802ABC40_7B35E0, 4096);  /* digit_8 */
    nuPiReadRom(0x7B4700, D_802ACD60_7B4700, 4096);  /* digit_9 */
    fprintf(stderr, "[PORT] starpoint digit textures loaded\n");

    /* Digit vertex data: 4 vtx each, 10 digits
     * Digit vtx are at ROM offsets just before each digit texture.
     * digit_N vtx = digit_N ROM offset - 0x40 (4 vtx * 16 bytes = 64 bytes)
     */
    {
        u8 dvbuf[64];
        struct { u8* dest; u32 rom; } dvtx[10] = {
            { vtx_digit_0, 0x7AACA0 },
            { vtx_digit_1, 0x7ABDC0 },
            { vtx_digit_2, 0x7ACEE0 },
            { vtx_digit_3, 0x7AE000 },
            { vtx_digit_4, 0x7AF120 },
            { vtx_digit_5, 0x7B0240 },
            { vtx_digit_6, 0x7B1360 },
            { vtx_digit_7, 0x7B2480 },
            { vtx_digit_8, 0x7B35A0 },
            { vtx_digit_9, 0x7B46C0 },
        };
        for (int i = 0; i < 10; i++) {
            nuPiReadRom(dvtx[i].rom, dvbuf, 64);
            convert_n64_vtx_to_pc(dvtx[i].dest, dvbuf, 4);
        }
        fprintf(stderr, "[PORT] starpoint digit vertices loaded: 10 * 4 vtx\n");
    }
}

/* ===== Entity model texture + vertex loading =====
 * Simple entities have compiled C display lists that reference
 * stub arrays (D_0A_* symbols). On N64, DMA fills segment 0x0A
 * with ROM data. On PC, we load directly into the stubs at boot.
 */

/* Entity texture externs (130 stubs in asset_data_stubs.c) */
extern u8 D_0A000000_E4A6A0[];
extern u8 D_0A000000_E992F0[];
extern u8 D_0A000080_E59F80[];
extern u8 D_0A000100_E43C20[];
extern u8 D_0A000120_E3DB80[];
extern u8 D_0A000140_E3DEF0[];
extern u8 D_0A000180_E3B9F0[];
extern u8 D_0A000180_E3C4A0[];
extern u8 D_0A000180_E423C0[];
extern u8 D_0A000180_E428C0[];
extern u8 D_0A000180_E9CA80[];
extern u8 D_0A0001A0_E3DC00[];
extern u8 D_0A000200_E4A8A0[];
extern u8 D_0A000220_E4A8C0[];
extern u8 D_0A000240_E4A8E0[];
extern u8 D_0A000260_E4A900[];
extern u8 D_0A000280_E317B0[];
extern u8 D_0A000280_E31CB0[];
extern u8 D_0A000280_E321B0[];
extern u8 D_0A000280_E625F0[];
extern u8 D_0A000290_E55CD0[];
extern u8 D_0A0002F8_E9CBF8[];
extern u8 D_0A000300_E62670[];
extern u8 D_0A000320_E58FF0[];
extern u8 D_0A000340_E3E0F0[];
extern u8 D_0A000380_E318B0[];
extern u8 D_0A000380_E31DB0[];
extern u8 D_0A000380_E322B0[];
extern u8 D_0A000380_E425C0[];
extern u8 D_0A0003A0_E59070[];
extern u8 D_0A0003C0_E3D190[];
extern u8 D_0A0003C0_E435B0[];
extern u8 D_0A0003D8_E62748[];
extern u8 D_0A000410_E64CE0[];
extern u8 D_0A000420_E45E10[];
extern u8 D_0A000440_E474F0[];
extern u8 D_0A000460_E48BE0[];
extern u8 D_0A000470_E59140[];
extern u8 D_0A000480_E44C80[];
extern u8 D_0A000480_E45590[];
extern u8 D_0A0004D8_E62848[];
extern u8 D_0A000508_E55F48[];
extern u8 D_0A000540_E5D2C0[];
extern u8 D_0A000570_E59240[];
extern u8 D_0A000580_E44D80[];
extern u8 D_0A000580_E45690[];
extern u8 D_0A000580_E59A00[];
extern u8 D_0A0005B0_E9BAB0[];
extern u8 D_0A0005D0_E63090[];
extern u8 D_0A0005D0_E63F90[];
extern u8 D_0A0006D0_E63190[];
extern u8 D_0A0006D0_E64090[];
extern u8 D_0A000700_E5B250[];
extern u8 D_0A000700_E5EE30[];
extern u8 D_0A000700_E60C50[];
extern u8 D_0A000780_E59C00[];
extern u8 D_0A0007E0_E68D60[];
extern u8 D_0A000800_E99AF0[];
extern u8 D_0A000820_E9BD20[];
extern u8 D_0A000840_E9DE90[];
extern u8 D_0A000848_E3D618[];
extern u8 D_0A000860_E66000[];
extern u8 D_0A000880_E5A780[];
extern u8 D_0A000898_E65168[];
extern u8 D_0A0008E0_E68E60[];
extern u8 D_0A000900_E44420[];
extern u8 D_0A000900_E4BBE0[];
extern u8 D_0A000948_E3D718[];
extern u8 D_0A000960_E66100[];
extern u8 D_0A000978_E5F0A8[];
extern u8 D_0A000978_E60EC8[];
extern u8 D_0A000978_E63438[];
extern u8 D_0A000978_E64338[];
extern u8 D_0A000998_E65268[];
extern u8 D_0A000A70_E65340[];
extern u8 D_0A000B00_E5B650[];
extern u8 D_0A000B70_E65440[];
extern u8 D_0A000B88_E69108[];
extern u8 D_0A000BF0_E57A50[];
extern u8 D_0A000C08_E663A8[];
extern u8 D_0A000C20_E46610[];
extern u8 D_0A000C40_E47CF0[];
extern u8 D_0A000C60_E493E0[];
extern u8 D_0A000D08_E56748[];
extern u8 D_0A000D08_E664A8[];
extern u8 D_0A000D40_E5DAC0[];
extern u8 D_0A000D40_E6E0D0[];
extern u8 D_0A000DA8_E5B8F8[];
extern u8 D_0A000DD0_E56810[];
extern u8 D_0A000E00_E69380[];
extern u8 D_0A000ED0_E56910[];
extern u8 D_0A000FB0_E66750[];
extern u8 D_0A000FE8_E5DD68[];
extern u8 D_0A000FF0_E57E50[];
extern u8 D_0A001040_E9E690[];
extern u8 D_0A001050_E398E0[];
extern u8 D_0A001060_E33480[];
extern u8 D_0A001060_E366D0[];
extern u8 D_0A001090_E6C240[];
extern u8 D_0A001150_E399E0[];
extern u8 D_0A001170_E4C450[];
extern u8 D_0A001178_E5F8A8[];
extern u8 D_0A001178_E616C8[];
extern u8 D_0A001200_E69780[];
extern u8 D_0A001260_E33680[];
extern u8 D_0A0013B0_E66B50[];
extern u8 D_0A0013E8_E5E168[];
extern u8 D_0A001400_E4FBF0[];
extern u8 D_0A001420_E5FB50[];
extern u8 D_0A001420_E61970[];
extern u8 D_0A0014A0_E58300[];
extern u8 D_0A001560_E52BA0[];
extern u8 D_0A0015A8_E5C0F8[];
extern u8 D_0A001820_E5FF50[];
extern u8 D_0A001820_E61D70[];
extern u8 D_0A001850_E5C3A0[];
extern u8 D_0A0018A0_E58700[];
extern u8 D_0A001AA0_E3FD00[];
extern u8 D_0A001C50_E5C7A0[];
extern u8 D_0A001D60_E533A0[];
extern u8 D_0A001D70_E8A210[];
extern u8 D_0A001E28_E53468[];
extern u8 D_0A0021E8_E4D4C8[];
extern u8 D_0A002628_E53C68[];
extern u8 D_0A005EE8_E74548[];
extern u8 D_0A005EE8_E7AAB8[];
extern u8 D_0A005EE8_E80FC8[];
extern u8 D_0A006840_E87E30[];
extern u8 D_0A006840_E91220[];
extern u8 D_0A006840_E980D0[];

/* Entity vertex externs (133 stubs in include/entity/model/ .vtx.inc.c) */
extern u8 D_0A000000_E6B1B0[];
extern u8 D_0A000390_E6B540[];
extern u8 D_0A000640_E6B7F0[];
extern u8 D_0A000720_E6B8D0[];
extern u8 D_0A000A10_E6BBC0[];
extern u8 D_0A000000_E639C0[];
extern u8 D_0A000300_E63CC0[];
extern u8 D_0A000390_E63D50[];
extern u8 D_0A0003D0_E63D90[];
extern u8 D_0A000000_E58CD0[];
extern u8 D_0A000060_E58D30[];
extern u8 D_0A0002E0_E58FB0[];
extern u8 D_0A000000_E4E7F0[];
extern u8 D_0A0001E0_E4E9D0[];
extern u8 D_0A0003C0_E4EBB0[];
extern u8 D_0A0005A0_E4ED90[];
extern u8 D_0A000780_E4EF70[];
extern u8 D_0A000900_E4F0F0[];
extern u8 D_0A000A80_E4F270[];
extern u8 D_0A000C80_E4F470[];
extern u8 D_0A000E80_E4F670[];
extern u8 D_0A001000_E4F7F0[];
extern u8 D_0A001180_E4F970[];
extern u8 D_0A001280_E4FA70[];
extern u8 D_0A000000_E9D650[];
extern u8 D_0A000380_E9D9D0[];
extern u8 D_0A0004A0_E9DAF0[];
extern u8 D_0A000600_E9DC50[];
extern u8 D_0A000720_E9DD70[];
extern u8 D_0A000000_E3DDB0[];
extern u8 D_0A000000_E648D0[];
extern u8 D_0A000190_E64A60[];
extern u8 D_0A000210_E64AE0[];
extern u8 D_0A000390_E64C60[];
extern u8 D_0A0008C8_E99BB8[];
extern u8 D_0A000968_E99C58[];
extern u8 D_0A0009C8_E99CB8[];
extern u8 D_0A000A68_E99D58[];
extern u8 D_0A000BB8_E99EA8[];
extern u8 D_0A000D28_E9A018[];
extern u8 D_0A000DB8_E9A0A8[];
extern u8 D_0A000F28_E9A218[];
extern u8 D_0A000000_E62370[];
extern u8 D_0A000080_E623F0[];
extern u8 D_0A000000_E31530[];
extern u8 D_0A000000_E31A30[];
extern u8 D_0A000000_E31F30[];
extern u8 D_0A000000_E431F0[];
extern u8 D_0A000000_E470B0[];
extern u8 D_0A000160_E47210[];
extern u8 D_0A000000_E48780[];
extern u8 D_0A000160_E488E0[];
extern u8 D_0A000000_E459F0[];
extern u8 D_0A000160_E45B50[];
extern u8 D_0A000000_E42740[];
extern u8 D_0A000100_E42840[];
extern u8 D_0A000000_E42240[];
extern u8 D_0A000100_E42340[];
extern u8 D_0A000000_E815F0[];
extern u8 D_0A000000_E884A0[];
extern u8 D_0A000000_E7B0E0[];
extern u8 D_0A000000_E74BD0[];
extern u8 D_0A000000_E6E660[];
extern u8 D_0A000000_E91890[];
extern u8 D_0A000000_E8A9E0[];
extern u8 D_0A000000_E5AB50[];
extern u8 D_0A0005C0_E5B110[];
extern u8 D_0A000620_E5B170[];
extern u8 D_0A000660_E5B1B0[];
extern u8 D_0A000000_E60550[];
extern u8 D_0A0005C0_E60B10[];
extern u8 D_0A000620_E60B70[];
extern u8 D_0A000660_E60BB0[];
extern u8 D_0A000000_E5E730[];
extern u8 D_0A0005C0_E5ECF0[];
extern u8 D_0A000620_E5ED50[];
extern u8 D_0A000660_E5ED90[];
extern u8 D_0A000000_E5CD80[];
extern u8 D_0A000200_E5CF80[];
extern u8 D_0A000240_E5CFC0[];
extern u8 D_0A000280_E5D000[];
extern u8 D_0A000000_E9B500[];
extern u8 D_0A000040_E9B540[];
extern u8 D_0A000290_E9B790[];
extern u8 D_0A0003B0_E9B8B0[];
extern u8 D_0A0004B0_E9B9B0[];
extern u8 D_0A000000_E43B20[];
extern u8 D_0A000000_E3DA60[];
extern u8 D_0A000000_E3C320[];
extern u8 D_0A000000_E62AC0[];
extern u8 D_0A000390_E62E50[];
extern u8 D_0A0003D0_E62E90[];
extern u8 D_0A000000_E4B2E0[];
extern u8 D_0A000370_E4B650[];
extern u8 D_0A0006C0_E4B9A0[];
extern u8 D_0A000000_E657A0[];
extern u8 D_0A000280_E65A20[];
extern u8 D_0A000300_E65AA0[];
extern u8 D_0A000380_E65B20[];
extern u8 D_0A000400_E65BA0[];
extern u8 D_0A000680_E65E20[];
extern u8 D_0A0007E0_E65F80[];
extern u8 D_0A000000_E3E260[];
extern u8 D_0A000000_E32420[];
extern u8 D_0A000000_E35670[];
extern u8 D_0A000000_E38890[];
extern u8 D_0A000000_E59480[];
extern u8 D_0A000120_E595A0[];
extern u8 D_0A000000_E68580[];
extern u8 D_0A000280_E68800[];
extern u8 D_0A000300_E68880[];
extern u8 D_0A000380_E68900[];
extern u8 D_0A000600_E68B80[];
extern u8 D_0A000760_E68CE0[];
extern u8 D_0A000000_E9C900[];
extern u8 D_0A000050_E9C950[];
extern u8 D_0A0000A0_E9C9A0[];
extern u8 D_0A000120_E9CA20[];
extern u8 D_0A000000_E55A40[];
extern u8 D_0A0000F0_E55B30[];
extern u8 D_0A000210_E55C50[];
extern u8 D_0A000250_E55C90[];
extern u8 D_0A000000_E44800[];
extern u8 D_0A000000_E6D390[];
extern u8 D_0A000240_E6D5D0[];
extern u8 D_0A000000_E56E60[];
extern u8 D_0A000410_E57270[];
extern u8 D_0A000820_E57680[];
extern u8 D_0A000860_E576C0[];
extern u8 D_0A000000_E45110[];
extern u8 D_0A000000_E3CDD0[];
extern u8 D_0A000000_E51640[];
extern u8 D_0A000000_E3B870[];

static void port_load_entity_model_data(void) {
    typedef struct { u8* dest; u32 rom; u32 size; } EntityTexEntry;
    typedef struct { u8* dest; u32 rom; u32 count; } EntityVtxEntry;

    /* --- 130 entity texture stubs --- */
    static EntityTexEntry tex_entries[] = {
    { D_0A000000_E4A6A0, 0xE4A6A0, 512 },
    { D_0A000000_E992F0, 0xE992F0, 2048 },
    { D_0A000080_E59F80, 0xE59F80, 2048 },
    { D_0A000100_E43C20, 0xE43C20, 2048 },
    { D_0A000120_E3DB80, 0xE3DB80, 128 },
    { D_0A000140_E3DEF0, 0xE3DEF0, 512 },
    { D_0A000180_E3B9F0, 0xE3B9F0, 2048 },
    { D_0A000180_E3C4A0, 0xE3C4A0, 2048 },
    { D_0A000180_E423C0, 0xE423C0, 512 },
    { D_0A000180_E428C0, 0xE428C0, 2048 },
    { D_0A000180_E9CA80, 0xE9CA80, 256 },
    { D_0A0001A0_E3DC00, 0xE3DC00, 32 },
    { D_0A000200_E4A8A0, 0xE4A8A0, 32 },
    { D_0A000220_E4A8C0, 0xE4A8C0, 32 },
    { D_0A000240_E4A8E0, 0xE4A8E0, 32 },
    { D_0A000260_E4A900, 0xE4A900, 32 },
    { D_0A000280_E317B0, 0xE317B0, 256 },
    { D_0A000280_E31CB0, 0xE31CB0, 256 },
    { D_0A000280_E321B0, 0xE321B0, 256 },
    { D_0A000280_E625F0, 0xE625F0, 128 },
    { D_0A000290_E55CD0, 0xE55CD0, 512 },
    { D_0A0002F8_E9CBF8, 0xE9CBF8, 2048 },
    { D_0A000300_E62670, 0xE62670, 32 },
    { D_0A000320_E58FF0, 0xE58FF0, 128 },
    { D_0A000340_E3E0F0, 0xE3E0F0, 32 },
    { D_0A000380_E318B0, 0xE318B0, 32 },
    { D_0A000380_E31DB0, 0xE31DB0, 32 },
    { D_0A000380_E322B0, 0xE322B0, 32 },
    { D_0A000380_E425C0, 0xE425C0, 32 },
    { D_0A0003A0_E59070, 0xE59070, 32 },
    { D_0A0003C0_E3D190, 0xE3D190, 1024 },
    { D_0A0003C0_E435B0, 0xE435B0, 1024 },
    { D_0A0003D8_E62748, 0xE62748, 256 },
    { D_0A000410_E64CE0, 0xE64CE0, 1024 },
    { D_0A000420_E45E10, 0xE45E10, 2048 },
    { D_0A000440_E474F0, 0xE474F0, 2048 },
    { D_0A000460_E48BE0, 0xE48BE0, 2048 },
    { D_0A000470_E59140, 0xE59140, 256 },
    { D_0A000480_E44C80, 0xE44C80, 256 },
    { D_0A000480_E45590, 0xE45590, 256 },
    { D_0A0004D8_E62848, 0xE62848, 32 },
    { D_0A000508_E55F48, 0xE55F48, 2048 },
    { D_0A000540_E5D2C0, 0xE5D2C0, 2048 },
    { D_0A000570_E59240, 0xE59240, 32 },
    { D_0A000580_E44D80, 0xE44D80, 32 },
    { D_0A000580_E45690, 0xE45690, 32 },
    { D_0A000580_E59A00, 0xE59A00, 512 },
    { D_0A0005B0_E9BAB0, 0xE9BAB0, 512 },
    { D_0A0005D0_E63090, 0xE63090, 256 },
    { D_0A0005D0_E63F90, 0xE63F90, 256 },
    { D_0A0006D0_E63190, 0xE63190, 512 },
    { D_0A0006D0_E64090, 0xE64090, 512 },
    { D_0A000700_E5B250, 0xE5B250, 1024 },
    { D_0A000700_E5EE30, 0xE5EE30, 512 },
    { D_0A000700_E60C50, 0xE60C50, 512 },
    { D_0A000780_E59C00, 0xE59C00, 32 },
    { D_0A0007E0_E68D60, 0xE68D60, 256 },
    { D_0A000800_E99AF0, 0xE99AF0, 32 },
    { D_0A000820_E9BD20, 0xE9BD20, 2048 },
    { D_0A000840_E9DE90, 0xE9DE90, 2048 },
    { D_0A000848_E3D618, 0xE3D618, 256 },
    { D_0A000860_E66000, 0xE66000, 256 },
    { D_0A000880_E5A780, 0xE5A780, 512 },
    { D_0A000898_E65168, 0xE65168, 256 },
    { D_0A0008E0_E68E60, 0xE68E60, 512 },
    { D_0A000900_E44420, 0xE44420, 512 },
    { D_0A000900_E4BBE0, 0xE4BBE0, 2048 },
    { D_0A000948_E3D718, 0xE3D718, 32 },
    { D_0A000960_E66100, 0xE66100, 512 },
    { D_0A000978_E5F0A8, 0xE5F0A8, 2048 },
    { D_0A000978_E60EC8, 0xE60EC8, 2048 },
    { D_0A000978_E63438, 0xE63438, 256 },
    { D_0A000978_E64338, 0xE64338, 256 },
    { D_0A000998_E65268, 0xE65268, 32 },
    { D_0A000A70_E65340, 0xE65340, 256 },
    { D_0A000B00_E5B650, 0xE5B650, 512 },
    { D_0A000B70_E65440, 0xE65440, 32 },
    { D_0A000B88_E69108, 0xE69108, 512 },
    { D_0A000BF0_E57A50, 0xE57A50, 1024 },
    { D_0A000C08_E663A8, 0xE663A8, 256 },
    { D_0A000C20_E46610, 0xE46610, 2048 },
    { D_0A000C40_E47CF0, 0xE47CF0, 2048 },
    { D_0A000C60_E493E0, 0xE493E0, 2048 },
    { D_0A000D08_E56748, 0xE56748, 32 },
    { D_0A000D08_E664A8, 0xE664A8, 512 },
    { D_0A000D40_E5DAC0, 0xE5DAC0, 512 },
    { D_0A000D40_E6E0D0, 0xE6E0D0, 512 },
    { D_0A000DA8_E5B8F8, 0xE5B8F8, 2048 },
    { D_0A000DD0_E56810, 0xE56810, 256 },
    { D_0A000E00_E69380, 0xE69380, 1024 },
    { D_0A000ED0_E56910, 0xE56910, 32 },
    { D_0A000FB0_E66750, 0xE66750, 1024 },
    { D_0A000FE8_E5DD68, 0xE5DD68, 1024 },
    { D_0A000FF0_E57E50, 0xE57E50, 1024 },
    { D_0A001040_E9E690, 0xE9E690, 32 },
    { D_0A001050_E398E0, 0xE398E0, 256 },
    { D_0A001060_E33480, 0xE33480, 512 },
    { D_0A001060_E366D0, 0xE366D0, 1024 },
    { D_0A001090_E6C240, 0xE6C240, 512 },
    { D_0A001150_E399E0, 0xE399E0, 32 },
    { D_0A001170_E4C450, 0xE4C450, 4096 },
    { D_0A001178_E5F8A8, 0xE5F8A8, 512 },
    { D_0A001178_E616C8, 0xE616C8, 512 },
    { D_0A001200_E69780, 0xE69780, 512 },
    { D_0A001260_E33680, 0xE33680, 512 },
    { D_0A0013B0_E66B50, 0xE66B50, 512 },
    { D_0A0013E8_E5E168, 0xE5E168, 512 },
    { D_0A001400_E4FBF0, 0xE4FBF0, 4096 },
    { D_0A001420_E5FB50, 0xE5FB50, 1024 },
    { D_0A001420_E61970, 0xE61970, 1024 },
    { D_0A0014A0_E58300, 0xE58300, 1024 },
    { D_0A001560_E52BA0, 0xE52BA0, 2048 },
    { D_0A0015A8_E5C0F8, 0xE5C0F8, 512 },
    { D_0A001820_E5FF50, 0xE5FF50, 512 },
    { D_0A001820_E61D70, 0xE61D70, 512 },
    { D_0A001850_E5C3A0, 0xE5C3A0, 1024 },
    { D_0A0018A0_E58700, 0xE58700, 32 },
    { D_0A001AA0_E3FD00, 0xE3FD00, 2048 },
    { D_0A001C50_E5C7A0, 0xE5C7A0, 512 },
    { D_0A001D60_E533A0, 0xE533A0, 32 },
    { D_0A001D70_E8A210, 0xE8A210, 512 },
    { D_0A001E28_E53468, 0xE53468, 2048 },
    { D_0A0021E8_E4D4C8, 0xE4D4C8, 4096 },
    { D_0A002628_E53C68, 0xE53C68, 32 },
    { D_0A005EE8_E74548, 0xE74548, 512 },
    { D_0A005EE8_E7AAB8, 0xE7AAB8, 512 },
    { D_0A005EE8_E80FC8, 0xE80FC8, 512 },
    { D_0A006840_E87E30, 0xE87E30, 512 },
    { D_0A006840_E91220, 0xE91220, 512 },
    { D_0A006840_E980D0, 0xE980D0, 512 },
    };

    for (int i = 0; i < (int)(sizeof(tex_entries)/sizeof(tex_entries[0])); i++) {
        nuPiReadRom(tex_entries[i].rom, tex_entries[i].dest, tex_entries[i].size);
    }
    fprintf(stderr, "[PORT] Entity model textures loaded: %d entries\n",
            (int)(sizeof(tex_entries)/sizeof(tex_entries[0])));

    /* --- 133 entity vertex stubs --- */
    static EntityVtxEntry vtx_entries[] = {
    { (u8*)D_0A000000_E6B1B0, 0xE6B1B0, 57 },
    { (u8*)D_0A000390_E6B540, 0xE6B540, 43 },
    { (u8*)D_0A000640_E6B7F0, 0xE6B7F0, 14 },
    { (u8*)D_0A000720_E6B8D0, 0xE6B8D0, 47 },
    { (u8*)D_0A000A10_E6BBC0, 0xE6BBC0, 14 },
    { (u8*)D_0A000000_E639C0, 0xE639C0, 48 },
    { (u8*)D_0A000300_E63CC0, 0xE63CC0, 9 },
    { (u8*)D_0A000390_E63D50, 0xE63D50, 4 },
    { (u8*)D_0A0003D0_E63D90, 0xE63D90, 32 },
    { (u8*)D_0A000000_E58CD0, 0xE58CD0, 6 },
    { (u8*)D_0A000060_E58D30, 0xE58D30, 40 },
    { (u8*)D_0A0002E0_E58FB0, 0xE58FB0, 4 },
    { (u8*)D_0A000000_E4E7F0, 0xE4E7F0, 30 },
    { (u8*)D_0A0001E0_E4E9D0, 0xE4E9D0, 30 },
    { (u8*)D_0A0003C0_E4EBB0, 0xE4EBB0, 30 },
    { (u8*)D_0A0005A0_E4ED90, 0xE4ED90, 30 },
    { (u8*)D_0A000780_E4EF70, 0xE4EF70, 24 },
    { (u8*)D_0A000900_E4F0F0, 0xE4F0F0, 24 },
    { (u8*)D_0A000A80_E4F270, 0xE4F270, 32 },
    { (u8*)D_0A000C80_E4F470, 0xE4F470, 32 },
    { (u8*)D_0A000E80_E4F670, 0xE4F670, 24 },
    { (u8*)D_0A001000_E4F7F0, 0xE4F7F0, 24 },
    { (u8*)D_0A001180_E4F970, 0xE4F970, 16 },
    { (u8*)D_0A001280_E4FA70, 0xE4FA70, 24 },
    { (u8*)D_0A000000_E9D650, 0xE9D650, 56 },
    { (u8*)D_0A000380_E9D9D0, 0xE9D9D0, 18 },
    { (u8*)D_0A0004A0_E9DAF0, 0xE9DAF0, 22 },
    { (u8*)D_0A000600_E9DC50, 0xE9DC50, 18 },
    { (u8*)D_0A000720_E9DD70, 0xE9DD70, 18 },
    { (u8*)D_0A000000_E3DDB0, 0xE3DDB0, 20 },
    { (u8*)D_0A000000_E648D0, 0xE648D0, 25 },
    { (u8*)D_0A000190_E64A60, 0xE64A60, 8 },
    { (u8*)D_0A000210_E64AE0, 0xE64AE0, 24 },
    { (u8*)D_0A000390_E64C60, 0xE64C60, 8 },
    { (u8*)D_0A0008C8_E99BB8, 0xE99BB8, 10 },
    { (u8*)D_0A000968_E99C58, 0xE99C58, 6 },
    { (u8*)D_0A0009C8_E99CB8, 0xE99CB8, 10 },
    { (u8*)D_0A000A68_E99D58, 0xE99D58, 21 },
    { (u8*)D_0A000BB8_E99EA8, 0xE99EA8, 23 },
    { (u8*)D_0A000D28_E9A018, 0xE9A018, 9 },
    { (u8*)D_0A000DB8_E9A0A8, 0xE9A0A8, 23 },
    { (u8*)D_0A000F28_E9A218, 0xE9A218, 21 },
    { (u8*)D_0A000000_E62370, 0xE62370, 8 },
    { (u8*)D_0A000080_E623F0, 0xE623F0, 32 },
    { (u8*)D_0A000000_E31530, 0xE31530, 40 },
    { (u8*)D_0A000000_E31A30, 0xE31A30, 40 },
    { (u8*)D_0A000000_E31F30, 0xE31F30, 40 },
    { (u8*)D_0A000000_E431F0, 0xE431F0, 60 },
    { (u8*)D_0A000000_E470B0, 0xE470B0, 22 },
    { (u8*)D_0A000160_E47210, 0xE47210, 24 },
    { (u8*)D_0A000000_E48780, 0xE48780, 22 },
    { (u8*)D_0A000160_E488E0, 0xE488E0, 22 },
    { (u8*)D_0A000000_E459F0, 0xE459F0, 22 },
    { (u8*)D_0A000160_E45B50, 0xE45B50, 22 },
    { (u8*)D_0A000000_E42740, 0xE42740, 16 },
    { (u8*)D_0A000100_E42840, 0xE42840, 8 },
    { (u8*)D_0A000000_E42240, 0xE42240, 16 },
    { (u8*)D_0A000100_E42340, 0xE42340, 8 },
    { (u8*)D_0A000000_E815F0, 0xE815F0, 32 },
    { (u8*)D_0A000000_E884A0, 0xE884A0, 399 },
    { (u8*)D_0A000000_E7B0E0, 0xE7B0E0, 32 },
    { (u8*)D_0A000000_E74BD0, 0xE74BD0, 32 },
    { (u8*)D_0A000000_E6E660, 0xE6E660, 32 },
    { (u8*)D_0A000000_E91890, 0xE91890, 32 },
    { (u8*)D_0A000000_E8A9E0, 0xE8A9E0, 32 },
    { (u8*)D_0A000000_E5AB50, 0xE5AB50, 92 },
    { (u8*)D_0A0005C0_E5B110, 0xE5B110, 6 },
    { (u8*)D_0A000620_E5B170, 0xE5B170, 4 },
    { (u8*)D_0A000660_E5B1B0, 0xE5B1B0, 10 },
    { (u8*)D_0A000000_E60550, 0xE60550, 92 },
    { (u8*)D_0A0005C0_E60B10, 0xE60B10, 6 },
    { (u8*)D_0A000620_E60B70, 0xE60B70, 4 },
    { (u8*)D_0A000660_E60BB0, 0xE60BB0, 10 },
    { (u8*)D_0A000000_E5E730, 0xE5E730, 92 },
    { (u8*)D_0A0005C0_E5ECF0, 0xE5ECF0, 6 },
    { (u8*)D_0A000620_E5ED50, 0xE5ED50, 4 },
    { (u8*)D_0A000660_E5ED90, 0xE5ED90, 10 },
    { (u8*)D_0A000000_E5CD80, 0xE5CD80, 32 },
    { (u8*)D_0A000200_E5CF80, 0xE5CF80, 4 },
    { (u8*)D_0A000240_E5CFC0, 0xE5CFC0, 4 },
    { (u8*)D_0A000280_E5D000, 0xE5D000, 44 },
    { (u8*)D_0A000000_E9B500, 0xE9B500, 4 },
    { (u8*)D_0A000040_E9B540, 0xE9B540, 37 },
    { (u8*)D_0A000290_E9B790, 0xE9B790, 18 },
    { (u8*)D_0A0003B0_E9B8B0, 0xE9B8B0, 16 },
    { (u8*)D_0A0004B0_E9B9B0, 0xE9B9B0, 16 },
    { (u8*)D_0A000000_E43B20, 0xE43B20, 16 },
    { (u8*)D_0A000000_E3DA60, 0xE3DA60, 18 },
    { (u8*)D_0A000000_E3C320, 0xE3C320, 24 },
    { (u8*)D_0A000000_E62AC0, 0xE62AC0, 57 },
    { (u8*)D_0A000390_E62E50, 0xE62E50, 4 },
    { (u8*)D_0A0003D0_E62E90, 0xE62E90, 32 },
    { (u8*)D_0A000000_E4B2E0, 0xE4B2E0, 20 },
    { (u8*)D_0A000370_E4B650, 0xE4B650, 53 },
    { (u8*)D_0A0006C0_E4B9A0, 0xE4B9A0, 36 },
    { (u8*)D_0A000000_E657A0, 0xE657A0, 40 },
    { (u8*)D_0A000280_E65A20, 0xE65A20, 8 },
    { (u8*)D_0A000300_E65AA0, 0xE65AA0, 8 },
    { (u8*)D_0A000380_E65B20, 0xE65B20, 8 },
    { (u8*)D_0A000400_E65BA0, 0xE65BA0, 40 },
    { (u8*)D_0A000680_E65E20, 0xE65E20, 22 },
    { (u8*)D_0A0007E0_E65F80, 0xE65F80, 8 },
    { (u8*)D_0A000000_E3E260, 0xE3E260, 426 },
    { (u8*)D_0A000000_E32420, 0xE32420, 262 },
    { (u8*)D_0A000000_E35670, 0xE35670, 262 },
    { (u8*)D_0A000000_E38890, 0xE38890, 261 },
    { (u8*)D_0A000000_E59480, 0xE59480, 18 },
    { (u8*)D_0A000120_E595A0, 0xE595A0, 70 },
    { (u8*)D_0A000000_E68580, 0xE68580, 40 },
    { (u8*)D_0A000280_E68800, 0xE68800, 8 },
    { (u8*)D_0A000300_E68880, 0xE68880, 8 },
    { (u8*)D_0A000380_E68900, 0xE68900, 40 },
    { (u8*)D_0A000600_E68B80, 0xE68B80, 22 },
    { (u8*)D_0A000760_E68CE0, 0xE68CE0, 8 },
    { (u8*)D_0A000000_E9C900, 0xE9C900, 5 },
    { (u8*)D_0A000050_E9C950, 0xE9C950, 5 },
    { (u8*)D_0A0000A0_E9C9A0, 0xE9C9A0, 8 },
    { (u8*)D_0A000120_E9CA20, 0xE9CA20, 6 },
    { (u8*)D_0A000000_E55A40, 0xE55A40, 15 },
    { (u8*)D_0A0000F0_E55B30, 0xE55B30, 18 },
    { (u8*)D_0A000210_E55C50, 0xE55C50, 4 },
    { (u8*)D_0A000250_E55C90, 0xE55C90, 4 },
    { (u8*)D_0A000000_E44800, 0xE44800, 72 },
    { (u8*)D_0A000000_E6D390, 0xE6D390, 36 },
    { (u8*)D_0A000240_E6D5D0, 0xE6D5D0, 119 },
    { (u8*)D_0A000000_E56E60, 0xE56E60, 65 },
    { (u8*)D_0A000410_E57270, 0xE57270, 65 },
    { (u8*)D_0A000820_E57680, 0xE57680, 4 },
    { (u8*)D_0A000860_E576C0, 0xE576C0, 57 },
    { (u8*)D_0A000000_E45110, 0xE45110, 72 },
    { (u8*)D_0A000000_E3CDD0, 0xE3CDD0, 60 },
    { (u8*)D_0A000000_E51640, 0xE51640, 278 },
    { (u8*)D_0A000000_E3B870, 0xE3B870, 24 },
    };

    /* Largest vertex count is 426, N64 vtx = 16 bytes each */
    u8 vtx_buf[426 * 16];
    for (int i = 0; i < (int)(sizeof(vtx_entries)/sizeof(vtx_entries[0])); i++) {
        u32 n64_size = vtx_entries[i].count * 16;
        nuPiReadRom(vtx_entries[i].rom, vtx_buf, n64_size);
        convert_n64_vtx_to_pc(vtx_entries[i].dest, vtx_buf, vtx_entries[i].count);
    }
    fprintf(stderr, "[PORT] Entity model vertices loaded: %d entries\n",
            (int)(sizeof(vtx_entries)/sizeof(vtx_entries[0])));
}

/* ===== Map texture loading (GFX display list textures) ===== */

/* dgb_01 bridge textures */
extern u8 dgb_01_img0_png[];
extern u8 dgb_01_img1_png[];
extern u8 dgb_01_img2_png[];
extern u8 dgb_01_img3_png[];
extern u8 dgb_01_img4_png[];
extern u8 dgb_01_img5_png[];
extern u8 dgb_01_img6_png[];
extern u8 dgb_01_img7_png[];

/* dro_02 Merlee card */
extern u8 dro_02_card[];
extern u8 dro_02_card_pal[];

/* pra_31 staircase textures */
extern u8 pra_31_texture_1_img[];
extern u8 pra_31_texture_1_pal[];
extern u8 pra_31_texture_2_img[];
extern u8 pra_31_texture_2_pal[];
extern u8 pra_31_texture_3_img[];

/* Toad House blanket (12 maps) */
extern u8 jan_03_toad_house_blanket_img[];
extern u8 dro_02_toad_house_blanket_img[];
extern u8 kmr_02_toad_house_blanket_img[];
extern u8 kmr_20_toad_house_blanket_img[];
extern u8 hos_03_toad_house_blanket_img[];
extern u8 mac_01_toad_house_blanket_img[];
extern u8 sam_06_toad_house_blanket_img[];
extern u8 sam_02_toad_house_blanket_img[];
extern u8 kkj_20_toad_house_blanket_img[];
extern u8 kpa_95_toad_house_blanket_img[];
extern u8 kpa_91_toad_house_blanket_img[];
extern u8 nok_01_toad_house_blanket_img[];

void port_load_map_textures(void) {
    fprintf(stderr, "[PORT] Loading map textures from ROM...\n");

    /* dgb_01 bridge textures (RGBA16) */
    nuPiReadRom(0xC047F0, dgb_01_img0_png, 4096);
    nuPiReadRom(0xC05870, dgb_01_img1_png, 4096);
    nuPiReadRom(0xC068F0, dgb_01_img2_png, 2048);
    nuPiReadRom(0xC07170, dgb_01_img3_png, 1024);
    nuPiReadRom(0xC075F0, dgb_01_img4_png, 1024);
    nuPiReadRom(0xC07A70, dgb_01_img5_png, 1024);
    nuPiReadRom(0xC07EF0, dgb_01_img6_png, 2048);
    nuPiReadRom(0xC08770, dgb_01_img7_png, 2048);

    /* dro_02 Merlee fortune card (CI4 32x32 + palette) */
    nuPiReadRom(0x9707A8, dro_02_card, 512);
    nuPiReadRom(0x9709A8, dro_02_card_pal, 32);

    /* pra_31 Crystal Palace staircase textures */
    nuPiReadRom(0xD82270, pra_31_texture_1_img, 2048);
    nuPiReadRom(0xD82A70, pra_31_texture_1_pal, 32);
    nuPiReadRom(0xD82B50, pra_31_texture_2_img, 2048);
    nuPiReadRom(0xD83350, pra_31_texture_2_pal, 32);
    nuPiReadRom(0xD83420, pra_31_texture_3_img, 2048);

    /* Toad House blanket (RGBA16 16x32 = 1024 bytes, same data for all maps) */
    nuPiReadRom(0x80D558, jan_03_toad_house_blanket_img, 1024);
    /* Copy to all 11 other map variants (identical texture data) */
    memcpy(dro_02_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(kmr_02_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(kmr_20_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(hos_03_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(mac_01_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(sam_06_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(sam_02_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(kkj_20_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(kpa_95_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(kpa_91_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);
    memcpy(nok_01_toad_house_blanket_img, jan_03_toad_house_blanket_img, 1024);

    fprintf(stderr, "[PORT] Map textures loaded: dgb_01(8), dro_02(2), pra_31(5), blanket(12)\n");
}
