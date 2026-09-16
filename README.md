# Paper Mario: PS Vita Port

A lot of N64 emulation on Vita is absolute bunk, so a native port of Paper Mario was my next project.

This is built on top of [PaperShip](https://github.com/versacepapermario/papermario-pc-upload), the decomp-based port of Paper Mario 64 running on [libultraship](https://github.com/Kenix3/libultraship), retargeted for PS Vita hardware.

## Flowers where they're due

This project stands entirely on other people's work and I want to say that up front:

- **[versacepapermario](https://github.com/versacepapermario)** built PaperShip, which this is built from. All the hard decomp-to-modern-hardware bridging work (the PORT layer, the audio bridge, the UI, the ROM loading) is theirs.
- **The [Paper Mario Decompilation Team](https://github.com/pmret/papermario)** did the actual multi-year reverse engineering that made any of this possible.
- **[Rinnegatamante](https://github.com/Rinnegatamante)**'s vitaGL, and their Vita ports of Ghostship (SM64) and 2ship2harkinian (OoT/MM) on this exact same libultraship stack, are the whole reason the Vita side of this is even possible. The Vita-specific patches in this repo (vitaGL init, NEON math hooks, SDL2/GL backend adjustments) are lifted straight from that work and adapted for Paper Mario.
- **[Kenix3](https://github.com/Kenix3)** and the libultraship / Ship of Harkinian team for the rendering engine all of this is built on.

If you're one of these people reading this, thank you!!

## Legal Notice

This is a clean-room port built entirely from the publicly available [Paper Mario decompilation](https://github.com/pmret/papermario), same legal approach as [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) and [Ghostship](https://github.com/HarbourMasters/Ghostship).

- **This repository contains NO copyrighted Nintendo assets.** No ROM data, no textures, no audio, no models.
- All game assets are read at runtime from a ROM you provide yourself.
- I don't condone piracy. Bring your own legally obtained copy.

## Current Status

**Not playable yet.** Zero hours on real hardware, no Vita build target exists, nothing has been compiled for Vita. This is early groundwork.

Done so far:
- Fixed a heap-sizing bug where Vita would have silently inherited 64-bit desktop heap sizes instead of the correct 32-bit N64-original ones
- Vita boot path wired into the entry point: heap allocation, max CPU/GPU clocks, game loop moved onto a properly-stacked worker thread
- libultraship `__vita__` patches ported in from Ghostship/2ship2harkinian: vitaGL scratch-buffer lifecycle, ARM NEON math, SDL2 input/window/framerate backend, OpenGL backend adjustments

Still to do:
- `Makefile.vita` / toolchain wiring, so there's actually something to build
- Two shader-cache-related `gfx_opengl.cpp` patches from the reference ports (that part of the file has drifted too far for a safe direct port)
- Everything after that: first boot, asset pipeline on-device, controls, performance work

## Building

Nothing to build yet. Once the Vita build target lands, instructions go here.

You'll need a **US** Paper Mario ROM in `.z64` format when that time comes.

| Version | SHA-1 |
|---------|-------|
| US | `3837f44cda784b466c9a2d99df70d77c322b97a0` |

## Requirements (eventual)

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

- **[kaziema](https://github.com/kaziema)** — PS Vita port
- **[versacepapermario](https://github.com/versacepapermario)** — PaperShip, the port this is built from
- [Paper Mario Decompilation Team](https://github.com/pmret/papermario) — the decomp
- [Rinnegatamante](https://github.com/Rinnegatamante) — vitaGL, and the Vita ports this platform layer is built from
- [libultraship / Ship of Harkinian Team](https://github.com/HarbourMasters) — rendering engine
