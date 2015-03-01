#pragma once

#include <toxprpl.h>

typedef struct _ToxPRPL_GroupChat {

    /*
     * Tox group number
     */
    int groupNumber;

    /*
     * Either TOX_GROUPCHAT_TYPE_AV, or TOX_GROUPCHAT_TYPE_TEXT
     */
    uint8_t groupType;

} ToxPRPL_GroupChat;

/*
 * Purple API
 */

extern const char* TOXPRPL_CONV_DATA_KEY;
extern const char* TOXPRPL_BUDDY_PEERID;

GList* ToxPRPL_Purple_findByPeerId(PurpleConvChat*, int);


/*
 * Tox backend
 */

/*
 * Handle incoming group invites
 */
void ToxPRPL_Tox_onGroupInvite(Tox*, int32_t, uint8_t, const uint8_t*, uint16_t, void*);

/*
 * Handle incoming group messages
 */
void ToxPRPL_Tox_onGroupMessage(Tox*, int, int, const uint8_t*, uint16_t, void*);

/*
 * Handle incoming group actions
 */
void ToxPRPL_Tox_onGroupAction(Tox*, int, int, const uint8_t*, uint16_t, void*);

/*
 * Handle chat title change notifications
 */
void ToxPRPL_Tox_onGroupChangeTitle(Tox*, int, int, const uint8_t*, uint8_t, void*);

/*
 * Handle buddylist change notifications
 */
void ToxPRPL_Tox_onGroupNamelistChange(Tox*, int, int, TOX_CHAT_CHANGE, void*);