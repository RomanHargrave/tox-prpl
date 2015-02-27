/*
 * Tox callbacks related to the friend system
 */

#include <toxprpl.h>
#include <toxprpl/buddy.h>

void on_request(struct Tox* tox, const uint8_t* public_key,
                const uint8_t* data, uint16_t length, void* user_data) {
    purple_debug_info("toxprpl", "incoming friend request!\n");
    gchar* dialog_message;
    PurpleConnection* gc = (PurpleConnection*) user_data;

    gchar* buddy_key = toxprpl_tox_bin_id_to_string(public_key);
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
                          G_CALLBACK(toxprpl_add_to_buddylist),
                          G_CALLBACK(toxprpl_do_not_add_to_buddylist));
    g_free(dialog_message);
    g_free(request_msg);
}

void on_friend_action(Tox* tox, int32_t friendnum, const uint8_t* string,
                      uint16_t length, void* user_data) {
    purple_debug_info("toxprpl", "action received\n");
    PurpleConnection* gc = (PurpleConnection*) user_data;

    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnum);
        return;
    }

    gchar* buddy_key = toxprpl_tox_bin_id_to_string(client_id);
    gchar* safemsg = g_strndup((const char*) string, length);
    gchar* message = g_strdup_printf("/me %s", safemsg);
    g_free(safemsg);

    serv_got_im(gc, buddy_key, message, PURPLE_MESSAGE_RECV,
                time(NULL));
    g_free(buddy_key);
    g_free(message);
}

void on_nick_change(Tox* tox, int32_t friendnum, const uint8_t* data,
                    uint16_t length, void* user_data) {
    purple_debug_info("toxprpl", "Nick change!\n");

    PurpleConnection* gc = (PurpleConnection*) user_data;

    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnum);
        return;
    }

    gchar* buddy_key = toxprpl_tox_bin_id_to_string(client_id);
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

void on_status_change(struct Tox* tox, int32_t friendnum,
                      uint8_t userstatus, void* user_data) {

    purple_debug_info("toxprpl", "Status change: %d\n", userstatus);
    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnum, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnum);
        return;
    }

    gchar* buddy_key = toxprpl_tox_bin_id_to_string(client_id);

    PurpleConnection* gc = (PurpleConnection*) user_data;
    PurpleAccount* account = purple_connection_get_account(gc);
    purple_debug_info("toxprpl", "Setting user status for user %s to %s\n",
                      buddy_key, toxprpl_statuses[
                    toxprpl_get_status_index(tox, friendnum, userstatus)].id);
    purple_prpl_got_user_status(account, buddy_key,
                                toxprpl_statuses[
                                        toxprpl_get_status_index(tox, friendnum, userstatus)].id,
                                NULL);
    g_free(buddy_key);
}

