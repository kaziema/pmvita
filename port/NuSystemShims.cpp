/**
 * NuSystem Shim Layer for PaperShip
 *
 * Provides PC implementations for all NuSystem functions used by Paper Mario.
 * These redirect to libultraship or act as no-ops where the N64-specific
 * functionality is not needed on PC.
 *
 * Reference: papermario-main/include/nu/nusys.h
 */
#include <libultraship.h>
#include "NuSystemShims.h"
#include "Engine.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
#include <string>
#include <mutex>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <execinfo.h>
#endif

// ROM file handle for nuPiReadRom
static FILE* sRomFile = nullptr;
static size_t sRomFileSize = 0;
static std::mutex sRomFileMutex;

extern "C" {
#include "common.h"

// ============================================================================
// NuSystem State Variables
// ============================================================================

// Retrace callback function pointer - set by nuGfxFuncSet(), called by Game.cpp
NUGfxFunc nuGfxFunc = NULL;

// PreNMI callback - called when user wants to quit/reset
NUGfxPreNMIFunc nuGfxPreNMIFunc_stored = NULL;

// Display flag - toggled by nuGfxDisplayOn/Off
u32 nuGfxDisplay = 0;

// Current framebuffer pointer - used by crash_screen_set_draw_info
u16* nuGfxCfb_ptr = NULL;

// Retrace counter
u32 nuScRetraceCounter = 0;

// PreNMI flag
u8 nuScPreNMIFlag = 0;

// Controller data (used by nuContDataGet/nuContDataGetAll)
OSContStatus nuContStatus[MAXCONTROLLERS];
OSContPad nuContData[MAXCONTROLLERS];
u32 nuContNum = 0;
u32 nuContDataLockKey = 0;

// Rumble state
NUContRmbCtl nuContRmbCtl[MAXCONTROLLERS] = {};

// PI Handle (for DMA operations - redirected to OTR on PC)
OSPiHandle* nuPiCartHandle = NULL;

// ============================================================================
// Graphics Manager Functions
// ============================================================================

/**
 * nuGfxInit / nuGfxInitEX2
 * On N64: Initializes the graphics system, scheduler, framebuffers, Z-buffer.
 * On PC: No-op - libultraship handles all graphics initialization.
 *
 * Note: In nusys.h, when F3DEX_GBI_2 is defined, nuGfxInit() is #defined to
 * nuGfxInitEX2(). Paper Mario uses F3DEX2, so both need to be available.
 */
void nuGfxInitEX2(void) {
    // libultraship handles actual GPU init. But game code passes framebuffer
    // addresses through GBI commands (gDPSetColorImage, gDPSetDepthImage) and
    // compares them to determine which buffer is active. Allocate dummy buffers
    // so these addresses are non-null and distinct.
    extern u16* nuGfxZBuffer;
    extern u16** nuGfxCfb;

    static u16* sCfbArray[3];
    sCfbArray[0] = (u16*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u16));
    sCfbArray[1] = (u16*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u16));
    sCfbArray[2] = (u16*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u16));
    nuGfxZBuffer = (u16*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u16));

    nuGfxCfb = sCfbArray;
    nuGfxCfb_ptr = sCfbArray[0];
}

// nuGfxInit is #defined to nuGfxInitEX2 when F3DEX_GBI_2 is set,
// but provide the base version too for safety
#ifndef F3DEX_GBI_2
void nuGfxInit(void) {
    // No-op
}
#endif

/**
 * nuGfxFuncSet - Register the retrace callback
 * On N64: Called by VBlank interrupt via scheduler.
 * On PC: Stored and called directly by Game.cpp's push_frame().
 */
void nuGfxFuncSet(NUGfxFunc func) {
    nuGfxFunc = func;
}

/**
 * nuGfxPreNMIFuncSet - Register the pre-NMI (reset) callback
 * On N64: Called when the reset button is pressed.
 * On PC: Called when the user closes the window or triggers soft reset.
 */
void nuGfxPreNMIFuncSet(NUGfxPreNMIFunc func) {
    nuGfxPreNMIFunc_stored = func;
}

/**
 * nuGfxDisplayOn/Off - Toggle display rendering
 * On N64: Controls whether the graphics thread processes display lists.
 * On PC: Sets a flag that Game.cpp can check.
 */
void nuGfxDisplayOn(void) {
    nuGfxDisplay = NU_GFX_DISPLAY_ON;
}

void nuGfxDisplayOff(void) {
    nuGfxDisplay = NU_GFX_DISPLAY_OFF;
}

/**
 * nuGfxTaskStart - Submit a display list for rendering
 * On N64: Queues a graphics task for the RCP.
 * On PC: Routes to GameEngine::ProcessGfxCommands() which sends to Fast3D.
 *
 * NOTE: This function is also defined in Game.cpp as the primary entry point.
 * This version exists as a fallback / for when Game.cpp's version isn't linked.
 * The Game.cpp version takes priority.
 */
// nuGfxTaskStart is defined in Game.cpp

/**
 * nuGfxTaskAllEndWait - Wait for all graphics tasks to complete
 * On N64: Blocks until the RCP finishes all queued tasks.
 * On PC: No-op - rendering is synchronous.
 */
void nuGfxTaskAllEndWait(void) {
    // No-op: PC rendering is synchronous within ProcessGfxCommands
}

/**
 * nuGfxSetCfb - Set framebuffer array
 * On N64: Configures the framebuffer pointers for triple buffering.
 * On PC: No-op - libultraship manages framebuffers.
 */
void nuGfxSetCfb(u16** framebuf, u32 framebufnum) {
    // Update framebuffer array — game code compares these pointers
    extern u16** nuGfxCfb;
    extern u32 nuGfxCfbNum;
    nuGfxCfb = framebuf;
    nuGfxCfbNum = framebufnum;
    if (framebuf != NULL && framebufnum > 0) {
        nuGfxCfb_ptr = framebuf[0];
    }
}

/**
 * nuGfxSwapCfb - Swap to a specific framebuffer
 * On N64: Sets the next framebuffer to display.
 * On PC: No-op.
 */
void nuGfxSwapCfb(void* framebuffer) {
    nuGfxCfb_ptr = (u16*)framebuffer;
}

/**
 * port_cycle_framebuffer - Simulate N64 framebuffer cycling
 * On N64, the display controller cycles nuGfxCfb_ptr through the
 * framebuffer array each retrace. Game code checks nuGfxCfb[N] against
 * nuGfxCfb_ptr for synchronization (e.g., state_step_battle waits for
 * nuGfxCfb[1] == nuGfxCfb_ptr). Without cycling, these checks hang.
 */
void port_cycle_framebuffer(void) {
    extern u16** nuGfxCfb;
    extern u32 nuGfxCfbNum;
    static u32 cfbIndex = 0;
    if (nuGfxCfb != NULL && nuGfxCfbNum > 0) {
        cfbIndex = (cfbIndex + 1) % nuGfxCfbNum;
        nuGfxCfb_ptr = nuGfxCfb[cfbIndex];
    }
}

/**
 * nuGfxRetraceWait - Wait for N retrace intervals
 * On N64: Blocks until the specified number of retraces occur.
 * On PC: No-op (timing handled by SDL/window system).
 */
void nuGfxRetraceWait(u32 retrace_num) {
    // No-op
    (void)retrace_num;
}

void nuGfxSwapCfbFuncSet(NUGfxSwapCfbFunc func) {
    // No-op
    (void)func;
}

void nuGfxTaskEndFuncSet(NUGfxTaskEndFunc func) {
    // No-op
    (void)func;
}

// ============================================================================
// Controller Manager Functions
// ============================================================================

/**
 * nuContInit - Initialize controllers
 * On N64: Queries the SI bus for connected controllers via SI DMA.
 * On PC: Initializes SDL gamecontroller + keyboard via libultraship's ControlDeck.
 */
u8 nuContInit(void) {
    nuContNum = 1;

    // Load SDL gamecontroller mappings (must happen before SDL_Init for gamepads)
    std::string controllerDb = Ship::Context::LocateFileAcrossAppDirs("gamecontrollerdb.txt");
    int mappingsAdded = SDL_GameControllerAddMappingsFromFile(controllerDb.c_str());
    if (mappingsAdded >= 0) {
        SPDLOG_INFO("nuContInit: loaded {} SDL gamecontroller mappings from {}", mappingsAdded, controllerDb);
    }

    // Initialize SDL gamecontroller subsystem (needed for Pro Controller, DualSense, Xbox, etc.)
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        SPDLOG_ERROR("nuContInit: failed to init SDL gamecontroller: {}", SDL_GetError());
    }

    // Initialize libultraship's ControlDeck (sets up keyboard + any connected gamepad)
    uint8_t controllerBits = 0;
    Ship::Context::GetInstance()->GetControlDeck()->Init(&controllerBits);
    SPDLOG_INFO("nuContInit: controllerBits=0x{:02X}", controllerBits);
    // Always report at least controller 1 connected (keyboard always available)
    return controllerBits | 0x01;
}

/**
 * nuContDataGet - Get controller data for one controller.
 * Polls libultraship's ControlDeck for the latest keyboard/gamepad state.
 */
void nuContDataGet(OSContPad* contdata, u32 padno) {
    // Poll ControlDeck to populate nuContData with latest input
    memset(nuContData, 0, sizeof(nuContData));
    Ship::Context::GetInstance()->GetControlDeck()->WriteToPad(nuContData);
    if (padno < MAXCONTROLLERS) {
        *contdata = nuContData[padno];
    }
}

/**
 * nuContDataGetAll - Get controller data for all controllers
 */
void nuContDataGetAll(OSContPad* contdata) {
    memset(nuContData, 0, sizeof(nuContData));
    Ship::Context::GetInstance()->GetControlDeck()->WriteToPad(nuContData);
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        contdata[i] = nuContData[i];
    }
}

void nuContDataLock(void) {
    nuContDataLockKey = 1;
}

void nuContDataUnLock(void) {
    nuContDataLockKey = 0;
}

void nuContDataRead(OSContPad* pad) {
    // Poll ControlDeck
    memset(nuContData, 0, sizeof(nuContData));
    Ship::Context::GetInstance()->GetControlDeck()->WriteToPad(nuContData);
    if (pad != NULL) {
        for (int i = 0; i < MAXCONTROLLERS; i++) {
            pad[i] = nuContData[i];
        }
    }
}

void nuContDataReadWait(void) {
    // No-op — polling is synchronous on PC
}

void nuContDataReadStart(void) {
    // No-op — polling is synchronous on PC
}

void nuContReadFuncSet(NUContReadFunc func) {
    // No-op
    (void)func;
}

void nuContQueryRead(void) {
    // No-op
}

void nuContDataClose(void) {
    // No-op
}

void nuContDataOpen(void) {
    // No-op
}

// ============================================================================
// Rumble Pak Functions
// ============================================================================

void nuContRmbMgrInit(void) {
    // No-op - rumble handled by libultraship/SDL2
}

void nuContRmbStart(u32 contNo, u16 freq, u16 frame) {
    // TODO: Forward to libultraship rumble system
    (void)contNo;
    (void)freq;
    (void)frame;
}

void nuContRmbStop(u32 contNo) {
    // TODO: Forward to libultraship rumble system
    (void)contNo;
}

s32 nuContRmbCheck(u32 contNo) {
    // Return 0 = rumble pak found
    (void)contNo;
    return 0;
}

void nuContRmbModeSet(u32 contNo, u8 mode) {
    if (contNo < MAXCONTROLLERS) {
        nuContRmbCtl[contNo].mode = mode;
    }
}

void nuContRmbForceStop(void) {
    // Stop all rumble
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        nuContRmbCtl[i].state = NU_CONT_RMB_STATE_STOP;
    }
}

void nuContRmbForceStopEnd(void) {
    // No-op
}

void nuContRmbSearchTimeSet(u32 frame) {
    // No-op
    (void)frame;
}

// ============================================================================
// PI (Parallel Interface) Functions - ROM/DMA Access
// ============================================================================

/**
 * nuPiInit - Initialize the PI manager
 * On N64: Sets up DMA transfers from cartridge ROM.
 * On PC: No-op - we load from OTR archives instead.
 */
void nuPiInit(void) {
    // No-op
}

/**
 * nuPiReadRom - Read data from ROM file
 * On N64: DMA transfer from cartridge.
 * On PC: Read from the ROM .z64 file on disk.
 *
 * The ROM file is searched in several locations:
 *   1. Current working directory (build/)
 *   2. Parent directory (papermario-recomp/)
 *   3. Project root (Paper Mario 64 Project/)
 *
 * Accepted filenames: Paper Mario (USA).z64, baserom.us.z64, pm64.z64
 */
static std::string getExeDirectory() {
#ifdef __APPLE__
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string s(path);
        auto pos = s.rfind('/');
        if (pos != std::string::npos) return s.substr(0, pos);
    }
#endif
    return ".";
}

static void nuPiReadRom_OpenRomFile(void) {
    static bool sSearched = false;
    if (sSearched) return;
    sSearched = true;

    const char* rom_names[] = {
        "Paper Mario (USA).z64",
        "baserom.us.z64",
        "pm64.z64",
        "papermario.z64",
    };

    // Search relative to CWD and relative to executable location
    std::string exeDir = getExeDirectory();
    std::vector<std::string> search_dirs = {
        ".",
        "..",
        "../..",
        exeDir,
        exeDir + "/..",
        exeDir + "/../..",
#ifdef __vita__
        // app0: (the VPK package itself) is read-only -- the ROM has to
        // live on the memory card, same place the save data does.
        "ux0:data/papership",
#endif
    };

    for (auto& dir : search_dirs) {
        for (auto name : rom_names) {
            std::string path = dir + "/" + name;
            FILE* f = fopen(path.c_str(), "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                sRomFileSize = ftell(f);
                fseek(f, 0, SEEK_SET);
                sRomFile = f;
                SPDLOG_INFO("Opened ROM file: {} ({} bytes)", path, sRomFileSize);
                return;
            }
        }
    }
    SPDLOG_ERROR("ROM file not found! Audio and other DMA-loaded data will be unavailable.");
    SPDLOG_ERROR("Place your Paper Mario (USA).z64 ROM in the build directory or project root.");
}

void nuPiReadRom(u32 rom_addr, void* buf_ptr, u32 size) {
    nuPiReadRom_OpenRomFile();

    // Serialize all ROM file access — audio thread and main thread may call
    // nuPiReadRom concurrently, and fseek+fread is not atomic.
    std::lock_guard<std::mutex> lock(sRomFileMutex);

    if (sRomFile == nullptr) {
        memset(buf_ptr, 0, size);
        return;
    }

    if (rom_addr + size > sRomFileSize) {
        static int sOutOfBoundsCount = 0;
        if (sOutOfBoundsCount < 5) {
            SPDLOG_WARN("nuPiReadRom: read past end of ROM (addr=0x{:08X} size={}, ROM size={})",
                         rom_addr, size, sRomFileSize);
            sOutOfBoundsCount++;
            if (sOutOfBoundsCount == 5) {
                SPDLOG_WARN("nuPiReadRom: suppressing further out-of-bounds warnings");
            }
        }
        memset(buf_ptr, 0, size);
        return;
    }

    fseek(sRomFile, rom_addr, SEEK_SET);
    size_t read = fread(buf_ptr, 1, size, sRomFile);
    if (read != size) {
        SPDLOG_WARN("nuPiReadRom: short read at 0x{:08X} (got {} of {} bytes)", rom_addr, read, size);
        memset((u8*)buf_ptr + read, 0, size - read);
    }

    // .z64 ROM data is big-endian. We read raw bytes here and do NOT byte-swap.
    // Callers that read multi-byte integers (u32/u16) from ROM data must handle
    // endianness conversion themselves. Raw byte data (compressed streams, textures,
    // rasters) is used as-is since individual bytes have no endianness.
}

/**
 * nuPiReadRomOverlay - Load an overlay segment from ROM
 * On N64: DMA transfer of code/data overlay.
 * On PC: All overlays are statically linked, so this is a no-op.
 */
void nuPiReadRomOverlay(NUPiOverlaySegment* segment) {
    // Overlay code is statically linked here, so there is nothing to DMA. The BSS range comes
    // from linker stub symbols my linker orders backwards, making bssEnd - bssStart negative;
    // as a size_t that asked memset to wipe ~4GB from the middle of BSS on every pause.
    if (segment->bssStart != NULL && segment->bssEnd != NULL && segment->bssEnd > segment->bssStart) {
        memset(segment->bssStart, 0, (size_t)(segment->bssEnd - segment->bssStart));
    }
}

// ============================================================================
// EEPROM Functions - Save Data
// ============================================================================

/**
 * On N64: EEPROM is the save medium (4Kbit or 16Kbit).
 * On PC: Redirected to file-based saves.
 * TODO: Implement proper SaveManager integration.
 */

void nuEepromMgrInit(void) {
    // No-op
}

s32 nuEepromCheck(void) {
    // Return EEPROM type (16Kbit for Paper Mario)
    return 2; // EEPROM_16K
}

s32 nuEepromRead(u8 address, u8* buffer, u16 len) {
    // TODO: Read from save file
    SPDLOG_WARN("nuEepromRead called: addr={} len={} - stub", address, len);
    memset(buffer, 0, len);
    return 0;
}

s32 nuEepromWrite(u8 address, u8* buffer, u16 len) {
    // TODO: Write to save file
    SPDLOG_WARN("nuEepromWrite called: addr={} len={} - stub", address, len);
    return 0;
}

// ============================================================================
// Scheduler Functions
// ============================================================================

void nuScCreateScheduler(u8 videoMode, u8 numFields) {
    // On N64: creates the scheduler with retrace count.
    // On PC: just store the retrace count — audio system needs it for frame size calculation.
    nusched.retraceCount = numFields;
    (void)videoMode;
}

void nuScAddClient(NUScClient* c, OSMesgQueue* mq, NUScMsg msgType) {
    // No-op
    (void)c;
    (void)mq;
    (void)msgType;
}

void nuScRemoveClient(NUScClient* client) {
    // No-op
    (void)client;
}

s32 nuScGetFrameRate(void) {
    return 1; // 60fps display rate
}

// ============================================================================
// SI Manager Functions
// ============================================================================

u8 nuSiMgrInit(void) {
    return 0;
}

void nuSiCallBackAdd(NUCallBackList* list) {
    (void)list;
}

void nuSiCallBackRemove(NUCallBackList* list) {
    (void)list;
}

s32 nuSiSendMesg(NUScMsg mesg, void* dataPtr) {
    (void)mesg;
    (void)dataPtr;
    return 0;
}

void nuSiSendMesgNW(NUScMsg mesg, void* dataPtr) {
    (void)mesg;
    (void)dataPtr;
}

void nuSiMgrStop(void) {
    // No-op
}

void nuSiMgrRestart(void) {
    // No-op
}

// ============================================================================
// Debug Functions (no-ops in release, but need to exist for linking)
// ============================================================================

// nuDebConDisp is #defined to nuDebConDispEX2 in nusys.h, so only define nuDebConDispEX2
void nuDebConDispEX2(u32 flag) { (void)flag; }
void nuDebConCPuts(u32 wndNo, const char* s) { (void)wndNo; (void)s; }
void nuDebConWindowSet(u32 wndNo, u32 winX, u32 winY, u32 width, u32 height) {
    (void)wndNo; (void)winX; (void)winY; (void)width; (void)height;
}
void nuDebConTextColor(u32 wndNo, u32 color) { (void)wndNo; (void)color; }
void nuDebConTextPos(u32 wndNo, u32 posX, u32 posY) { (void)wndNo; (void)posX; (void)posY; }
void nuDebConClear(u32 wndNo) { (void)wndNo; }
void nuDebConPutc(u32 wndNo, u32 c) { (void)wndNo; (void)c; }
void nuDebConPrintf(u32 wndNo, const char* fmt, ...) { (void)wndNo; (void)fmt; }

} // extern "C"
