/*
 * Tox functions for general chat, or specifically one-to-one chat.
 * Group chat should go in ``groupchat.c''
 */

#include <toxprpl.h>

void ToxPRPL_Tox_onMessageReceived(Tox* tox, int32_t friendnum, uint8_t const *string, uint16_t length,
                                   void* user_data) {
    purple_debug_info("toxprpl", "Message received!\n");
    PurpleConnection* gc = (PurpleConnection*) user_data;

    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnum);
        return;
    }

    gchar* buddy_key = ToxPRPL_toxClientIdToString(client_id);
    gchar* safemsg = g_strndup((const char*) string, length);
    serv_got_im(gc, buddy_key, safemsg, PURPLE_MESSAGE_RECV,
                time(NULL));
    g_free(buddy_key);
    g_free(safemsg);
}

void ToxPRPL_Tox_onUserTypingChange(Tox* tox, int32_t friendnum, uint8_t is_typing, void* userdata) {
    purple_debug_info("toxprpl", "Friend typing status change: %d", friendnum);

    PurpleConnection* gc = userdata;
    toxprpl_return_if_fail(gc != NULL);

    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnum);
        return;
    }

    gchar* buddy_key = ToxPRPL_toxClientIdToString(client_id);
    PurpleAccount* account = purple_connection_get_account(gc);
    PurpleBuddy* buddy = purple_find_buddy(account, buddy_key);
    if (buddy == NULL) {
        purple_debug_info("toxprpl", "Ignoring typing change because buddy %s was not found\n", buddy_key);
        g_free(buddy_key);
        return;
    }

    g_free(buddy_key);

    if (is_typing) {
        serv_got_typing(gc, buddy->name, 5, PURPLE_TYPING);
        /*   ^ timeout for typing status (0 = disabled) */
    }
    else {
        serv_got_typing_stopped(gc, buddy->name);
    }
}

