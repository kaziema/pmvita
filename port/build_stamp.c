// Recompiled on every build (see the build_stamp rule in Makefile.vita) so the stamp printed
// at boot always matches the binary. Decoding a crash dump needs the exact binary that made it.
const char gPortBuildStamp[] = __DATE__ " " __TIME__;

// vita-elf-create appends its SCE data after the read-only segment, and the linker script puts
// the data segment at the next 64K boundary. Once the binary grew past 0x8183EED4 that gap was
// 36 bytes too small to fit and the build died at the .velf step. This pushes the read-only
// segment over the boundary so the gap is a full 64K again.
const volatile char gPortLinkPad[0x4000] = { 1 };
