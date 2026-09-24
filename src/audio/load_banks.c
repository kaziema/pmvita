#include "audio.h"
#include "audio/core.h"
#ifdef PORT
#include "port/endian.h"
#include <stdio.h>
#endif

void au_load_BK_headers(AuGlobals* globals, ALHeap* heap) {
    SBNFileEntry fileEntry;
    InitBankEntry buffer[INIT_BANK_BUFFER_SIZE];
    s32 i;

    au_read_rom(globals->bkFileListOffset, &buffer, globals->bkListLength);

#ifdef PORT
    // au_read_rom keeps data big-endian. Swap the u16 fileIndex field.
    // bankIndex and bankSet are u8 — no swap needed.
    for (i = 0; i < ARRAY_COUNT(buffer); i++) {
        buffer[i].fileIndex = bswap16(buffer[i].fileIndex);
        if (buffer[i].fileIndex == 0xFFFF) {
            break;
        }
    }
#endif

    for (i = 0; i < ARRAY_COUNT(buffer); i++) {
        if (buffer[i].fileIndex == 0xFFFF) {
            break;
        }

#ifdef PORT
        if (au_fetch_SBN_file(buffer[i].fileIndex, AU_FMT_BK, &fileEntry) != AU_RESULT_OK) {
            fprintf(stderr, "[au_load_BK_headers] fetch FAILED for fileIndex=%u\n", buffer[i].fileIndex);
            continue;
        }
#else
        au_fetch_SBN_file(buffer[i].fileIndex, AU_FMT_BK, &fileEntry);
#endif
        au_load_BK_to_bank(fileEntry.offset, nullptr, buffer[i].bankIndex, buffer[i].bankSet);
    }
}
