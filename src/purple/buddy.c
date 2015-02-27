#include <toxprpl.h>
#include <toxprpl/account.h>
#include <string.h>
#include <toxprpl_data.h>

/*
 * LibPurple friend add callback.
 */
int ToxPRPL_Purple_addFriend(Tox* tox, PurpleConnection* gc, const char* buddy_key, gboolean sendrequest,
                             const char* message) {
    unsigned char* bin_key = toxprpl_hex_string_to_data(buddy_key);
    int ret;

    if (sendrequest == TRUE) {
        if ((message == NULL) || (strlen(message) == 0)) {
            message = DEFAULT_REQUEST_MESSAGE;
        }
        ret = tox_add_friend(tox, bin_key, (uint8_t*) message,
                             (uint16_t) strlen(message) + 1);
    }
    else {
        ret = tox_add_friend_norequest(tox, bin_key);
    }

    g_free(bin_key);
    const char* msg;
    switch (ret) {
        case TOX_FAERR_TOOLONG:
            msg = "Message too long";
            break;
        case TOX_FAERR_NOMESSAGE:
            msg = "Missing request message";
            break;
        case TOX_FAERR_OWNKEY:
            msg = "You're trying to add yourself as a friend";
            break;
        case TOX_FAERR_ALREADYSENT:
            msg = "Friend request already sent";
            break;
        case TOX_FAERR_BADCHECKSUM:
            msg = "Can't add friend: bad checksum in ID";
            break;
        case TOX_FAERR_SETNEWNOSPAM:
            msg = "Can't add friend: wrong nospam ID";
            break;
        case TOX_FAERR_NOMEM:
            msg = "Could not allocate memory for friendlist";
            break;
        case TOX_FAERR_UNKNOWN:
            msg = "Error adding friend";
            break;
        default:
            msg = "No Error";
            break;
    }

    if (ret < 0) {
        purple_notify_error(gc, _("Error"), msg, NULL);
    } else {
        purple_debug_info("toxprpl", "Friend %s added as %d\n", buddy_key, ret);

        // save account so buddy is not lost in case pidgin does not exit
        // cleanly
        PurpleAccount* account = purple_connection_get_account(gc);
        ToxPRPL_saveAccount(account, tox);
    }

    return ret;
}

/*
 * Retrieve the current status of the given buddy
 * This is called by ``ToxPRPL_Purple_addBuddy'' and ``tox_connection_check''
 */
void ToxPRPL_Purple_getBuddyInfo(gpointer data, gpointer user_data) {
    purple_debug_info("toxprpl", "ToxPRPL_Purple_getBuddyInfo\n");
    PurpleBuddy* buddy = (PurpleBuddy*) data;
    PurpleConnection* gc = (PurpleConnection*) user_data;
    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);

    toxprpl_buddy_data* buddy_data = purple_buddy_get_protocol_data(buddy);
    if (buddy_data == NULL) {
        unsigned char* bin_key = toxprpl_hex_string_to_data(buddy->name);
        int fnum = tox_get_friend_number(plugin->tox, bin_key);
        buddy_data = g_new0(toxprpl_buddy_data, 1);
        buddy_data->tox_friendlist_number = fnum;
        purple_buddy_set_protocol_data(buddy, buddy_data);
        g_free(bin_key);
    }

    int statusIndex = toxprpl_get_status_index(plugin->tox, buddy_data->tox_friendlist_number,
                                               tox_get_user_status(plugin->tox, buddy_data->tox_friendlist_number));

    PurpleAccount* account = purple_connection_get_account(gc);
    purple_debug_info("toxprpl", "Setting user status for user %s to %s\n",
                      buddy->name, toxprpl_statuses[statusIndex].id);
    purple_prpl_got_user_status(account, buddy->name, toxprpl_statuses[statusIndex].id, NULL);

    uint8_t alias[TOX_MAX_NAME_LENGTH + 1];
    if (tox_get_name(plugin->tox, buddy_data->tox_friendlist_number, alias) == 0) {
        alias[TOX_MAX_NAME_LENGTH] = '\0';
        purple_blist_alias_buddy(buddy, (const char*) alias);
    }
}

void ToxPRPL_Purple_removeBuddy(PurpleConnection* gc, PurpleBuddy* buddy, PurpleGroup* group) {
    purple_debug_info("toxprpl", "removing buddy %s\n", buddy->name);
    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);
    toxprpl_buddy_data* buddy_data = purple_buddy_get_protocol_data(buddy);
    if (buddy_data != NULL) {
        purple_debug_info("toxprpl", "removing tox friend #%d\n",
                          buddy_data->tox_friendlist_number);
        tox_del_friend(plugin->tox, buddy_data->tox_friendlist_number);

        // save account to make sure buddy stays deleted in case pidgin does
        // not exit cleanly
        PurpleAccount* account = purple_connection_get_account(gc);
        ToxPRPL_saveAccount(account, plugin->tox);
    }
}

void ToxPRPL_Purple_addBuddy(PurpleConnection* gc, PurpleBuddy* buddy, PurpleGroup* group, const char* msg) {
    purple_debug_info("toxprpl", "adding %s to buddy list\n", buddy->name);

    buddy->name = g_strstrip(buddy->name);
    if (strlen(buddy->name) != (TOX_FRIEND_ADDRESS_SIZE * 2)) {
        purple_notify_error(gc, _("Error"),
                            _("Invalid Tox ID given (must be 76 characters long)"), NULL);
        purple_blist_remove_buddy(buddy);
        return;
    }

    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);
    int ret = ToxPRPL_Purple_addFriend(plugin->tox, gc, buddy->name, TRUE, msg);
    if (ret < 0) {
        purple_debug_info("toxprpl", "adding buddy %s failed (%d)\n",
                          buddy->name, ret);
        purple_blist_remove_buddy(buddy);
        return;
    }

    // save account so buddy is not lost in case pidgin does not exit cleanly
    PurpleAccount* account = purple_connection_get_account(gc);
    ToxPRPL_saveAccount(account, plugin->tox);

    gchar* cut = g_ascii_strdown(buddy->name, TOX_CLIENT_ID_SIZE * 2 + 1);
    cut[TOX_CLIENT_ID_SIZE * 2] = '\0';
    purple_debug_info("toxprpl", "converted %s to %s\n", buddy->name, cut);
    purple_blist_rename_buddy(buddy, cut);
    g_free(cut);
    // buddy data will be added by the query_buddy_info function
    ToxPRPL_Purple_getBuddyInfo((gpointer) buddy, (gpointer) gc);
}

/*
 * TODO Buddy icons
 * Implementation for buddy icons should just return the name of  the buddy in terms of tox
 */
const char* ToxPRPL_Purple_getListIconForUser(PurpleAccount* acct, PurpleBuddy* buddy) {
    return "tox";
}
