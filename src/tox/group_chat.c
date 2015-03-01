/*
 * Tox group chat callbacks
 */

#include <toxprpl.h>
#include <toxprpl/group_chat.h>
#include <string.h>


// Group Invitation Handler -------------------------------------------------------------------------------

static const PurpleConnectionFlags TOX_CONNECTION_FLAGS =
        PURPLE_CONNECTION_NO_FONTSIZE | PURPLE_CONNECTION_NO_BGCOLOR | PURPLE_CONNECTION_NO_IMAGES;

const char* TOXPRPL_CONV_DATA_KEY = "tox_data";

/*
 * Transactional data type used in invite processing callbacks
 */
typedef struct _toxprpl_group_invite_data {

    uint8_t groupType;
    int groupNumber;

    const uint8_t* inviteData;
    uint16_t dataLength;

    int32_t friend;

    Tox* toxConnection;
    PurpleConnection* connection;

} ToxPRPL_GroupInviteData;

void ToxPRPL_Tox_onGroupInviteAccepted(ToxPRPL_GroupInviteData* data) {

    data->groupNumber = tox_join_groupchat(data->toxConnection, data->friend, data->inviteData, data->dataLength);

    if (data->groupNumber < 0) {
        purple_debug_error(TOXPRPL_ID, "Unable to join group chat (tox API returned failure value)\n");
        g_free(data);
        return;
    }

    ToxPRPL_GroupChat* chatData = g_new0(ToxPRPL_GroupChat, 1);
    chatData->groupNumber = data->groupNumber;
    chatData->groupType = data->groupType;

    char chatName[TOX_MAX_NAME_LENGTH + 1];
    tox_group_get_title(data->toxConnection, data->groupNumber, (uint8_t*) chatName, TOX_MAX_NAME_LENGTH);

    PurpleConversation* conversation =
            purple_conversation_new(PURPLE_CONV_TYPE_CHAT, data->connection->account, (const char*) chatName);

    purple_conversation_set_data(conversation, TOXPRPL_CONV_DATA_KEY, chatData);
    purple_conversation_set_features(conversation, TOX_CONNECTION_FLAGS);
    purple_conv_chat_set_id(purple_conversation_get_chat_data(conversation), chatData->groupNumber);

    purple_conversation_present(conversation);

    g_free(data);
}


void ToxPRPL_Tox_onGroupInviteRejected(ToxPRPL_GroupInviteData* data) {

    g_free(data);
}

void ToxPRPL_Tox_onGroupInvite(Tox* tox, int32_t friendNumber, uint8_t groupType, const uint8_t* data,
                               uint16_t length, void* userData) {

    PurpleConnection* purpleConnection = (PurpleConnection*) userData;

    uint8_t buddyName[TOX_MAX_NAME_LENGTH + 1];
    tox_get_name(tox, friendNumber, buddyName);

    purple_debug(PURPLE_DEBUG_INFO, TOXPRPL_ID, "%s invited us to a group chat", buddyName);

    if (groupType == TOX_GROUPCHAT_TYPE_AV) {
        purple_debug(PURPLE_DEBUG_WARNING, TOXPRPL_ID,
                     "user was invited to a group chat of the type AV. the invite was ignored because AV support is not implemented.\n");
        return;
    }

    ToxPRPL_GroupInviteData* inviteData = g_new0(ToxPRPL_GroupInviteData, 1);

    inviteData->groupType = groupType;
    inviteData->inviteData = data;
    inviteData->dataLength = length;
    inviteData->friend = friendNumber;
    inviteData->toxConnection = tox;
    inviteData->connection = purpleConnection;

    char* messageFormatted = g_strdup_printf(_("%s has invited you to a group chat"), buddyName);

    purple_request_accept_cancel(purpleConnection,
                                 _("You've been invited to a group chat"),
                                 messageFormatted,
                                 _("If you wish to join, choose accept."),
                                 PURPLE_DEFAULT_ACTION_NONE,
                                 purpleConnection->account,
                                 (const char*) buddyName,
                                 NULL,
                                 &inviteData,
                                 G_CALLBACK(ToxPRPL_Tox_onGroupInviteAccepted),
                                 G_CALLBACK(ToxPRPL_Tox_onGroupInviteRejected));

    g_free(messageFormatted);
}

// Group Message Handler ----------------------------------------------------------------------------------


void ToxPRPL_Tox_onGroupMessage(Tox* tox, int groupNumber, int peerNumber, const uint8_t* message,
                                uint16_t length, void* userData) {

    PurpleConnection* purpleConnection = (PurpleConnection*) userData;

    char chatName[TOX_MAX_NAME_LENGTH + 1];
    tox_group_get_title(tox, groupNumber, (uint8_t*) chatName, TOX_MAX_NAME_LENGTH);

    char peerName[TOX_MAX_NAME_LENGTH + 1];
    tox_group_peername(tox, groupNumber, peerNumber, (uint8_t*) peerName);

    PurpleConversation* purpleConvo = purple_find_chat(purpleConnection, groupNumber);

    if (!purpleConvo) {
        purple_debug_warning(TOXPRPL_ID, "Received a message for chat %s, but no such chat exists\n", chatName);
        return;
    }

    PurpleConvChat* purpleChat = purple_conversation_get_chat_data(purpleConvo);

    purple_conv_chat_write(purpleChat, peerName, (const char*) message,
                           PURPLE_MESSAGE_RECV, time(NULL));

}

// Group Action Handler -------------------------------------------------------------------------------

/*
 * An ``action'' in Tox seems to refer to ``/me ...''
 * As such, this can pretty much just turn the action in to ``/me ...'' and forward it
 * to the incoming message handler.
 */
void ToxPRPL_Tox_onGroupAction(Tox* tox, int groupNumber, int peerNumber, const uint8_t* action,
                               uint16_t length, void* userData) {

    char* message = g_strdup_printf("/me %s", action);

    ToxPRPL_Tox_onGroupMessage(tox, groupNumber, peerNumber, (const uint8_t*) message,
                               (uint16_t) strlen(message), userData);

    g_free(message);

}

// Group title change handler --------------------------------------------------------------------------

static const char* USER_CHANGE_TITLE_ACTION = _("changed the group title");

void ToxPRPL_Tox_onGroupChangeTitle(Tox* tox, int groupNumber, int peerNumber, const uint8_t* newTitle,
                                    uint8_t titleLenght, void* userData) {

    PurpleConnection* purpleConnection = (PurpleConnection*) userData;

    PurpleConversation* purpleConvo = purple_find_chat(purpleConnection, groupNumber);

    if(!purpleConvo) {
        purple_debug_warning(TOXPRPL_ID, "Received title change notification for a nonexistant group\n");
        return;
    }

    ToxPRPL_Tox_onGroupAction(tox, groupNumber, peerNumber, (const uint8_t*) USER_CHANGE_TITLE_ACTION,
                              (uint16_t) strlen(USER_CHANGE_TITLE_ACTION), userData);

    purple_conversation_set_title(purpleConvo, (const char*) newTitle);

}

// Group namelist change handler ----------------------------------------------------------------------

static void groupBuddyAdd(Tox* tox, PurpleConversation* convo, int groupNumber, int peerNumber) {

    char buddyName[TOX_MAX_NAME_LENGTH];
    tox_group_peername(tox, groupNumber, peerNumber, (uint8_t*) buddyName);

    PurpleConvChat* chat = purple_conversation_get_chat_data(convo);

    purple_conv_chat_add_user(chat, buddyName, NULL, PURPLE_CBFLAGS_NONE, TRUE);

    PurpleConvChatBuddy* buddy = purple_conv_chat_cb_find(chat, buddyName);

    char* peerNumString = g_strdup_printf("%i", peerNumber);

    purple_conv_chat_cb_set_attribute(chat, buddy, TOXPRPL_BUDDY_PEERID, peerNumString);

    g_free(peerNumString);
}

static void groupBuddyDel(Tox* tox, PurpleConversation* convo, int groupNumber, int peerNumber) {

    char buddyName[TOX_MAX_NAME_LENGTH + 1];
    tox_group_peername(tox, groupNumber, peerNumber, (uint8_t*) buddyName);

    PurpleConvChat* chat = purple_conversation_get_chat_data(convo);

    purple_conv_chat_remove_user(chat, (const char*) buddyName, NULL);
}

static void groupBuddyRename(Tox* tox, PurpleConversation* convo, int groupNumber, int peerNumber) {

    PurpleConvChat* chat = purple_conversation_get_chat_data(convo);

    GList* usersWithPeerId = ToxPRPL_Purple_findByPeerId(chat, peerNumber);

    // Iterate through users that match (should be one, but why not comply?)
    GList* currentLink = usersWithPeerId;
    do{
        PurpleConvChatBuddy* buddy = (PurpleConvChatBuddy*) currentLink->data;

        char newName[TOX_MAX_NAME_LENGTH];
        tox_group_peername(tox, groupNumber, peerNumber, (uint8_t*) newName);

        // The way purple_conv_chat_cb_new() works, name and alias are the same.
        purple_conv_chat_rename_user(chat, buddy->name, newName);

        currentLink = currentLink->next;
    } while(currentLink);

    g_list_free(currentLink);
}

void ToxPRPL_Tox_onGroupNamelistChange(Tox* tox, int groupNumber, int peerNumber, TOX_CHAT_CHANGE change,
                                       void* userData) {

    PurpleConnection* purpleConnection = (PurpleConnection*) userData;

    PurpleConversation* purpleConvo = purple_find_chat(purpleConnection, groupNumber);

    if(!purpleConvo) {
        purple_debug_warning(TOXPRPL_ID, "Received a change notification for a nonexistant group %i\n", groupNumber);
        return;
    }

    switch(change) {
        case TOX_CHAT_CHANGE_PEER_ADD:
            groupBuddyAdd(tox, purpleConvo, groupNumber, peerNumber);
            break;
        case TOX_CHAT_CHANGE_PEER_DEL:
            groupBuddyDel(tox, purpleConvo, groupNumber, peerNumber);
            break;
        case TOX_CHAT_CHANGE_PEER_NAME:
            groupBuddyRename(tox, purpleConvo, groupNumber, peerNumber);
            break;
    }

}
