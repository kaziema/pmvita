/**
 * os_stubs.c - N64 OS function stubs for PC port
 *
 * These provide implementations for N64 OS functions that are:
 * - Declared in our ultra64.h bridge header
 * - NOT provided by libultraship (libultraship provides its own implementations
 *   for: osViSetMode, osViSetSpecialFeatures, osViSetYScale, osViSwapBuffer,
 *   osPiStartDma, osAiSetFrequency, osAiSetNextBuffer, osAiGetLength,
 *   osGetCount, osGetTime, osSetTime, osSetTimer, osSetIntMask,
 *   osCartRomInit, osEPiStartDma, osCreatePiManager, osContInit,
 *   osContStartReadData, osContGetReadData, osInvalICache, osInvalDCache,
 *   osWritebackDCache, osWritebackDCacheAll)
 *
 * On PC, most of these are no-ops or return dummy values because the
 * underlying hardware operations don't apply.
 */

#include "ultra64.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <errno.h>

void port_debug_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

// ============================================================
// VI globals — stubs on PC
// ============================================================

s32 osTvType = 1; // OS_TV_NTSC

// Stub VI mode structs (libultraship may provide these, weak to allow override)
__attribute__((weak)) OSViMode osViModeNtscLan1 = {0};
__attribute__((weak)) OSViMode osViModeMpalLan1 = {0};
__attribute__((weak)) OSViMode osViModeFpalLpn1 = {0};

/* osViGetCurrentFramebuffer — provided by libultraship */

// ============================================================
// Virtual/Physical address mapping
// On PC, all addresses are virtual — identity operation
// ============================================================

void* osVirtualToPhysical(void* addr) {
    return addr;
}

// ============================================================
// Flash emulation — file-backed save system
//
// N64 flash: 128KB = 1024 pages × 128 bytes/page
// Organized in 8 sectors × 128 pages (16KB/sector)
// Paper Mario layout:
//   Logical pages 0-5: save data (SaveData = 0x1380 bytes each)
//   Logical pages 6-7: globals (SaveGlobals = 0x80 bytes, duplicated)
// Each logical page maps to flash page logicalPage*128
// ============================================================

#define FLASH_PAGE_SIZE    128
#define FLASH_NUM_PAGES    1024
#define FLASH_SIZE         (FLASH_PAGE_SIZE * FLASH_NUM_PAGES)
#define FLASH_SECTOR_PAGES 128

static u8 flash_data[FLASH_SIZE];
static u8 flash_write_buf[FLASH_PAGE_SIZE];
static int flash_initialized = 0;
static int flash_dirty = 0;
#ifdef __vita__
// The working directory is read-only on Vita, so the save has to live on the memory card.
static const char* SAVE_FILENAME = "ux0:data/papership/papership_save.bin";
#else
static const char* SAVE_FILENAME = "papership_save.bin";
#endif

static void flash_load_from_disk(void) {
    FILE* f = fopen(SAVE_FILENAME, "rb");
    if (f) {
        size_t bytesRead = fread(flash_data, 1, FLASH_SIZE, f);
        fclose(f);
        fprintf(stderr, "[Flash] Loaded save data from %s (%u bytes)\n", SAVE_FILENAME, (unsigned)bytesRead);
    } else {
        memset(flash_data, 0, FLASH_SIZE);
        fprintf(stderr, "[Flash] No save file found, starting fresh\n");
    }
}

static void flash_flush_to_disk(void) {
    FILE* f = fopen(SAVE_FILENAME, "wb");
    if (f) {
        size_t written = fwrite(flash_data, 1, FLASH_SIZE, f);
        s32 closeResult = fclose(f);
        fprintf(stderr, "[Flash] Flushed %u/%d bytes to %s (fclose=%d)\n", (unsigned)written, FLASH_SIZE,
                SAVE_FILENAME, closeResult);
    } else {
        fprintf(stderr, "[Flash] ERROR: Could not write save file %s (errno=%d)\n", SAVE_FILENAME, errno);
    }
}

// Batches dozens of per-page writes into one flush a frame instead of rewriting 128KB each time.
void port_flash_flush_if_dirty(void) {
    if (flash_dirty) {
        flash_dirty = 0;
        flash_flush_to_disk();
    }
}

void osFlashInit(void) {
    if (!flash_initialized) {
        flash_load_from_disk();
        flash_initialized = 1;
    }
}

s32 osFlashReadArray(void* mb, s32 priority, u32 pageNum, void* dramAddr, u32 nPages, void* mq) {
    (void)mb; (void)priority; (void)mq;

    if (!flash_initialized) {
        flash_load_from_disk();
        flash_initialized = 1;
    }

    u32 offset = pageNum * FLASH_PAGE_SIZE;
    u32 size = nPages * FLASH_PAGE_SIZE;

    if (offset + size > FLASH_SIZE) {
        // Expected when save slots are empty (LogicalSaveInfo[].slot == -1)
        memset(dramAddr, 0, size);
        return 0;
    }

    memcpy(dramAddr, flash_data + offset, size);
    return 0;
}

s32 osFlashWriteBuffer(void* mb, s32 priority, void* dramAddr, void* mq) {
    (void)mb; (void)priority; (void)mq;
    memcpy(flash_write_buf, dramAddr, FLASH_PAGE_SIZE);
    return 0;
}

s32 osFlashWriteArray(u32 pageNum) {
    u32 offset = pageNum * FLASH_PAGE_SIZE;

    if (offset + FLASH_PAGE_SIZE > FLASH_SIZE) {
        fprintf(stderr, "[Flash] WriteArray: out of bounds page=%u\n", pageNum);
        return -1;
    }

    memcpy(flash_data + offset, flash_write_buf, FLASH_PAGE_SIZE);
    flash_dirty = 1;

    static int sWriteLogCount = 0;
    if (sWriteLogCount < 20) {
        fprintf(stderr, "[Flash] WriteArray page=%u (dirty set)\n", pageNum);
        sWriteLogCount++;
    }
    return 0;
}

s32 osFlashSectorErase(u32 pageNum) {
    // Erase entire sector (128 pages = 16KB) containing pageNum
    u32 sectorStart = (pageNum / FLASH_SECTOR_PAGES) * FLASH_SECTOR_PAGES;
    u32 offset = sectorStart * FLASH_PAGE_SIZE;
    u32 size = FLASH_SECTOR_PAGES * FLASH_PAGE_SIZE;

    if (offset + size > FLASH_SIZE) {
        fprintf(stderr, "[Flash] SectorErase: out of bounds page=%u\n", pageNum);
        return -1;
    }

    memset(flash_data + offset, 0, size);
    flash_dirty = 1;
    return 0;
}

void osFlashClearStatus(void) {
    // No-op on PC
}

u32 osFlashGetAddr(s32 pageNum) {
    (void)pageNum;
    return 0;
}

s32 osFlashReadId(u32* type, u32* maker) {
    if (type) *type = 0;
    if (maker) *maker = 0;
    return 0;
}

s32 osFlashReadStatus(u8* flashStatus) {
    if (flashStatus) *flashStatus = 0;
    return 0;
}

void osFlashAllErase(void) {
    memset(flash_data, 0, FLASH_SIZE);
    flash_flush_to_disk();
    fprintf(stderr, "[Flash] All erase\n");
}

void osFlashAllEraseThrough(void) {
    memset(flash_data, 0, FLASH_SIZE);
    flash_flush_to_disk();
}

void osFlashSectorEraseThrough(s32 pageNum) {
    osFlashSectorErase(pageNum);
}

// ============================================================
// AI status (not provided by libultraship)
// ============================================================

u32 osAiGetStatus(void) {
    return 0;
}

// ============================================================
// Thread operations — single-threaded on PC
// (libultraship does not provide thread stubs)
// ============================================================

void osCreateThread(OSThread* thread, OSId id, void (*entry)(void*),
                    void* arg, void* sp, OSPri pri) {
    (void)thread; (void)id; (void)entry;
    (void)arg; (void)sp; (void)pri;
}

void osStartThread(OSThread* thread) {
    (void)thread;
}

void osStopThread(OSThread* thread) {
    (void)thread;
}

void osDestroyThread(OSThread* thread) {
    (void)thread;
}

void osSetThreadPri(OSThread* thread, OSPri pri) {
    (void)thread; (void)pri;
}

OSPri osGetThreadPri(OSThread* thread) {
    (void)thread;
    return 0;
}

void osYieldThread(void) {
    // No-op
}

// ============================================================
// Message queue — single-threaded, no actual queuing needed
/* osCreateMesgQueue, osSendMesg, osRecvMesg, osJamMesg — provided by libultraship */
/* osSetEventMesg — provided by libultraship */

// ============================================================
// TLB — not applicable on PC
// ============================================================

s32 osMapTLB(s32 index, OSPageMask pm, void* vaddr, u32 evenpaddr, u32 oddpaddr, s32 asid) {
    (void)index; (void)pm; (void)vaddr;
    (void)evenpaddr; (void)oddpaddr; (void)asid;
    return 0;
}

void osUnmapTLB(s32 index) {
    (void)index;
}

void osUnmapTLBAll(void) {
    // No-op
}

// ============================================================
// SP (Signal Processor / RSP) — handled by Fast3D on PC
// ============================================================

void osSpTaskLoad(OSTask* tp) {
    (void)tp;
}

void osSpTaskStartGo(OSTask* tp) {
    (void)tp;
}

void osSpTaskYield(void) {
    // No-op
}

OSYieldResult osSpTaskYielded(OSTask* tp) {
    (void)tp;
    return 0;
}

// ============================================================
// Interrupts
// ============================================================

OSIntMask osSetIntMask(OSIntMask mask) {
    (void)mask;
    return 0;
}

u32 __osDisableInt(void) {
    return 0;
}

void __osRestoreInt(u32 mask) {
    (void)mask;
}

u32 __osGetCause(void) {
    return 0;
}

// ============================================================
// SI (Serial Interface) — controller I/O handled by libultraship
// ============================================================

s32 __osSiRawStartDma(s32 direction, void* dramAddr) {
    (void)direction; (void)dramAddr;
    return 0;
}

// ============================================================
// EPI extra functions (osEPiReadIo, osEPiWriteIo not in libultraship)
// ============================================================

s32 osEPiReadIo(OSPiHandle* pihandle, u32 devAddr, u32* data) {
    (void)pihandle; (void)devAddr;
    if (data) *data = 0;
    return 0;
}

s32 osEPiWriteIo(OSPiHandle* pihandle, u32 devAddr, u32 data) {
    (void)pihandle; (void)devAddr; (void)data;
    return 0;
}

// Must match libultra's table exactly: all game movement trig goes through these, and demos replay frame-exact.
#include "../src/os/sintable.inc.c"

s16 sins(u16 x) {
    s16 val;

    x >>= 4;

    if (x & 0x400) {
        val = sintable[0x3ff - (x & 0x3ff)];
    } else {
        val = sintable[x & 0x3ff];
    }

    if (x & 0x800) {
        return -val;
    } else {
        return val;
    }
}

s16 coss(u16 x) {
    return sins(x + 0x4000);
}

// Flash functions are defined above (lines 53-108), no duplicates needed
