#pragma once
/*
 * .o2r mod support: PaperBoat-format text, font and texture mods placed in
 * ux0:data/papership/mods/. Anything a mod does not provide comes from the ROM.
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Generated table entry (port/mod_asset_table.c): ROM offset -> PaperBoat resource path. */
typedef struct PortModAsset {
    uint32_t rom;
    uint32_t size;
    uint8_t kind; /* PORT_MOD_KIND_* */
    const char* path;
} PortModAsset;

#define PORT_MOD_KIND_TEXTURE 0
#define PORT_MOD_KIND_BLOB 1

extern const PortModAsset gPortModAssets[];
extern const unsigned int gPortModAssetCount;

/* "Intro_0001" etc. for a message ID, or NULL (port/mod_msg_names.c, generated). */
const char* port_mod_msg_suffix(unsigned int msgID);

/* Fill dest (cap bytes) with a mod's text for msgID. Returns 1 if a mod supplied it. */
int Port_ModLoadMessage(unsigned int msgID, void* dest, unsigned int cap);

/* Called after ROM bytes [rom, rom+size) land at dest: registers textures for replacement
 * and applies font overrides. */
void Port_ModOnRomRead(uint32_t rom, void* dest, uint32_t size);

/* Map textures come from compressed archives, so they are registered by name. */
void Port_ModSetTexArchive(const char* archiveName);
void Port_ModRegisterMapTexture(const void* raster, uint32_t size, const char* name, int aux);

#ifdef __cplusplus
}

#include <memory>
namespace Fast {
class Texture;
}
/* Replacement texture for a raw texture pointer, or nullptr. Used by the Fast3D SETTIMG handler. */
std::shared_ptr<Fast::Texture> Port_ModTextureFor(const void* addr);
#endif
