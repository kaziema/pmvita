#include "common.h"
#ifdef PORT
#include <stdio.h>
extern void port_heap_check_all(void);
extern void* volatile* gPortWatchAddr;
extern void* gPortWatchVal;
extern void port_watch_check(const char* step);
#endif

#define INTRO_MSG_BLANK -1

enum {
    INTRO_MSG_STATE_APPEAR    = 0,
    INTRO_MSG_STATE_SHOWING   = 1,
    INTRO_MSG_STATE_VANISH    = 2,
    INTRO_MSG_STATE_DONE      = 3,
    INTRO_MSG_STATE_BLANK     = 4,
};

typedef struct IntroMessage {
    /* 00 */ s32 messageID;
    /* 04 */ s32 duration;
} IntroMessage; // size: 0x8

u32 N(IntroMessageState) = 0; // mode
s32 N(IntroMessageAlpha) = 0; // alpha related
IntroMessage* N(CurMessageList) = nullptr;

void N(UpdateIntroMessages)(IntroMessage** introMessageLists) {
    u8 type;
    f32 zoom1;
    f32 zoom2;
    s32 messageID;
    s32 opacity;
    s32 yOffset;
    static s32 N(IntroMessageDelay);

#ifdef PORT
    port_heap_check_all();
#endif

    if (N(CurMessageList) == nullptr) {
        N(CurMessageList) = introMessageLists[IntroMessageIdx];
    }

    switch (N(IntroMessageState)) {
        case INTRO_MSG_STATE_APPEAR:
            if (N(CurMessageList)->messageID == INTRO_MSG_BLANK) {
                N(IntroMessageState) = INTRO_MSG_STATE_BLANK;
                N(IntroMessageDelay) = N(CurMessageList)->duration;
            } else {
                N(IntroMessageAlpha) += 10;
                if (N(IntroMessageAlpha) > 255) {
                    N(IntroMessageAlpha) = 255;
                    N(IntroMessageState) = INTRO_MSG_STATE_SHOWING;
                    N(IntroMessageDelay) = N(CurMessageList)->duration;
                }
            }
            break;
        case INTRO_MSG_STATE_SHOWING:
            if (N(IntroMessageDelay) == 0) {
                N(IntroMessageState) = INTRO_MSG_STATE_VANISH;
            } else {
                N(IntroMessageDelay)--;
            }
            break;
        case INTRO_MSG_STATE_VANISH:
            N(IntroMessageAlpha) -= 10;
            if (N(IntroMessageAlpha) < 0) {
                N(IntroMessageAlpha) = 0;
                N(CurMessageList)++;
                if (N(CurMessageList)->messageID == MSG_NONE) {
                    N(IntroMessageState) = INTRO_MSG_STATE_DONE;
                } else {
                    N(IntroMessageState) = INTRO_MSG_STATE_APPEAR;
                }
            }
            break;
        case INTRO_MSG_STATE_DONE:
            break;
        case INTRO_MSG_STATE_BLANK:
            if (N(IntroMessageDelay) != 0) {
                N(IntroMessageDelay)--;
                break;
            }
            N(CurMessageList)++;
            if (N(CurMessageList)->messageID == MSG_NONE) {
                N(IntroMessageState) = INTRO_MSG_STATE_DONE;
            } else {
                N(IntroMessageState) = INTRO_MSG_STATE_APPEAR;
            }
            break;
    }
    get_screen_overlay_params(SCREEN_LAYER_BACK, &type, &zoom1);
    get_screen_overlay_params(SCREEN_LAYER_FRONT, &type, &zoom2);
    opacity = ((N(IntroMessageAlpha) * (255.0f - zoom1) * (255.0f - zoom2)) / 255.0f) / 255.0f;
    if (opacity > 0) {
        messageID = N(CurMessageList)->messageID;
#ifdef PORT
        // Intro message IDs are 1..0x1A or INTRO_MSG_BLANK. Anything else means
        // CurMessageList is corrupt, and draw_msg would treat it as a pointer.
        if (messageID != 0 && messageID != INTRO_MSG_BLANK && (u32)messageID > 0xFFFF) {
            static s32 sBadMsgLogCount = 0;
            if (sBadMsgLogCount++ < 5) {
                fprintf(stderr, "[narrator] bad messageID=0x%X cur=%p state=%d alpha=%d idx=%d\n",
                        messageID, (void*)N(CurMessageList), (int)N(IntroMessageState),
                        (int)N(IntroMessageAlpha), (int)IntroMessageIdx);
            }
            return;
        }
#endif
        if (messageID != 0) {
#if VERSION_JP
            draw_msg(N(CurMessageList)->messageID, 0, 200, opacity, -1, 0);
#else
            yOffset = 0;
#ifdef PORT
            gPortWatchAddr = (void* volatile*)&N(CurMessageList);
            gPortWatchVal = (void*)N(CurMessageList);
#endif
            if (get_msg_lines(messageID) >= 2) {
                yOffset = -7;
            }
#ifdef PORT
            port_watch_check("narrator: get_msg_lines returned");
            gPortWatchAddr = NULL;
#endif
#ifdef PORT
            // The ID gets read again after get_msg_lines(), and on Vita that second read came
            // back as garbage. Log it, and draw with the ID that passed the check above.
            if (N(CurMessageList)->messageID != messageID) {
                static s32 sChangedLogCount = 0;
                if (sChangedLogCount++ < 5) {
                    fprintf(stderr, "[narrator] messageID changed during get_msg_lines: was 0x%X now 0x%X at %p\n",
                            messageID, N(CurMessageList)->messageID, (void*)N(CurMessageList));
                }
            }
            draw_msg(messageID, 0, yOffset + 196, opacity, -1, 0);
#else
            draw_msg(N(CurMessageList)->messageID, 0, yOffset + 196, opacity, -1, 0);
#endif
#endif
        }
    }
}

API_CALLABLE(N(SetCurtainCallback)) {
    Bytecode* args = script->ptrReadPos;

    set_curtain_draw_callback((void (*)) evt_get_variable(script, *args++));
    return ApiStatus_DONE2;
}
