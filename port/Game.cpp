/**
 * PaperShip - Paper Mario 64 PC Port
 * Game.cpp - PC entry point and main game loop
 *
 * This mirrors Ghostship/src/port/Game.cpp but adapted for Paper Mario's
 * NuSystem-based architecture. Key difference: SM64 uses thread5_iteration()
 * in its loop, while Paper Mario uses gfxRetrace_Callback() which internally
 * calls step_game_loop() + gfx_draw_frame().
 */
#include <libultraship.h>
#include <fast/interpreter.h>
#include <fast/Fast3dWindow.h>
#include "Engine.h"
#include "port_aspect.h"
#include <signal.h>
#include <chrono>
#include <thread>
#ifdef __APPLE__
#include <execinfo.h>
#endif
#ifdef __vita__
#include <vitasdk.h>
#include <pthread.h>
// vitasdk's default newlib heap is too small for this game. Ghostship and
// 2ship2harkinian both use this exact value on the same libultraship stack.
int _newlib_heap_size_user = 256 * 1024 * 1024;

// Keeps the code segment clear of a 64KB boundary vita-elf-create needs free space after.
extern "C" __attribute__((used)) const unsigned char port_text_segment_padding[24 * 1024] = { 0 };
#endif

extern "C" {
#include "common.h"
#include "nu/nusys.h"

// Forward declarations from Paper Mario decomp
void boot_main(void* data);
void gfxRetrace_Callback(s32 gfxTaskNum);
void gfxPreNMI_Callback(void);
void create_audio_system(void);
void load_engine_data(void);
void step_game_loop(void);
extern "C" const char gPortBuildStamp[];
void gfx_draw_frame(void);
void gfx_task_background(void);

// PORT: UI texture loader
void port_load_ui_textures(void);
void port_load_map_textures(void);

// Game mode
void set_game_mode(s16 mode);
s16 get_game_mode(void);
extern s32 gBattleState;
extern s32 gBattleSubState;

// Simulate N64 framebuffer cycling (NuSystemShims.cpp)
void port_cycle_framebuffer(void);

// Writes the save file if a save touched it this frame (os_stubs.c)
void port_flash_flush_if_dirty(void);

// Worker crash diagnostics
void worker_dump_last(void);

// Walks the heap node lists and reports the first inconsistency (43F0.c)
void port_heap_check_all(void);

// NuSystem state managed by our shim layer
extern NUGfxFunc nuGfxFunc;
extern u32 nuGfxDisplay;

// Externs from main_loop.c
extern s16 D_80073E0A;
extern s16 D_80073E08;

}

#ifdef __APPLE__
extern "C" void mac_prime_event_loop(void);
extern "C" int mac_capture_window_screenshot(const char* outputPath);
#endif

/**
 * nuGfxTaskStart replacement - routes display list to Fast3D renderer
 * This is called by gfx_draw_frame() and gfx_task_background() in the decomp
 * when they finish building a display list.
 */
/**
 * Paper Mario submits two display lists per frame:
 *   1. Background DL (NU_SC_NOSWAPBUFFER) — background fill, map backgrounds
 *   2. Main DL (NU_SC_SWAPBUFFER) — all game content
 *
 * ProcessGfxCommands always swaps/presents buffers (via Fast3D EndFrame).
 * Calling it twice per frame causes flickering (background flash → game content).
 *
 * Fix: Save the background DL, then combine both into a single wrapper DL
 * when the main DL arrives. One ProcessGfxCommands call = one swap per frame.
 */
static Gfx* sPendingBackgroundDl = nullptr;
static Gfx sCombinedDl[3];
static bool sFrameSubmitted = false;

// Minimal display list for empty frames (loading screens, etc.)
static Gfx sEmptyDl[2];

extern "C" void nuGfxTaskStart(Gfx* gfxList_ptr, u32 gfxListSize, u32 ucode, u32 flag) {
#ifdef __vita__
    {
        static bool sLoggedFirstTask = false;
        if (!sLoggedFirstTask) {
            sLoggedFirstTask = true;
            fprintf(stderr, "[nuGfxTaskStart] first call, flag=0x%x swap=%d\n", flag, (flag & NU_SC_SWAPBUFFER) != 0);
        }
    }
#endif
    if (!(flag & NU_SC_SWAPBUFFER)) {
        // Background DL — save for combining with the next main DL
        sPendingBackgroundDl = gfxList_ptr;
        return;
    }

    // Main DL with swap flag — combine with pending background and render once
    sFrameSubmitted = true;
    if (sPendingBackgroundDl != nullptr) {
        // Create wrapper DL: call background (push), then branch to main
        // Background DL ends with gSPEndDisplayList which pops back here,
        // then gSPBranchList jumps to main DL (no return needed).
        gSPDisplayList(&sCombinedDl[0], sPendingBackgroundDl);
        gSPBranchList(&sCombinedDl[1], gfxList_ptr);
        sPendingBackgroundDl = nullptr;
        GameEngine::ProcessGfxCommands(sCombinedDl);
    } else {
        GameEngine::ProcessGfxCommands(gfxList_ptr);
    }
}

/**
 * Paper Mario's frame pump - called every frame from the main loop.
 *
 * In the original game, gfxRetrace_Callback is driven by VBlank interrupts
 * via NuSystem's scheduler. On PC, we call it directly each frame.
 *
 * The callback has a toggle (D_80073E0A) that means it only runs game logic
 * every OTHER retrace (i.e., game runs at 30fps with 60fps retrace).
 * On PC we call it once per frame and manage timing through the interpolation system.
 */
static int sPushFrameCount = 0;

// Window aspect ratio for background scaling (see port_aspect.h)
float gPortWindowAspectRatio = 4.0f / 3.0f;

void push_frame() {
#ifdef __vita__
    {
        static bool sLoggedFirstFrame = false;
        if (!sLoggedFirstFrame) {
            sLoggedFirstFrame = true;
            fprintf(stderr, "[push_frame] first call\n");
        }
    }
#endif
    // Frame rate limiter: target 30fps using accumulative timing.
    // Sleep for the bulk of the wait, then spin-wait for the last ~2ms for accuracy.
    // This avoids the OS scheduler overshooting sleep_for() which was causing ~25fps.
    {
        static auto nextFrameTime = std::chrono::high_resolution_clock::now();
        static const auto frameDuration = std::chrono::microseconds(1000000 / PM64_GAME_FPS);
        auto now = std::chrono::high_resolution_clock::now();
        auto remaining = nextFrameTime - now;
        // Sleep for most of the wait (leave 2ms margin for spin-wait)
        if (remaining > std::chrono::milliseconds(2)) {
            std::this_thread::sleep_for(remaining - std::chrono::milliseconds(2));
        }
        // Spin-wait for the final portion (accurate to microsecond level)
        while (std::chrono::high_resolution_clock::now() < nextFrameTime) {
            // busy-wait
        }
        nextFrameTime += frameDuration;
        // If we've fallen more than one frame behind, reset to prevent catch-up burst
        now = std::chrono::high_resolution_clock::now();
        if (now - nextFrameTime > frameDuration) {
            nextFrameTime = now;
        }
    }

    sPushFrameCount++;

    // Time the work half of the frame (everything past the limiter) so a lag spike names itself.
    auto portWorkStart = std::chrono::high_resolution_clock::now();

#ifdef __vita__
    // Demo-freeze watchdog: logs script/battle state once a second while demoState is active.
    if (gGameStatusPtr != nullptr && gGameStatusPtr->demoState != 0) {
        static int sDemoWatchdogCount = 0;
        if (++sDemoWatchdogCount >= 30) {
            sDemoWatchdogCount = 0;
            fprintf(stderr,
                    "[demo-watch] demoState=%d mode=%d mainScriptID=%d gBattleState=%d gBattleSubState=%d "
                    "demoButtonInput=0x%X demoStickX=%d demoStickY=%d\n",
                    gGameStatusPtr->demoState, get_game_mode(), gGameStatusPtr->mainScriptID, gBattleState,
                    gBattleSubState, (unsigned)(uint16_t)gGameStatusPtr->demoButtonInput,
                    gGameStatusPtr->demoStickX, gGameStatusPtr->demoStickY);
        }
    }
#endif

#ifdef __vita__
    // Was defined but never called, so heap damage went unreported. Check once a second.
    {
        static int sHeapCheckCount = 0;
        if (++sHeapCheckCount >= 30) {
            sHeapCheckCount = 0;
            port_heap_check_all();
        }
    }
#endif

    // FPS diagnostic: print every 300 frames
    {
        static auto fpsStart = std::chrono::high_resolution_clock::now();
        static int fpsFrameCount = 0;
        fpsFrameCount++;
        if (fpsFrameCount >= 300) {
            auto fpsNow = std::chrono::high_resolution_clock::now();
            double elapsed_s = std::chrono::duration<double>(fpsNow - fpsStart).count();
            fprintf(stderr, "[FPS] %d frames in %.2fs = %.1f fps\n", fpsFrameCount, elapsed_s, fpsFrameCount / elapsed_s);
            fpsStart = fpsNow;
            fpsFrameCount = 0;
        }
    }

    port_flash_flush_if_dirty();

    // Simulate N64 framebuffer cycling — game code compares nuGfxCfb[N] vs
    // nuGfxCfb_ptr for synchronization (e.g., battle init waits for specific buffer).
    port_cycle_framebuffer();

    // Update window aspect ratio for background scaling
    {
        auto window = Ship::Context::GetInstance()->GetWindow();
        uint32_t w = window->GetWidth(), h = window->GetHeight();
        if (h > 0) gPortWindowAspectRatio = (float)w / (float)h;
    }

    GameEngine::StartAudioFrame();
    GameEngine::Instance->StartFrame();

    // Track whether the game submits a frame this iteration
    sFrameSubmitted = false;

    // Call Paper Mario's retrace callback with gfxTaskNum=0 (tasks available)
    gfxRetrace_Callback(0);

    // If the game didn't submit any display list (e.g. DISABLE_DRAW_FRAME during loading),
    // pump window events to keep the app responsive without rendering a frame,
    // and add a small sleep to avoid spinning the CPU.
    if (!sFrameSubmitted) {
        sPendingBackgroundDl = nullptr;
        auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(
            Ship::Context::GetInstance()->GetWindow());
        if (wnd) {
            wnd->HandleEvents();
        }
        // No display list submitted — sleep briefly to avoid CPU spin
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    GameEngine::EndAudioFrame();

    {
        static int sSlowLogged = 0;
        double ms = std::chrono::duration<double, std::milli>(
                        std::chrono::high_resolution_clock::now() - portWorkStart)
                        .count();
        if (ms > 40.0 && sSlowLogged < 120) {
            sSlowLogged++;
            fprintf(stderr, "[slow] %.1fms mode=%d ctx=%d submitted=%d script=%d frame=%d\n", ms, get_game_mode(),
                    gGameStatusPtr ? gGameStatusPtr->context : -1, sFrameSubmitted ? 1 : 0,
                    gGameStatusPtr ? gGameStatusPtr->mainScriptID : -1, sPushFrameCount);
        }
    }

#if 0 // Disabled: screenshot capture was causing ~100ms overhead per capture
#ifdef __APPLE__
    {
        bool doScreenshot = false;
        if (sPushFrameCount > 0 && sPushFrameCount <= 15000 && (sPushFrameCount % 10) == 0) {
            doScreenshot = true;
        }
        if (doScreenshot) {
            char path[256];
            snprintf(path, sizeof(path), "/tmp/papership_frame%05d.png", sPushFrameCount);
            mac_capture_window_screenshot(path);
        }
    }
#endif
#endif
}

/**
 * PC-adapted boot sequence.
 * The original boot_main() does NuSystem init then enters `while(true){}`.
 * We extract the init sequence and replace the infinite loop with our
 * window-driven main loop.
 */
void boot_main_pc() {
    // Stamp the build into every log. Decoding a crash dump needs the exact binary that
    // produced it, and a log with no build identity costs a whole test round to resolve.
    fprintf(stderr, "[PaperShip] build %s\n", gPortBuildStamp);
    fprintf(stderr, "[PaperShip] boot_main_pc: start\n");

    // Skip TV mode setup (osViSetMode) - libultraship handles display
    // Skip crash_screen_init() - we have our own crash handling

    // These are the key init calls from boot_main():
    // nuGfxInit() allocates dummy framebuffers (nuGfxCfb, nuGfxZBuffer, nuGfxCfb_ptr)
    // that the game references in GBI commands. Must be called before rendering!
    fprintf(stderr, "[PaperShip] boot_main_pc: nuGfxInit\n");
    nuGfxInit();
    fprintf(stderr, "[PaperShip] boot_main_pc: nuContInit\n");
    gGameStatusPtr->contBitPattern = nuContInit();

    // Create the audio system and load engine data
    fprintf(stderr, "[PaperShip] boot_main_pc: create_audio_system\n");
    create_audio_system();
    fprintf(stderr, "[PaperShip] boot_main_pc: load_engine_data\n");
    load_engine_data();

    // Load UI textures (window borders, file menu graphics) from ROM
    port_load_ui_textures();

    // Load map-specific textures (dgb_01 bridges, pra_31 stairs, dro_02 cards, blanket)
    port_load_map_textures();

    fprintf(stderr, "[PaperShip] boot_main_pc: init complete\n");

    // Set the retrace callback (stored by our shim, called in push_frame)
    nuGfxFuncSet((NUGfxFunc)gfxRetrace_Callback);
    nuGfxPreNMIFuncSet(gfxPreNMI_Callback);

    // Seed random
    gRandSeed += 12345; // osGetCount() replacement

    // Enable display
    nuGfxDisplayOn();
}

#ifdef __vita__
extern "C" void *vita_main(void *argv);
#endif

#ifdef _WIN32
int SDL_main(int argc, char** argv) {
#elif defined(__vita__)
int main(int argc, char* argv[]) {
    // Vita's main thread has a small default stack. Bump clocks, hand off
    // to a properly-stacked worker thread, same as Ghostship/2ship2harkinian.
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    sceIoMkdir("ux0:data/papership", 0777);
    // Has to exist before vitaGL initializes (Interpreter's constructor,
    // called during window setup below) -- it was only being created later
    // in gfx_sdl2.cpp's Init(), after vitaGL's first shader compile already
    // needed it. Matches Ghostship-vita's actual working order.
    sceIoMkdir("ux0:data/papership/shader_cache", 0777);

    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2 * 1024 * 1024);
    pthread_create(&t, &attr, vita_main, NULL);
    return sceKernelExitDeleteThread(0);
}

extern "C" void *vita_main(void *argv) {
#else
int main(int argc, char* argv[]) {
#endif
#ifdef __vita__
    // stderr goes nowhere on real hardware with nothing attached to catch
    // it -- redirect it to a file so every fprintf checkpoint below is
    // actually readable after the fact.
    freopen("ux0:data/papership/log.txt", "w", stderr);
#endif
    // Make stderr unbuffered so all log messages are visible immediately
    setvbuf(stderr, nullptr, _IONBF, 0);

    // Install crash handler for debugging with backtrace
    auto crashHandler = [](int sig) {
        const char* sigName = (sig == SIGSEGV) ? "SIGSEGV" : (sig == SIGBUS) ? "SIGBUS" : "SIGABRT";
        fprintf(stderr, "\n[CRASH] %s received!\n", sigName);
        // Dump worker and audio debug info
        worker_dump_last();
        // Also write to a file so crash info is always accessible
#ifdef __vita__
        FILE* crashFile = fopen("ux0:data/papership/crash.log", "w");
#else
        FILE* crashFile = fopen("/tmp/papership_crash.log", "w");
#endif
#ifdef __APPLE__
        void* callstack[64];
        int frames = backtrace(callstack, 64);
        char** symbols = backtrace_symbols(callstack, frames);
        fprintf(stderr, "Backtrace (%d frames):\n", frames);
        for (int i = 0; i < frames; i++) {
            fprintf(stderr, "  %s\n", symbols[i]);
            if (crashFile) fprintf(crashFile, "%s\n", symbols[i]);
        }
        free(symbols);
#endif
        if (crashFile) { fclose(crashFile); }
        fflush(stderr);
#ifdef __vita__
        // abort() lands here and would exit with no dump. Fault on purpose so the system
        // writes a .psp2dmp whose stack still shows who called abort().
        if (sig == SIGABRT) {
            volatile int* p = nullptr;
            *p = 0;
        }
#endif
        _exit(1);
    };
    signal(SIGSEGV, crashHandler);
    signal(SIGBUS, crashHandler);
    signal(SIGABRT, crashHandler);

    // Initialize the engine (creates Ship::Context, window, resource manager)
#ifdef __vita__
    // vita_main() only receives the pthread argv blob (unused), not real
    // argc/argv -- there's no command line on Vita.
    (void)argv;
    GameEngine::Create(0, nullptr);
#else
    GameEngine::Create(argc, argv);
#endif

    // Run Paper Mario's boot sequence (without the infinite loop)
    boot_main_pc();

    // On macOS, the first SDL_PumpEvents/PollEvent call blocks in the Cocoa
    // event loop (-[NSApp nextEventMatchingMask:untilDate:inMode:dequeue:])
    // waiting for a native event. Post a dummy NSEvent to wake it up.
#ifdef __APPLE__
    fprintf(stderr, "[main] calling mac_prime_event_loop...\n");
    mac_prime_event_loop();
    fprintf(stderr, "[main] mac_prime_event_loop returned, calling SDL_PollEvent...\n");
    {
        SDL_Event evt;
        SDL_PollEvent(&evt);
    }
    fprintf(stderr, "[main] macOS event system primed\n");
#endif

    // Main game loop - replaces the `while(true){}` in boot_main
    int loopCount = 0;
    while (WindowIsRunning()) {
        push_frame();
        loopCount++;
    }
    fprintf(stderr, "[main] exited after %d frames\n", loopCount);

    GameEngine::Instance->Destroy();
#ifdef __vita__
    return NULL;
#else
    return 0;
#endif
}
