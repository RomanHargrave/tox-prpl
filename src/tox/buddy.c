/*
 * Tox callbacks related to the friend system
 */

#include <toxprpl.h>
#include <toxprpl/buddy.h>
#include <string.h>

void ToxPRPL_Tox_onUserConnectionStatusChange(Tox* tox, int32_t fnum, uint8_t status, void* user_data) {
    PurpleConnection* gc = (PurpleConnection*) user_data;
    int tox_status = TOXPRPL_STATUS_OFFLINE;
    if (status == 1) {
        tox_status = TOXPRPL_STATUS_ONLINE;
    }

    purple_debug_info("toxprpl", "Friend status change: %d\n", status);
    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, fnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend #%d\n",
                          fnum);
        return;
    }

    gchar* buddy_key = ToxPRPL_toxClientIdToString(client_id);
    PurpleAccount* account = purple_connection_get_account(gc);
    purple_prpl_got_user_status(account, buddy_key,
                                ToxPRPL_ToxStatuses[tox_status].id, NULL);
    g_free(buddy_key);
}

/*
 * Tox/Purple glue dialog action that handles acceptance of a friend request
 */
void ToxPRPL_Action_acceptFriendRequest(toxprpl_accept_friend_data* data) {
    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(data->gc);

    int ret = ToxPRPL_Purple_addFriend(plugin->tox, data->gc, data->buddy_key,
                                       FALSE, NULL);
    if (ret < 0) {
        g_free(data->buddy_key);
        g_free(data);
        // error dialogs handled in ToxPRPL_Purple_addFriend()
        return;
    }

    PurpleAccount* account = purple_connection_get_account(data->gc);

    uint8_t alias[TOX_MAX_NAME_LENGTH + 1];

    PurpleBuddy* buddy;
    int rc = tox_get_name(plugin->tox, ret, alias);
    alias[TOX_MAX_NAME_LENGTH] = '\0';
    if ((rc == 0) && (strlen((const char*) alias) > 0)) {
        purple_debug_info("toxprpl", "Got friend alias %s\n", alias);
        buddy = purple_buddy_new(account, data->buddy_key, (const char*) alias);
    }
    else {
        purple_debug_info("toxprpl", "Adding [%s]\n", data->buddy_key);
        buddy = purple_buddy_new(account, data->buddy_key, NULL);
    }

    toxprpl_buddy_data* buddy_data = g_new0(toxprpl_buddy_data, 1);
    buddy_data->tox_friendlist_number = ret;
    purple_buddy_set_protocol_data(buddy, buddy_data);
    purple_blist_add_buddy(buddy, NULL, NULL, NULL);
    TOX_USERSTATUS userstatus = (TOX_USERSTATUS) tox_get_user_status(plugin->tox, ret);
    purple_debug_info("toxprpl", "Friend %s has status %d\n",
                      data->buddy_key, userstatus);
    purple_prpl_got_user_status(account, data->buddy_key,
                                ToxPRPL_ToxStatuses[ToxPRPL_getStatusTypeIndex(plugin->tox, ret, userstatus)].id,
                                NULL);

    g_free(data->buddy_key);
    g_free(data);
}

/*
 * Tox/Purple glue dialog action that handles rejection (ignore) of a friend request
 */
void ToxPRPL_Action_rejectFriendRequest(toxprpl_accept_friend_data* data) {
    g_free(data->buddy_key);
    g_free(data);
}

void ToxPRPL_Tox_onFriendRequest(struct Tox* tox, uint8_t* public_key, uint8_t* data, uint16_t length,
                                 void* user_data) {
    purple_debug_info("toxprpl", "incoming friend request!\n");
    gchar* dialog_message;
    PurpleConnection* gc = (PurpleConnection*) user_data;

    gchar* buddy_key = ToxPRPL_toxClientIdToString(public_key);
    purple_debug_info("toxprpl", "Buddy request from %s: %s\n",
                      buddy_key, data);

    PurpleAccount* account = purple_connection_get_account(gc);
    PurpleBuddy* buddy = purple_find_buddy(account, buddy_key);
    if (buddy != NULL) {
        purple_debug_info("toxprpl", "Buddy %s already in buddy list!\n",
                          buddy_key);
        g_free(buddy_key);
        return;
    }

    dialog_message = g_strdup_printf("The user %s has sent you a friend "
                                             "request, do you want to add them?",
                                     buddy_key);

    gchar* request_msg = NULL;
    if (length > 0) {
        request_msg = g_strndup((const gchar*) data, length);
    }

    toxprpl_accept_friend_data* fdata = g_new0(toxprpl_accept_friend_data, 1);
    fdata->gc = gc;
    fdata->buddy_key = buddy_key;
    purple_request_yes_no(gc, "New friend request", dialog_message,
                          request_msg,
                          PURPLE_DEFAULT_ACTION_NONE,
                          account, NULL,
                          NULL,
                          fdata, // buddy key will be freed elsewhere
                          G_CALLBACK(ToxPRPL_Action_acceptFriendRequest),
                          G_CALLBACK(ToxPRPL_Action_rejectFriendRequest));
    g_free(dialog_message);
    g_free(request_msg);
}

/*
 * Tox callback invoked when a friend performs an action, such as an instant message
 */
void ToxPRPL_Tox_onFriendAction(Tox* tox, int32_t friendnum, uint8_t *string, uint16_t length, void* user_data) {
    purple_debug_info("toxprpl", "action received\n");
    PurpleConnection* gc = (PurpleConnection*) user_data;

    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnum);
        return;
    }

    gchar* buddy_key = ToxPRPL_toxClientIdToString(client_id);
    gchar* safemsg = g_strndup((const char*) string, length);
    gchar* message = g_strdup_printf("/me %s", safemsg);
    g_free(safemsg);

    serv_got_im(gc, buddy_key, message, PURPLE_MESSAGE_RECV,
                time(NULL));
    g_free(buddy_key);
    g_free(message);
}

void ToxPRPL_Tox_onFriendChangeNickname(Tox* tox, int32_t friendnum, uint8_t *data, uint16_t length,
                                        void* user_data) {
    purple_debug_info("toxprpl", "Nick change!\n");

    PurpleConnection* gc = (PurpleConnection*) user_data;

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
        purple_debug_info("toxprpl", "Ignoring nick change because buddy %s was not found\n", buddy_key);
        g_free(buddy_key);
        return;
    }

    g_free(buddy_key);
    gchar* safedata = g_strndup((const char*) data, length);
    purple_blist_alias_buddy(buddy, safedata);
    g_free(safedata);
}

void ToxPRPL_Tox_onFriendChangeStatus(struct Tox* tox, int32_t friendnum, uint8_t userstatus, void* user_data) {

    purple_debug_info("toxprpl", "Status change: %d\n", userstatus);
    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnum);
        return;
    }

    gchar* buddy_key = ToxPRPL_toxClientIdToString(client_id);

    PurpleConnection* gc = (PurpleConnection*) user_data;
    PurpleAccount* account = purple_connection_get_account(gc);
    purple_debug_info("toxprpl", "Setting user status for user %s to %s\n",
                      buddy_key, ToxPRPL_ToxStatuses[
                    ToxPRPL_getStatusTypeIndex(tox, friendnum, userstatus)].id);
    purple_prpl_got_user_status(account, buddy_key,
                                ToxPRPL_ToxStatuses[
                                        ToxPRPL_getStatusTypeIndex(tox, friendnum, userstatus)].id,
                                NULL);
    g_free(buddy_key);
}
