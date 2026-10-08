/*
 * ModAssets.cpp - PaperBoat-format .o2r mods: text, fonts and textures.
 *
 * Engine.cpp adds every .o2r/.otr in mods/ to the archive manager. PaperBoat mods name their
 * assets by resource path, while this port reads the same assets from ROM offsets, so the
 * generated tables map one onto the other:
 *   - text:     message ID -> "messages/MSG_<name>" blob
 *   - fonts:    charset ROM offset -> "charset/<name>" blob, copied over the ROM bytes
 *   - textures: the pointer a ROM texture was loaded to -> its resource path; the Fast3D
 *               SETTIMG handler asks for a replacement and draws the mod's texture instead
 */
#include "mod_assets.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <ship/resource/type/Blob.h>
#include "fast/resource/type/Texture.h"

extern std::thread::id gPortMainThreadId;
extern "C" void gfx_texture_cache_clear(void);

namespace {

constexpr uint8_t kMsgEnd = 0xFD;
// HD textures above this many texels are halved until they fit; Vita memory is tight
constexpr uint32_t kMaxTexels = 256 * 256;
// bytes hashed to confirm a registered buffer still holds the texture it was registered with
constexpr uint32_t kSigBytes = 64;

struct Registered {
    uint32_t size;
    uint32_t sig;
    uint32_t rowBytes; // nonzero: draws may start at any whole row inside the image (backgrounds)
    std::string path;
};

bool sInitDone = false;
bool sHasMessages = false;
bool sHasTextures = false;
bool sHasBlobs = false;
std::unordered_set<std::string> sModFiles;
std::map<uintptr_t, Registered> sRegistry;
std::unordered_map<std::string, std::shared_ptr<Fast::Texture>> sTextures;
std::unordered_set<std::string> sMissing;
// replaced textures stay resident until this many bytes, then the set is dropped and reloaded on demand
constexpr size_t kTextureBudget = 48 * 1024 * 1024;
size_t sTextureBytes = 0;
int sReplacedLogged = 0;
char sTexArchive[32];

std::shared_ptr<Ship::ArchiveManager> Archives() {
    auto context = Ship::Context::GetInstance();
    if (context == nullptr || context->GetResourceManager() == nullptr) {
        return nullptr;
    }
    return context->GetResourceManager()->GetArchiveManager();
}

bool IsModPath(const std::string& name) {
    static const char* const kPrefixes[] = { "messages/", "charset/", "textures/", "ui/",       "icons/",
                                             "effects/",  "entities/", "battle/",  "level_up/", "logos/",
                                             "theater/",  "world/",    "misc/",   "sprites/", "backgrounds/" };
    for (const char* p : kPrefixes) {
        if (name.compare(0, strlen(p), p) == 0) {
            return true;
        }
    }
    return false;
}

void Init() {
    if (sInitDone) {
        return;
    }
    auto archives = Archives();
    if (archives == nullptr) {
        return;
    }
    sInitDone = true;

    auto files = archives->ListFiles();
    if (files != nullptr) {
        for (const auto& f : *files) {
            std::string name = f.compare(0, 4, "alt/") == 0 ? f.substr(4) : f;
            if (!IsModPath(name)) {
                continue;
            }
            sModFiles.insert(name);
            if (name.compare(0, 9, "messages/") == 0) {
                sHasMessages = true;
            } else if (name.compare(0, 8, "charset/") == 0) {
                sHasBlobs = true;
            } else {
                sHasTextures = true;
            }
        }
    }
    fprintf(stderr, "[mods] %u mod assets found (text=%d fonts=%d textures=%d)\n", (unsigned)sModFiles.size(), sHasMessages,
            sHasBlobs, sHasTextures);
}

// "alt/" copies win, matching how Ship ports layer HD packs
std::string ResolvePath(const std::string& path) {
    auto archives = Archives();
    std::string alt = "alt/" + path;
    if (archives != nullptr && archives->HasFile(alt)) {
        return alt;
    }
    return path;
}

std::shared_ptr<Ship::IResource> Load(const std::string& path) {
    auto context = Ship::Context::GetInstance();
    return context->GetResourceManager()->LoadResource(ResolvePath(path), true);
}

uint32_t Signature(const void* data, uint32_t size) {
    const uint8_t* p = (const uint8_t*)data;
    uint32_t n = size < kSigBytes ? size : kSigBytes;
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

void Register(const void* dest, uint32_t size, const std::string& path, uint32_t rowBytes = 0) {
    uintptr_t start = (uintptr_t)dest;
    auto it = sRegistry.lower_bound(start);
    while (it != sRegistry.end() && it->first < start + size) {
        it = sRegistry.erase(it);
    }
    sRegistry[start] = { size, Signature(dest, size), rowBytes, path };
}

void Unregister(uintptr_t start, uint32_t size) {
    auto it = sRegistry.lower_bound(start);
    while (it != sRegistry.end() && it->first < start + size) {
        it = sRegistry.erase(it);
    }
}

// halve an RGBA32 texture until it fits kMaxTexels; scale factors keep the draw size the same
void Downscale(Fast::Texture* tex, const std::string& path) {
    if (tex->Type != Fast::TextureType::RGBA32bpp) {
        return;
    }
    uint32_t w = tex->Width, h = tex->Height;
    const uint32_t origW = w, origH = h;
    uint8_t* data = tex->ImageData;
    while (w * h > kMaxTexels && w >= 2 && h >= 2 && (w % 2) == 0 && (h % 2) == 0) {
        uint32_t nw = w / 2, nh = h / 2;
        uint8_t* out = new uint8_t[nw * nh * 4];
        for (uint32_t y = 0; y < nh; y++) {
            for (uint32_t x = 0; x < nw; x++) {
                for (uint32_t c = 0; c < 4; c++) {
                    uint32_t s = data[((2 * y) * w + 2 * x) * 4 + c] + data[((2 * y) * w + 2 * x + 1) * 4 + c] +
                                 data[((2 * y + 1) * w + 2 * x) * 4 + c] + data[((2 * y + 1) * w + 2 * x + 1) * 4 + c];
                    out[(y * nw + x) * 4 + c] = (uint8_t)((s + 2) / 4);
                }
            }
        }
        if (data != tex->ImageData) {
            delete[] data;
        }
        data = out;
        w = nw;
        h = nh;
    }
    if (data == tex->ImageData) {
        return;
    }
    delete[] tex->ImageData;
    tex->ImageData = data;
    tex->ImageDataSize = w * h * 4;
    tex->HByteScale *= (float)w / origW;
    tex->VPixelScale *= (float)h / origH;
    tex->Width = w;
    tex->Height = h;
    fprintf(stderr, "[mods] downscaled %s %ux%u -> %ux%u\n", path.c_str(), origW, origH, w, h);
}

const PortModAsset* FirstAssetAtOrAfter(uint32_t rom) {
    uint32_t lo = 0, hi = gPortModAssetCount;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        if (gPortModAssets[mid].rom < rom) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo < gPortModAssetCount ? &gPortModAssets[lo] : nullptr;
}

} // namespace

extern "C" int Port_ModLoadMessage(unsigned int msgID, void* dest, unsigned int cap) {
    Init();
    if (!sHasMessages || dest == nullptr || cap < 2) {
        return 0;
    }
    const char* suffix = port_mod_msg_suffix(msgID);
    if (suffix == nullptr) {
        return 0;
    }
    std::string path = std::string("messages/MSG_") + suffix;
    if (sModFiles.find(path) == sModFiles.end()) {
        return 0;
    }
    auto blob = std::dynamic_pointer_cast<Ship::Blob>(Load(path));
    if (blob == nullptr || blob->Data.empty()) {
        fprintf(stderr, "[mods] %s could not be read, using ROM text\n", path.c_str());
        return 0;
    }

    uint8_t* out = (uint8_t*)dest;
    size_t n = blob->Data.size();
    if (n > cap - 1) {
        fprintf(stderr, "[mods] %s is %u bytes, cut to %u\n", path.c_str(), (unsigned)n, cap - 1);
        n = cap - 1;
    }
    memcpy(out, blob->Data.data(), n);
    if (out[n - 1] != kMsgEnd) {
        out[n] = kMsgEnd;
    }
    return 1;
}

extern "C" void Port_ModOnRomRead(uint32_t rom, void* dest, uint32_t size) {
    if (dest == nullptr || size == 0 || std::this_thread::get_id() != gPortMainThreadId) {
        return;
    }
    Init();
    if (!sHasTextures && !sHasBlobs) {
        return;
    }

    Unregister((uintptr_t)dest, size);
    for (const PortModAsset* a = FirstAssetAtOrAfter(rom); a != nullptr && a < gPortModAssets + gPortModAssetCount;
         a++) {
        if (a->rom >= rom + size) {
            break;
        }
        // a texture must be read whole; a blob may be read in part (the game reads 0x5100 of the
        // 0x5300-byte standard charset)
        if (a->kind == PORT_MOD_KIND_TEXTURE && a->rom + a->size > rom + size) {
            continue;
        }
        const uint32_t readable = rom + size - a->rom < a->size ? rom + size - a->rom : a->size;
        std::string path = a->path;
        if (sModFiles.find(path) == sModFiles.end()) {
            continue;
        }
        uint8_t* at = (uint8_t*)dest + (a->rom - rom);

        if (a->kind == PORT_MOD_KIND_TEXTURE) {
            Register(at, a->size, path);
        } else if (path.compare(0, 8, "charset/") == 0) {
            // a mod font replaces the ROM bytes in place, never past the original size
            auto blob = std::dynamic_pointer_cast<Ship::Blob>(Load(path));
            if (blob != nullptr && !blob->Data.empty()) {
                size_t n = blob->Data.size() < readable ? blob->Data.size() : readable;
                memcpy(at, blob->Data.data(), n);
                fprintf(stderr, "[mods] font %s applied (%u bytes)\n", path.c_str(), (unsigned)n);
            }
        }
    }
}

extern "C" void Port_ModSetTexArchive(const char* archiveName) {
    snprintf(sTexArchive, sizeof(sTexArchive), "%s", archiveName != nullptr ? archiveName : "");
}

extern "C" void Port_ModRegisterMapTexture(const void* raster, uint32_t size, const char* name, int aux) {
    Init();
    if (!sHasTextures || raster == nullptr || size == 0 || sTexArchive[0] == '\0') {
        return;
    }
    char path[96];
    snprintf(path, sizeof(path), "textures/%s/%.32s%s", sTexArchive, name, aux ? "_aux" : "");
    if (sModFiles.find(path) == sModFiles.end()) {
        Unregister((uintptr_t)raster, size);
        return;
    }
    Register(raster, size, path);
}

extern "C" void Port_ModRegisterPath(const void* data, uint32_t size, const char* path) {
    Port_ModRegisterImage(data, size, 0, path);
}

extern "C" void Port_ModRegisterImage(const void* data, uint32_t size, uint32_t rowBytes, const char* path) {
    Init();
    if (!sHasTextures || data == nullptr || size == 0) {
        return;
    }
    if (sModFiles.find(path) == sModFiles.end()) {
        Unregister((uintptr_t)data, size);
        return;
    }
    Register(data, size, path, rowBytes);
}

std::shared_ptr<Fast::Texture> Port_ModTextureFor(const void* addr, uint32_t* hdOffset, uint32_t* hdRowsSkipped) {
    *hdOffset = 0;
    *hdRowsSkipped = 0;
    if (!sHasTextures || sRegistry.empty()) {
        return nullptr;
    }
    uintptr_t a = (uintptr_t)addr;
    auto it = sRegistry.upper_bound(a);
    if (it == sRegistry.begin()) {
        return nullptr;
    }
    --it;
    uint32_t inner = (uint32_t)(a - it->first);
    if (inner >= it->second.size || (inner != 0 && (it->second.rowBytes == 0 || inner % it->second.rowBytes != 0))) {
        return nullptr;
    }
    // the buffer may have been reused for something else since it was registered
    if (Signature((const void*)it->first, it->second.size) != it->second.sig) {
        sRegistry.erase(it);
        return nullptr;
    }
    const uint32_t rowsIn = inner != 0 ? inner / it->second.rowBytes : 0;

    const std::string& path = it->second.path;
    auto rowOffset = [&](const std::shared_ptr<Fast::Texture>& t) -> std::shared_ptr<Fast::Texture> {
        if (rowsIn == 0) {
            return t;
        }
        // HD rows per N64 row is the vertical scale; only RGBA32 images can be offset this way
        if (t->Type != Fast::TextureType::RGBA32bpp) {
            return nullptr;
        }
        uint32_t hdRow = (uint32_t)(rowsIn * t->VPixelScale);
        if (hdRow >= t->Height) {
            return nullptr;
        }
        *hdOffset = hdRow * t->Width * 4;
        *hdRowsSkipped = hdRow;
        return t;
    };
    auto cached = sTextures.find(path);
    if (cached != sTextures.end()) {
        return rowOffset(cached->second);
    }
    if (sMissing.count(path) != 0) {
        return nullptr;
    }
    auto tex = std::dynamic_pointer_cast<Fast::Texture>(Load(path));
    if (tex == nullptr || tex->ImageData == nullptr) {
        fprintf(stderr, "[mods] texture %s could not be loaded, using ROM\n", path.c_str());
        sMissing.insert(path);
        return nullptr;
    }
    Downscale(tex.get(), path);
    if (sTextureBytes + tex->ImageDataSize > kTextureBudget && !sTextures.empty()) {
        auto rm = Ship::Context::GetInstance()->GetResourceManager();
        for (auto& entry : sTextures) {
            if (entry.second != tex) {
                rm->UnloadResource(ResolvePath(entry.first));
            }
        }
        sTextures.clear();
        sTextureBytes = 0;
        gfx_texture_cache_clear();
        fprintf(stderr, "[mods] texture budget reached, cache flushed\n");
    }
    sTextures[path] = tex;
    sTextureBytes += tex->ImageDataSize;
    if (sReplacedLogged < 200) {
        sReplacedLogged++;
        fprintf(stderr, "[mods] texture %s replaced (%ux%u)\n", path.c_str(), tex->Width, tex->Height);
    }
    return rowOffset(tex);
}
