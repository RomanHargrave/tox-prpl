/*
 * This file contains chat functions that pertain to either the general act of chatting,
 * or specifically to user-to-user chat.
 * Group chat functionality resides in ``purple/group_chat.c''.
 */

#include <toxprpl.h>
#include <string.h>

/**
* This PRPL function should return a positive value on success.
* If the message is too big to be sent, return -E2BIG.  If
* the account is not connected, return -ENOTCONN.  If the
* PRPL is unable to send the message for another reason, return
* some other negative value.  You can use one of the valid
* errno values, or just big something.  If the message should
* not be echoed to the conversation window, return 0.
*/
int ToxPRPL_Purple_sendUserMessage(PurpleConnection* gc, const char* who, const char* message,
                                   PurpleMessageFlags flags) {
    const char* from_username = gc->account->username;

    purple_debug_info("toxprpl", "sending message from %s to %s\n",
                      from_username, who);

    int message_sent = -999;

    PurpleAccount* account = purple_connection_get_account(gc);
    PurpleBuddy* buddy = purple_find_buddy(account, who);
    if (buddy == NULL) {
        purple_debug_info("toxprpl", "Can't send message because buddy %s was not found\n", who);
        return message_sent;
    }
    ToxPRPL_BuddyData* buddy_data = purple_buddy_get_protocol_data(buddy);
    if (buddy_data == NULL) {
        purple_debug_info("toxprpl", "Can't send message because tox friend number is unknown\n");
        return message_sent;
    }
    ToxPRPL_PluginData* plugin = purple_connection_get_protocol_data(gc);
    char* no_html = purple_markup_strip_html(message);

    if (purple_message_meify(no_html, -1)) {
        if (tox_send_action(plugin->tox, buddy_data->tox_friendlist_number,
                            (uint8_t*) no_html, strlen(no_html)) != 0) {
            message_sent = 1;
        }
    }
    else {
        if (tox_send_message(plugin->tox, buddy_data->tox_friendlist_number,
                             (uint8_t*) no_html, strlen(no_html)) != 0) {
            message_sent = 1;
        }
    }
    if (no_html) {
        free(no_html);
    }
    return message_sent;
}


/*
 * LibPurple typing callback
 */
unsigned int ToxPRPL_Purple_updateTypingState(PurpleConnection* gc, const char* who, PurpleTypingState state) {
    purple_debug_info("toxprpl", "send_typing\n");

    toxprpl_return_val_if_fail(gc != NULL, 0);
    toxprpl_return_val_if_fail(who != NULL, 0);

    ToxPRPL_PluginData* plugin = purple_connection_get_protocol_data(gc);
    toxprpl_return_val_if_fail(plugin != NULL && plugin->tox != NULL, 0);

    PurpleAccount* account = purple_connection_get_account(gc);
    toxprpl_return_val_if_fail(account != NULL, 0);

    PurpleBuddy* buddy = purple_find_buddy(account, who);
    toxprpl_return_val_if_fail(buddy != NULL, 0);

    ToxPRPL_BuddyData* buddy_data = purple_buddy_get_protocol_data(buddy);
    toxprpl_return_val_if_fail(buddy_data != NULL, 0);

    switch (state) {
        case PURPLE_TYPING:
            purple_debug_info("toxprpl", "Send typing state: TYPING\n");
            tox_set_user_is_typing(plugin->tox, buddy_data->tox_friendlist_number, TRUE);
            break;

        case PURPLE_TYPED:
            purple_debug_info("toxprpl", "Send typing state: TYPED\n"); /* typing pause */
            tox_set_user_is_typing(plugin->tox, buddy_data->tox_friendlist_number, FALSE);
            break;

        default:
            purple_debug_info("toxprpl", "Send typing state: NOT_TYPING\n");
            tox_set_user_is_typing(plugin->tox, buddy_data->tox_friendlist_number, FALSE);
            break;
    }

    return 0;
}

