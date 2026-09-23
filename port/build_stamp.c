// Recompiled on every build (see the build_stamp rule in Makefile.vita) so the stamp printed
// at boot always matches the binary. Decoding a crash dump needs the exact binary that made it.
const char gPortBuildStamp[] = __DATE__ " " __TIME__;
