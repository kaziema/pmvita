#include "Engine.h"
#include "build.h"
#include "cvar_prefixes.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/audiobridge.h>
#include <fast/Fast3dWindow.h>
#include <fast/interpreter.h>
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>

#include <fast/resource/ResourceType.h>
#include <ship/window/gui/Fonts.h>
#include <fast/resource/factory/DisplayListFactory.h>
#include <fast/resource/factory/TextureFactory.h>
#include <fast/resource/factory/MatrixFactory.h>
#include <fast/resource/factory/VertexFactory.h>
#include <fast/resource/factory/LightFactory.h>
#include <ship/resource/factory/BlobFactory.h>
#include <ship/utils/StringHelper.h>
#include <ship/resource/ResourceType.h>
#include <ship/resource/archive/O2rArchive.h>
#include <ship/utils/binarytools/MemoryStream.h>
#include <ship/utils/binarytools/BinaryReader.h>
#include <ship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h>
#include <libultraship/controller/controldeck/ControlDeck.h>

#ifdef __SWITCH__
#include <ship/port/switch/SwitchImpl.h>
#endif

#include "ui/PaperShipMenu.h"

std::shared_ptr<Fast::Fast3dWindow> psFast3dWindow;
bool portArchiveVersionMatch = false;
std::string assets_path;

namespace fs = std::filesystem;

extern "C" {
// Paper Mario game headers
bool prevAltAssets = false;

// Audio system functions from Paper Mario decomp
// Acmd is a union of audio command structs (8 bytes each on N64).
// With the software mixer, commands execute immediately and the Acmd* just advances.
// We allocate a raw buffer so the pointer arithmetic works without needing the full type.
typedef union { long long int force_align; uint32_t words[2]; } Acmd;
Acmd* alAudioFrame(Acmd* cmdList, int32_t* cmdLen, int16_t* outBuf, int32_t outLen);
void nuAuCleanDMABuffers(void);

}

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
} OTRVersion;

GameEngine* GameEngine::Instance;
AudioState GameEngine::audio;

// Stub for frame interpolation — Paper Mario interpolation not yet implemented.
// Returns an empty matrix replacement map (no interpolation applied).
static std::unordered_map<Mtx*, MtxF> FrameInterpolation_Interpolate(float /*step*/) {
    return {};
}

// Create LUS::ControlDeck with keyboard and gamepad mappings for Paper Mario 64.
// Follows SpaghettiKart/Ghostship pattern — LUS::ControlDeck provides WriteToPad
// that reads from SDL (keyboard + any connected gamepad including PS5 DualSense).
static std::shared_ptr<LUS::ControlDeck> CreateControlDeck() {
    auto defaultMappings = std::make_shared<Ship::ControllerDefaultMappings>(
        // Keyboard → N64 button mappings
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<Ship::KbScancode>>{
            { BTN_A,      { Ship::KbScancode::LUS_KB_X } },
            { BTN_B,      { Ship::KbScancode::LUS_KB_C } },
            { BTN_Z,      { Ship::KbScancode::LUS_KB_Z } },
            { BTN_L,      { Ship::KbScancode::LUS_KB_Q } },
            { BTN_R,      { Ship::KbScancode::LUS_KB_E } },
            { BTN_START,  { Ship::KbScancode::LUS_KB_ENTER } },
            { BTN_CUP,    { Ship::KbScancode::LUS_KB_I } },
            { BTN_CDOWN,  { Ship::KbScancode::LUS_KB_K } },
            { BTN_CLEFT,  { Ship::KbScancode::LUS_KB_J } },
            { BTN_CRIGHT, { Ship::KbScancode::LUS_KB_L } },
            { BTN_DUP,    { Ship::KbScancode::LUS_KB_ARROWKEY_UP } },
            { BTN_DDOWN,  { Ship::KbScancode::LUS_KB_ARROWKEY_DOWN } },
            { BTN_DLEFT,  { Ship::KbScancode::LUS_KB_ARROWKEY_LEFT } },
            { BTN_DRIGHT, { Ship::KbScancode::LUS_KB_ARROWKEY_RIGHT } }
        },
        // Keyboard → analog stick mappings (WASD for left stick)
        std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, Ship::KbScancode>>>{
            { Ship::StickIndex::LEFT_STICK, {
                { Ship::Direction::UP,    Ship::KbScancode::LUS_KB_W },
                { Ship::Direction::DOWN,  Ship::KbScancode::LUS_KB_S },
                { Ship::Direction::LEFT,  Ship::KbScancode::LUS_KB_A },
                { Ship::Direction::RIGHT, Ship::KbScancode::LUS_KB_D }
            }}
        },
        // SDL gamepad → N64 button mappings (works with PS5 DualSense, Xbox, etc.)
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>{
            { BTN_A,      { SDL_CONTROLLER_BUTTON_A } },
            { BTN_B,      { SDL_CONTROLLER_BUTTON_X } },
            { BTN_START,  { SDL_CONTROLLER_BUTTON_START } },
            { BTN_CUP,    { SDL_CONTROLLER_BUTTON_RIGHTSTICK } },
            { BTN_CLEFT,  { SDL_CONTROLLER_BUTTON_Y } },
            { BTN_CDOWN,  { SDL_CONTROLLER_BUTTON_B } },
            { BTN_DUP,    { SDL_CONTROLLER_BUTTON_DPAD_UP } },
            { BTN_DDOWN,  { SDL_CONTROLLER_BUTTON_DPAD_DOWN } },
            { BTN_DLEFT,  { SDL_CONTROLLER_BUTTON_DPAD_LEFT } },
            { BTN_DRIGHT, { SDL_CONTROLLER_BUTTON_DPAD_RIGHT } },
            { BTN_R,      { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER } },
            { BTN_L,      { SDL_CONTROLLER_BUTTON_LEFTSHOULDER } }
        },
        // SDL button → stick axis (unused, use defaults)
        std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, SDL_GameControllerButton>>>(),
        // SDL axis → N64 button mappings (triggers and right stick)
        std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>{
            { BTN_Z,      { { SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1 } } },
            { BTN_R,      { { SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1 } } },
            { BTN_CUP,    { { SDL_CONTROLLER_AXIS_RIGHTY, -1 } } },
            { BTN_CDOWN,  { { SDL_CONTROLLER_AXIS_RIGHTY, 1 } } },
            { BTN_CLEFT,  { { SDL_CONTROLLER_AXIS_RIGHTX, -1 } } },
            { BTN_CRIGHT, { { SDL_CONTROLLER_AXIS_RIGHTX, 1 } } }
        },
        // SDL axis → stick axis (use defaults — left stick mapped automatically)
        std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>()
    );

    auto buttonNames = std::unordered_map<CONTROLLERBUTTONS_T, std::string>{
        { BTN_A,      "A" },
        { BTN_B,      "B" },
        { BTN_L,      "L" },
        { BTN_R,      "R" },
        { BTN_Z,      "Z" },
        { BTN_START,  "Start" },
        { BTN_CLEFT,  "C-Left" },
        { BTN_CRIGHT, "C-Right" },
        { BTN_CUP,    "C-Up" },
        { BTN_CDOWN,  "C-Down" },
        { BTN_DLEFT,  "D-Left" },
        { BTN_DRIGHT, "D-Right" },
        { BTN_DUP,    "D-Up" },
        { BTN_DDOWN,  "D-Down" },
    };

    return std::make_shared<LUS::ControlDeck>(
        std::vector<CONTROLLERBUTTONS_T>(), defaultMappings, buttonNames);
}

OTRVersion ReadPortVersionFromOTR(std::string otrPath) {
    OTRVersion version = {};
    auto archive = std::make_shared<Ship::O2rArchive>(otrPath);
    if (archive->Open()) {
        auto t = archive->LoadFile("portVersion");
        if (t != nullptr && t->IsLoaded) {
            auto stream = std::make_shared<Ship::MemoryStream>(t->Buffer->data(), t->Buffer->size());
            auto reader = std::make_shared<Ship::BinaryReader>(stream);
            reader->SetEndianness(Ship::Endianness::Big);
            version.major = reader->ReadUInt16();
            version.minor = reader->ReadUInt16();
            version.patch = reader->ReadUInt16();
        }
    }
    return version;
}

OTRVersion DetectOTRVersion(std::string fileName) {
    std::string otrPath = Ship::Context::LocateFileAcrossAppDirs(fileName);
    if (!std::filesystem::exists(otrPath)) {
        return { INT16_MAX, INT16_MAX, INT16_MAX };
    }
    return ReadPortVersionFromOTR(otrPath);
}

bool VerifyArchiveVersion(OTRVersion version) {
    return version.major == gBuildVersionMajor && version.minor == gBuildVersionMinor;
}

GameEngine::GameEngine() {
    fprintf(stderr, "[PaperShip] GameEngine: CreateUninitializedInstance\n");
    this->context = Ship::Context::CreateUninitializedInstance("PaperShip", "pm64", "papership.cfg.json");

#ifdef __SWITCH__
    Ship::Switch::Init(Ship::PreInitPhase);
    Ship::Switch::Init(Ship::PostInitPhase);
#endif

    fprintf(stderr, "[PaperShip] GameEngine: InitConfiguration\n");
    this->context->InitConfiguration();
    fprintf(stderr, "[PaperShip] GameEngine: InitConsoleVariables\n");
    this->context->InitConsoleVariables();

    fprintf(stderr, "[PaperShip] GameEngine: LocateFileAcrossAppDirs\n");
    assets_path = Ship::Context::LocateFileAcrossAppDirs("papership.o2r");
    portArchiveVersionMatch = std::filesystem::exists(assets_path);

    fprintf(stderr, "[PaperShip] GameEngine: InitControlDeck\n");
    auto controlDeck = CreateControlDeck();
    this->context->InitControlDeck(controlDeck);
    fprintf(stderr, "[PaperShip] GameEngine: InitResourceManager\n");
    // Only pass archive paths that actually exist to avoid "OTR not found" popup
    std::vector<std::string> archivePaths;
    if (portArchiveVersionMatch) {
        archivePaths.push_back(assets_path);
    }
    this->context->InitResourceManager(archivePaths, {}, 3);
    fprintf(stderr, "[PaperShip] GameEngine: InitConsole\n");
    this->context->InitConsole();

    fprintf(stderr, "[PaperShip] GameEngine: creating Fast3dWindow\n");
    psFast3dWindow = std::make_shared<Fast::Fast3dWindow>(std::vector<std::shared_ptr<Ship::GuiWindow>>({}));
    fprintf(stderr, "[PaperShip] GameEngine: InitWindow\n");
    this->context->InitWindow(psFast3dWindow);
    fprintf(stderr, "[PaperShip] GameEngine: constructor done\n");

    PaperShipGui::SetupMenu();
}

void GameEngine::FinishInit() {
    fprintf(stderr, "[PaperShip] FinishInit: start\n");
    // Load ROM-extracted assets
    std::string romPath = Ship::Context::LocateFileAcrossAppDirs("pm64.o2r", "pm64");
    if (std::filesystem::exists(romPath)) {
        context->GetResourceManager()->GetArchiveManager()->AddArchive(romPath);
    }

    // Load user mods from mods/ directory
    const std::string patches_path = Ship::Context::GetPathRelativeToAppDirectory("mods");
    if (!patches_path.empty()) {
        if (!std::filesystem::exists(patches_path)) {
            std::filesystem::create_directories(patches_path);
        }
        if (std::filesystem::is_directory(patches_path)) {
            for (const auto& p : std::filesystem::recursive_directory_iterator(patches_path)) {
                const auto ext = p.path().extension().string();
                if (StringHelper::IEquals(ext, ".otr") || StringHelper::IEquals(ext, ".o2r")) {
                    Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->AddArchive(
                        p.path().generic_string());
                }
            }
        }
    }

#if (_DEBUG)
    auto defaultLogLevel = spdlog::level::debug;
#else
    auto defaultLogLevel = spdlog::level::info;
#endif
    auto logLevel =
        static_cast<spdlog::level::level_enum>(CVarGetInteger(CVAR_DEVELOPER_TOOLS("LogLevel"), defaultLogLevel));
    context->InitLogging(logLevel, logLevel);
    Ship::Context::GetInstance()->GetLogger()->set_pattern("[%H:%M:%S.%e] [%s:%#] [%l] %v");
    SPDLOG_INFO("Starting PaperShip version {} (Branch: {} | Commit: {})", (char*)gBuildVersion, (char*)gGitBranch,
                (char*)gGitCommitHash);
    fprintf(stderr, "[PaperShip] FinishInit: logging initialized\n");

    fprintf(stderr, "[PaperShip] FinishInit: InitGfxDebugger\n");
    context->InitGfxDebugger();
    context->InitFileDropMgr();
    context->InitCrashHandler();

    // Paper Mario audio: 32000Hz sample rate.
    // DesiredBuffered=2200 (~69ms, ~2 frames) is the adaptive buffer threshold.
    // When buffered samples drop below this, generate SAMPLES_HIGH (1104).
    // CoreAudio ring buffer is 6000 samples; keeping 2200 as threshold gives
    // plenty of headroom during LOW phases (920 samples/frame drain rate).
    fprintf(stderr, "[PaperShip] FinishInit: InitAudio\n");
    this->context->InitAudio({ .SampleRate = 32000, .SampleLength = 544, .DesiredBuffered = 2200 });
    fprintf(stderr, "[PaperShip] FinishInit: audio initialized\n");

    psFast3dWindow->SetTargetFps(PM64_GAME_FPS);
    psFast3dWindow->SetMaximumFrameLatency(1);
    psFast3dWindow->SetRendererUCode(ucode_f3dex2);

    auto loader = context->GetResourceManager()->GetResourceLoader();
    auto blobFactory = std::make_shared<Ship::ResourceFactoryBinaryBlobV0>();

    // Register standard libultraship resource factories
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 1);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY,
                                    "Vertex", static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryDisplayListV0>(),
                                    RESOURCE_FORMAT_BINARY, "DisplayList",
                                    static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryMatrixV0>(), RESOURCE_FORMAT_BINARY,
                                    "Matrix", static_cast<uint32_t>(Fast::ResourceType::Matrix), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryLightV0>(), RESOURCE_FORMAT_BINARY,
                                    "Light", static_cast<uint32_t>(Fast::ResourceType::Light), 0);
    loader->RegisterResourceFactory(blobFactory, RESOURCE_FORMAT_BINARY, "Blob",
                                    static_cast<uint32_t>(Ship::ResourceType::Blob), 0);

    // TODO: Register Paper Mario-specific resource factories:
    // loader->RegisterResourceFactory(std::make_shared<PM64::SpriteFactoryV0>(), ...);
    // loader->RegisterResourceFactory(std::make_shared<PM64::MapDataFactoryV0>(), ...);
    // loader->RegisterResourceFactory(std::make_shared<PM64::EvtScriptFactoryV0>(), ...);
    // loader->RegisterResourceFactory(std::make_shared<PM64::AudioBankFactoryV0>(), ...);
    // loader->RegisterResourceFactory(std::make_shared<PM64::MessageFactoryV0>(), ...);

    prevAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 1);
    context->GetResourceManager()->SetAltAssetsEnabled(prevAltAssets);

    fprintf(stderr, "[PaperShip] FinishInit: done\n");
    // TODO: PaperShipGui::SetupGuiElements();
    Instance->AudioInit();
}

void GameEngine::Create(int argc, char* argv[]) {
    const auto instance = Instance = new GameEngine();
    instance->RunExtract(argc, argv);
    instance->FinishInit();
}

void GameEngine::Destroy() {
    psFast3dWindow = nullptr;
    Instance->context = nullptr;
    AudioExit();
}

void GameEngine::StartFrame() const {
    using Ship::KbScancode;
    const int32_t dwScancode = this->context->GetWindow()->GetLastScancode();
    this->context->GetWindow()->SetLastScancode(-1);

    switch (dwScancode) {
        case KbScancode::LUS_KB_TAB: {
            CVarSetInteger("gEnhancements.Mods.AlternateAssets",
                           !CVarGetInteger("gEnhancements.Mods.AlternateAssets", 1));
            break;
        }
        default:
            break;
    }
}

uint32_t GameEngine::GetInterpolationFPS() {
    if (CVarGetInteger(CVAR_SETTING("MatchRefreshRate"), 0)) {
        return Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate();
    } else if (CVarGetInteger(CVAR_VSYNC_ENABLED, 1) ||
               !Ship::Context::GetInstance()->GetWindow()->CanDisableVerticalSync()) {
        return std::min<uint32_t>(Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate(),
                                  CVarGetInteger(CVAR_SETTING("InterpolationFPS"), PM64_GAME_FPS));
    }
    return CVarGetInteger(CVAR_SETTING("InterpolationFPS"), PM64_GAME_FPS);
}

// Audio

void GameEngine::HandleAudioThread() {
    static Acmd audioCmdBuf[0x1000];

    int diag_frame = 0;
    int diag_underruns = 0;
    int diag_min_buffered = 999999;

    while (audio.running) {
        {
            std::unique_lock<std::mutex> Lock(audio.mutex);
            while (!audio.processing && audio.running) {
                audio.cv_to_thread.wait(Lock);
            }
            if (!audio.running) {
                break;
            }
        }

        // Adaptive buffer sizing (matches Ghostship/SM64 approach).
        int samples_left = AudioPlayerBuffered();
        int desired = AudioPlayerGetDesiredBuffered();
        uint32_t num_audio_samples = samples_left < desired ? SAMPLES_HIGH : SAMPLES_LOW;

        if (samples_left < diag_min_buffered) diag_min_buffered = samples_left;
        if (samples_left == 0) diag_underruns++;

        int16_t audio_buffer[SAMPLES_HIGH * NUM_AUDIO_CHANNELS];
        memset(audio_buffer, 0, sizeof(audio_buffer));

        int32_t cmdLen = 0;
        {
            std::unique_lock<std::mutex> Lock(audio.mutex);
            alAudioFrame(audioCmdBuf, &cmdLen, audio_buffer, num_audio_samples);
            nuAuCleanDMABuffers();
        }

        // Apply output gain
        float master_vol = CVarGetInteger("gSettings.Volume.Master", 100) / 100.0f;
        float output_gain = master_vol * 0.85f;

        for (uint32_t i = 0; i < num_audio_samples * NUM_AUDIO_CHANNELS; i++) {
            int32_t out = static_cast<int32_t>(audio_buffer[i] * output_gain);
            if (out > 32767) out = 32767;
            if (out < -32768) out = -32768;
            audio_buffer[i] = static_cast<int16_t>(out);
        }

        AudioPlayerPlayFrame((uint8_t*)audio_buffer, num_audio_samples * NUM_AUDIO_CHANNELS * sizeof(int16_t));

        // Print diagnostics every 5 seconds (150 frames at 30fps)
        diag_frame++;
        if (diag_frame % 150 == 0) {
            fprintf(stderr, "[AUDIO] f=%d buf=%d min=%d desired=%d underruns=%d\n",
                    diag_frame, samples_left, diag_min_buffered, desired, diag_underruns);
            diag_min_buffered = 999999;
            diag_underruns = 0;
        }

        {
            std::unique_lock<std::mutex> Lock(audio.mutex);
            audio.processing = false;
        }
        audio.cv_from_thread.notify_one();
    }
}

void GameEngine::StartAudioFrame() {
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        audio.processing = true;
    }
    audio.cv_to_thread.notify_one();
}

void GameEngine::EndAudioFrame() {
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        while (audio.processing) {
            audio.cv_from_thread.wait(Lock);
        }
    }
}

/**
 * Flush audio: wait for the audio thread to finish its current frame.
 * Call this before resetting the heap or freeing audio bank memory
 * to prevent the audio thread from reading freed data (SIGSEGV).
 * After this returns, the audio thread is idle until the next StartAudioFrame().
 */
extern "C" void port_flush_audio(void) {
    GameEngine::EndAudioFrame();
}

void GameEngine::AudioInit() {
    if (!audio.running) {
        audio.running = true;
        audio.thread = std::thread(HandleAudioThread);
    }
}

void GameEngine::AudioExit() {
    {
        std::unique_lock lock(audio.mutex);
        audio.running = false;
    }
    audio.cv_to_thread.notify_all();
    audio.thread.join();
}

void GameEngine::RunCommands(Gfx* Commands, const std::vector<std::unordered_map<Mtx*, MtxF>>& mtx_replacements) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    if (wnd == nullptr) {
        return;
    }

    auto interpreter = wnd->GetInterpreterWeak().lock().get();
    wnd->HandleEvents();

    interpreter->mInterpolationIndex = 0;
#ifdef __vita__
    static bool sLoggedFirstRun = false;
    if (!sLoggedFirstRun) {
        sLoggedFirstRun = true;
        fprintf(stderr, "[RunCommands] first call, %zu mtx_replacements\n", mtx_replacements.size());
    }
#endif
    for (const auto& mtxStack : mtx_replacements) {
        wnd->DrawAndRunGraphicsCommands(Commands, mtxStack);
        interpreter->mInterpolationIndex++;
    }

    bool curAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 1);
    if (prevAltAssets != curAltAssets) {
        prevAltAssets = curAltAssets;
        Ship::Context::GetInstance()->GetResourceManager()->SetAltAssetsEnabled(curAltAssets);
        gfx_texture_cache_clear();
    }
}

void GameEngine::ProcessGfxCommands(Gfx* commands) {
    std::vector<std::unordered_map<Mtx*, MtxF>> mtx_replacements;
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    int target_fps = GetInterpolationFPS();
    static int last_fps;
    static int time;
    int fps = target_fps;
    int original_fps = 60 / 2; // Paper Mario: 30fps NTSC

    if (target_fps == 30 || original_fps > target_fps) {
        fps = original_fps;
    }

    if (last_fps != fps) {
        time = 0;
    }

    int next_original_frame = fps;
    while (time + original_fps <= next_original_frame) {
        time += original_fps;
        if (time != next_original_frame) {
            mtx_replacements.push_back(FrameInterpolation_Interpolate((float)time / next_original_frame));
        } else {
            mtx_replacements.emplace_back();
        }
    }

    time -= fps;

    if (wnd != nullptr) {
        wnd->SetTargetFps(GetInterpolationFPS());
        wnd->SetMaximumFrameLatency(1);
    }
    RunCommands(commands, mtx_replacements);

    last_fps = fps;
}

bool GameEngine::IsAltAssetsEnabled() {
    return prevAltAssets;
}

void GameEngine::RunExtract(int argc, char* argv[]) {
    // TODO: Implement ROM extraction UI (similar to Ghostship's RunExtract)
    // For now, assume pm64.o2r already exists
    SPDLOG_INFO("Checking for PM64 archives...");

    std::string modsPath = Ship::Context::GetPathRelativeToAppDirectory("mods", "pm64");
    if (!std::filesystem::exists(modsPath)) {
        try {
            std::filesystem::create_directories(modsPath);
        } catch (std::filesystem::filesystem_error const& ex) {
            // Continue silently
        }
    }
}

// C-accessible functions

extern "C" uint32_t GameEngine_GetInterpolatedFPS() {
    return GameEngine::GetInterpolationFPS();
}

extern "C" uint32_t GameEngine_GetSampleRate() {
    auto player = Ship::Context::GetInstance()->GetAudio()->GetAudioPlayer();
    if (player == nullptr || !player->IsInitialized()) {
        return 0;
    }
    return player->GetSampleRate();
}

extern "C" uint32_t GameEngine_GetSamplesPerFrame() {
    return SAMPLES_PER_FRAME;
}

Fast::Interpreter* GameEngine_GetInterpreter() {
    return static_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow())
        ->GetInterpreterWeak()
        .lock()
        .get();
}

extern "C" float GameEngine_GetAspectRatio() {
    auto interpreter = GameEngine_GetInterpreter();
    return interpreter->mCurDimensions.aspect_ratio;
}

extern "C" uint32_t OTRGetCurrentWidth() {
    return GameEngine::Instance->context->GetWindow()->GetWidth();
}

extern "C" uint32_t OTRGetCurrentHeight() {
    return GameEngine::Instance->context->GetWindow()->GetHeight();
}

extern "C" float OTRGetHUDAspectRatio() {
    if (CVarGetInteger("gHUDAspectRatio.Enabled", 0) == 0 || CVarGetInteger("gHUDAspectRatio.X", 0) == 0 ||
        CVarGetInteger("gHUDAspectRatio.Y", 0) == 0) {
        return GameEngine_GetAspectRatio();
    }
    return ((float)CVarGetInteger("gHUDAspectRatio.X", 1) / (float)CVarGetInteger("gHUDAspectRatio.Y", 1));
}

extern "C" float OTRGetDimensionFromLeftEdge(float v) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 -
            interpreter->mNativeDimensions.height / 2 * interpreter->mCurDimensions.aspect_ratio + (v));
}

extern "C" float OTRGetDimensionFromRightEdge(float v) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 +
            interpreter->mNativeDimensions.height / 2 * interpreter->mCurDimensions.aspect_ratio - (v));
}

extern "C" float OTRGetDimensionFromLeftEdgeForcedAspect(float v, float aspectRatio) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 -
            interpreter->mNativeDimensions.height / 2 *
                (aspectRatio > 0 ? aspectRatio : interpreter->mCurDimensions.aspect_ratio) +
            (v));
}

extern "C" float OTRGetDimensionFromRightEdgeForcedAspect(float v, float aspectRatio) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 +
            interpreter->mNativeDimensions.height / 2 *
                (aspectRatio > 0 ? aspectRatio : interpreter->mCurDimensions.aspect_ratio) -
            (v));
}

extern "C" int16_t OTRGetRectDimensionFromLeftEdge(float v) {
    return ((int)floorf(OTRGetDimensionFromLeftEdge(v)));
}

extern "C" int16_t OTRGetRectDimensionFromRightEdge(float v) {
    return ((int)ceilf(OTRGetDimensionFromRightEdge(v)));
}

extern "C" int16_t OTRGetRectDimensionFromLeftEdgeForcedAspect(float v, float aspectRatio) {
    return ((int)floorf(OTRGetDimensionFromLeftEdgeForcedAspect(v, aspectRatio)));
}

extern "C" int16_t OTRGetRectDimensionFromRightEdgeForcedAspect(float v, float aspectRatio) {
    return ((int)ceilf(OTRGetDimensionFromRightEdgeForcedAspect(v, aspectRatio)));
}

extern "C" uint32_t OTRGetGameRenderWidth() {
    auto interpreter = GameEngine_GetInterpreter();
    return interpreter->mCurDimensions.width;
}

extern "C" uint32_t OTRGetGameRenderHeight() {
    auto interpreter = GameEngine_GetInterpreter();
    return interpreter->mCurDimensions.height;
}

extern "C" int32_t OTRConvertHUDXToScreenX(int32_t v) {
    auto interpreter = GameEngine_GetInterpreter();
    float gameAspectRatio = interpreter->mCurDimensions.aspect_ratio;
    int32_t gameHeight = interpreter->mCurDimensions.height;
    int32_t gameWidth = interpreter->mCurDimensions.width;
    float hudAspectRatio = 4.0f / 3.0f;
    int32_t hudHeight = gameHeight;
    int32_t hudWidth = hudHeight * hudAspectRatio;
    float hudScreenRatio = (hudWidth / 320.0f);
    float hudCoord = v * hudScreenRatio;
    float gameOffset = (gameWidth - hudWidth) / 2;
    float gameCoord = hudCoord + gameOffset;
    float gameScreenRatio = (320.0f / gameWidth);
    float screenScaledCoord = gameCoord * gameScreenRatio;
    return (int32_t)screenScaledCoord;
}

extern "C" uint32_t GameEngine_GetGameVersion() {
    return Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions()[0];
}

extern "C" bool GameEngine_OTRSigCheck(const char* data) {
    return Ship::Context::GetInstance()->GetResourceManager()->OtrSignatureCheck(data);
}
