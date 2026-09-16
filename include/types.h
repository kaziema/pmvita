#ifndef _TYPES_H_
#define _TYPES_H_

#include "ultra64.h"

#include <stddef.h>
#include <stdint.h>

#ifndef __cplusplus
// Macro wins over vitasdk's native C23 nullptr keyword, so this decomp's
// assumption that nullptr is a plain (void*)0 still holds on Vita.
#define nullptr ((void*)0)
// vitasdk's GCC already provides nullptr_t natively (C23 feature).
#ifndef __vita__
typedef void* nullptr_t;
#endif
#endif

#define UNK_TYPE s32
#define UNK_PTR void*
#define UNK_RET void
#define UNK_FUN_ARG void(*)(void)
#define UNK_FUN_PTR(name) void(*name)(void)
#define UNK_ARGS

// PrintCallback is provided by libultraship (libultra/printf.h)
// with signature: typedef void* (*PrintCallback)(void*, const char*, u32);

/// Linker symbol address, as in `ld_addrs.h`.
/// On N64, these are incomplete array types whose addresses are set by the linker.
/// On PC, they remain as zero-length arrays; dma_copy resolves real ROM offsets at runtime.
typedef u8 Addr[];

#define BTL_NONE 0
#define BTL_DEFAULT_STAGE -1

/// Terminates an extraAnimationList
#define ANIM_LIST_END -1

#endif
