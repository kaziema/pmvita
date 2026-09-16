// Static substitute for build.c.in -- this build has no CMake configure step
// to fill in version/git info, so it's hardcoded here instead.
#include <libultraship/libultra.h>

const char gBuildVersion[] = "PS Vita port (0.1.0)";
const u16 gBuildVersionMajor = 0;
const u16 gBuildVersionMinor = 1;
const u16 gBuildVersionPatch = 0;

const char gGitBranch[] = "";
const char gGitCommitHash[] = "";
const char gGitCommitTag[] = "";

const char gBuildTeam[] = "PaperShip Team";
const char gBuildDate[] = __DATE__ " " __TIME__;
const char gBuildMakeOption[] = "";
