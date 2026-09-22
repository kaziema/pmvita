# Paper Mario: PS Vita Port

This is built on top of [PaperShip](https://github.com/versacepapermario/papermario-pc-upload), the decomp-based port of Paper Mario 64 running on [libultraship](https://github.com/Kenix3/libultraship), retargeted for PS Vita hardware.

## Flowers Where They're Due

This project stands entirely on other people's work and I want to say that up front:

- **[versacepapermario](https://github.com/versacepapermario)** built PaperShip, which this is built from. All the hard decomp-to-modern-hardware bridging work (the PORT layer, the audio bridge, the UI, the ROM loading) is theirs.
- **The [Paper Mario Decompilation Team](https://github.com/pmret/papermario)** did the actual multi-year reverse engineering that made any of this possible.
- **[Rinnegatamante](https://github.com/Rinnegatamante)**'s vitaGL, and their Vita ports of Ghostship (SM64) and 2ship2harkinian (OoT/MM) on this exact same libultraship stack, are the whole reason the Vita side of this is even possible. The Vita-specific patches in this repo (vitaGL init, NEON math hooks, SDL2/GL backend adjustments) are lifted straight from that work and adapted for Paper Mario.
- **[Kenix3](https://github.com/Kenix3)** and the libultraship / Ship of Harkinian team for the rendering engine all of this is built on.

If you're one of these people reading this, thank you!

## Legal Notice

This is a clean-room port built entirely from the publicly available [Paper Mario decompilation](https://github.com/pmret/papermario), same legal approach as [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) and [Ghostship](https://github.com/HarbourMasters/Ghostship).

- **This repository contains no ROM data.** No game textures, no audio, no models.
- All game assets are read at runtime from a ROM you provide yourself.
- I don't condone piracy. Bring your own legally obtained copy.

## Installation

You'll need a Vita that's already homebrew-enabled (h-encore/HENkaku) with VitaShell installed. That part isn't covered here.

1. On your Vita, open VitaShell and go to `ux0:/data/`.
2. Press **Triangle**, choose **New**, and name the new folder `papership`. You should now have `ux0:data/papership`.
3. Get three files into that `papership` folder (FTP or USB transfer with VitaShell, whichever you normally use to move files onto the memory card):
   - `PaperMarioVita.vpk`
   - Your own ROM, named exactly one of: `Paper Mario (USA).z64`, `baserom.us.z64`, `pm64.z64`, or `papermario.z64`. Same rule as the Legal Notice above, your own dump, not a downloaded one.
   - `papership.o2r`, which holds the shader templates. The game won't render without it.
4. Back in VitaShell, go into `ux0:data/papership`, highlight `PaperMarioVita.vpk`, and press **X**. Press **X** again to accept the extended-permissions prompt. That installs the app. The `.vpk` file itself can stay in the folder or be deleted afterward, doesn't matter.
5. Launch Paper Mario from the LiveArea like any other app. First boot creates `log.txt`, `crash.log`, and a `shader_cache` folder in `ux0:data/papership` on its own. You don't need to make those.

## Updating

Drop the new `PaperMarioVita.vpk` into `ux0:data/papership`, highlight it in VitaShell, and press **X** twice like the first install. Every build uses the same Title ID, so this installs over the old version in place.

Your save is safe either way. It lives at `ux0:data/papership/papership_save.bin`, and isn't part of the app package, so installing an update or even deleting the app doesn't touch it.

## Current Status

Done so far:
- `Makefile.vita` wired end to end, producing a working VPK
- Fixed a heap-sizing bug where Vita would have silently inherited 64-bit desktop heap sizes instead of the correct 32-bit N64-original ones
- Vita boot path wired into the entry point: heap allocation, max CPU/GPU clocks, game loop moved onto a properly-stacked worker thread
- libultraship `__vita__` patches ported in from Ghostship/2ship2harkinian: vitaGL scratch-buffer lifecycle, ARM NEON math, SDL2 input/window/framerate backend, OpenGL backend adjustments
- Boots to gameplay and renders on hardware at roughly 30fps, with occasional frame hiccups still to chase down
- LiveArea art and GitHub/Discord link buttons
- Widescreen (16:9) rendering: the 3D world, backgrounds, and the intro's curtains and starry-sky scenes fill the screen instead of being boxed at 4:3
- Controls mapped to the Vita: face buttons and D-pad for A/B/Z/D-pad, right stick for the four C buttons, no face button doubles as a C direction
- Saving and loading, to `ux0:data/papership/papership_save.bin` on the memory card, independent of the installed app
- Fixed several crashes traced to a corrupted intro script variable, a full background display list buffer, and an unguarded assertion macro that gave no diagnostic on failure

Still to do:
- The intro's staged cutscene (Bowser, Kammy, and the Star Rod, between the curtains) still renders boxed at 4:3 inside the widescreen frame
- Broader testing across areas, battles, and menus beyond the first area
- Remaining performance hiccups
- General playthrough coverage

## Known Issues

- The intro's staged cutscene (the theater scene with character models, not the starry-sky narration) renders at 4:3 inside the wider frame around it.
- Menus, text boxes, and the HUD stay at 4:3 by design; only the 3D world and full-frame backgrounds go widescreen.
- Backgrounds that use the wavy-effect renderer (a handful of areas) aren't widened yet.
- A crash was found in the title screen's idle attract-mode demo playback, most likely heap exhaustion when creating an animated NPC. Assertion failures now log which one fired and where before aborting, but the underlying cause isn't confirmed fixed.
- Noticeable stutter during some area transitions (e.g. mid-intro, when the cutscene changes areas). This looks like real load time reading assets off the memory card rather than a bug, but hasn't been optimized.
- Battles, chapters, shops, partners, and minigames are untested.

## Building

```
make -f Makefile.vita
```

Produces `PaperMarioVita.vpk`. `papership.o2r` isn't part of that build. It's packed separately, from `assets/port`, using the vendored `Torch` tool (built and run natively on your desktop, not for Vita):

```
cd Torch && cmake -B build && cmake --build build
./build/torch pack ../assets/port ../papership.o2r o2r -u 0.1.0
```

You'll need a **US** Paper Mario ROM in `.z64` format to actually run the game.

| Version | SHA-1 |
|---------|-------|
| US | `3837f44cda784b466c9a2d99df70d77c322b97a0` |

## Requirements

- A homebrew-enabled PS Vita or PS TV
- [VitaSDK](https://vitasdk.org/)
- vitaGL, vitaShaRK, math-neon (all available through `vdpm`)

## Layout

```
├── src/              # Decomp game source (C) with #ifdef PORT adaptations
├── include/          # Game headers
├── port/             # PORT layer: OS stubs, ROM loading, texture conversion,
│                     #   UI, shape swizzling, audio bridge, Vita entry point
├── libultraship/     # Rendering engine, carrying the Vita patches
└── assets/           # Asset YAML definitions (non-copyrighted metadata)
```

## Credits

- **[kaziema](https://github.com/kaziema)**: PS Vita port
- **[versacepapermario](https://github.com/versacepapermario)**: PaperShip, the port this is built from
- [Paper Mario Decompilation Team](https://github.com/pmret/papermario): the decomp
- [Rinnegatamante](https://github.com/Rinnegatamante): vitaGL, and the Vita ports this platform layer is built from
- [libultraship / Ship of Harkinian Team](https://github.com/HarbourMasters): rendering engine
