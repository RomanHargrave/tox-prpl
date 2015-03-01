/*
 *  Copyright (c) 2013 Sergey 'Jin' Bostandzhyan <jin at mediatomb dot cc>
 *
 *  tox-prlp - libpurple protocol plugin or Tox (see http://tox.im)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  This plugin is based on the Nullprpl mockup from Pidgin / Finch / libpurple
 *  which is disributed under GPL v2 or later.  See http://pidgin.im/
 */

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include <string.h>
#include <sys/types.h>

#ifdef __WIN32__
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else

#include <netdb.h>

#endif

#include <glib.h>

#define PURPLE_PLUGINS

#include <toxprpl.h>
#include <toxprpl/account.h>
#include <toxprpl/buddy.h>
#include <toxprpl/xfers.h>
#include <toxprpl/group_chat.h>

void ToxPRPL_initializePRPL(PurpleAccount* acct);

// prpl chat functions ------------------------------------------------

/*
 * In file ``purple/chat.c''
 */

int ToxPRPL_Purple_sendUserMessage(PurpleConnection*, const char*, const char*, PurpleMessageFlags);

unsigned int ToxPRPL_Purple_updateTypingState(PurpleConnection*, const char*, PurpleTypingState);

// end prpl chat functions --------------------------------------------

// PRPL Commands -------------------------------------------------------------------------------------------------------

/*
 * In file ``purple/commands.c''
 */

/*
 * /myid command
 */
PurpleCmdRet ToxPRPL_Command_myId(PurpleConversation*, const gchar*, gchar**, gchar**, void*);

/*
 * /nick command
 */
PurpleCmdRet ToxPRPL_Command_nick(PurpleConversation*, const gchar*, gchar**, gchar**, void*);

// End PRPL Commands ---------------------------------------------------------------------------------------------------

// Tox Callbacks -------------------------------------------------------------------------------------------------------

/*
 * In file ``tox/buddy.c''
 */

void ToxPRPL_Tox_onUserConnectionStatusChange(Tox*, int32_t, uint8_t, void*);

void ToxPRPL_Tox_onFriendRequest(struct Tox*, uint8_t const *, uint8_t const *, uint16_t, void*);

void ToxPRPL_Tox_onFriendAction(Tox*, int32_t, uint8_t const *, uint16_t, void*);

void ToxPRPL_Tox_onFriendChangeNickname(Tox*, int32_t, uint8_t const *, uint16_t, void*);

void ToxPRPL_Tox_onFriendChangeStatus(struct Tox*, int32_t, uint8_t, void*);

/*
 * In file ``tox/chat.c''
 */

void ToxPRPL_Tox_onMessageReceived(Tox*, int32_t, uint8_t const *, uint16_t, void*);

void ToxPRPL_Tox_onUserTypingChange(Tox*, int32_t, uint8_t, void*);

// End of Tox Callbacks ------------------------------------------------------------------------------------------------

/*
 * Keep Tox active
 */
static gboolean ToxPRPL_updateConnectionState(gpointer data) {
    PurpleConnection* gc = (PurpleConnection*) data;
    ToxPRPL_PluginData* plugin = purple_connection_get_protocol_data(gc);
    if ((plugin != NULL) && (plugin->tox != NULL)) {
        tox_do(plugin->tox);
    }
    return TRUE;
}

/*
 * LibPurple recurring call.
 *
 * Every time that this is invoked, DHT connection status will be checked,
 * and data for each buddy will be freshened
 */
gboolean ToxPRPL_updateClientStatus(gpointer gc) {
    ToxPRPL_PluginData* plugin = purple_connection_get_protocol_data(gc);

    if ((plugin->connected == 0) && tox_isconnected(plugin->tox)) {
        plugin->connected = 1;
        purple_connection_update_progress(gc, _("Connected"),
                                          1,   /* which connection step this is */
                                          2);  /* total number of steps */
        purple_connection_set_state(gc, PURPLE_CONNECTED);
        purple_debug_info("toxprpl", "DHT connected!\n");

        // query status of all buddies
        PurpleAccount* account = purple_connection_get_account(gc);
        GSList* buddy_list = purple_find_buddies(account, NULL);
        g_slist_foreach(buddy_list, ToxPRPL_Purple_getBuddyInfo, gc);
        g_slist_free(buddy_list);

        uint8_t our_name[TOX_MAX_NAME_LENGTH + 1];
        uint16_t name_len = tox_get_self_name(plugin->tox, our_name);
        // bug in the library?
        if (name_len == 0) {
            our_name[0] = '\0';
        }
        our_name[TOX_MAX_NAME_LENGTH] = '\0';

        const char* nick = purple_account_get_string(account, "nickname", NULL);
        if (strlen(nick) == 0) {
            if (strlen((const char*) our_name) > 0) {
                purple_connection_set_display_name(gc, (const char*) our_name);
                purple_account_set_string(account, "nickname",
                                          (const char*) our_name);
            }
        }
        else {
            ToxPRPL_Purple_onSetNickname(gc, nick);
        }

        PurpleStatus* status = purple_account_get_active_status(account);
        if (status != NULL) {
            purple_debug_info("toxprpl", "(re)setting status\n");
            ToxPRPL_Purple_onSetStatus(account, status);
        }
    }
    else if ((plugin->connected == 1) && !tox_isconnected(plugin->tox)) {
        plugin->connected = 0;
        purple_debug_info("toxprpl", "DHT disconnected!\n");
        purple_connection_notice(gc,
                                 _("Connection to DHT server lost, attempging to reconnect..."));
        purple_connection_update_progress(gc, _("Reconnecting..."),
                                          0,   /* which connection step this is */
                                          2);  /* total number of steps */
    }
    return TRUE;
}


// PRPL API Backend ----------------------------------------------------------------------------------------------------

/*
 * Returns potential status types
 * Called only by libpurple
 */
static GList* ToxPRPL_getStatusTypes(PurpleAccount* acct) {
    GList* types = NULL;
    PurpleStatusType* type;
    int i;

    purple_debug_info("toxprpl", "setting up status types\n");

    for (i = 0; i < TOXPRPL_MAX_STATUS; i++) {
        type = purple_status_type_new_with_attrs(ToxPRPL_ToxStatuses[i].primitive,
                                                 ToxPRPL_ToxStatuses[i].id, ToxPRPL_ToxStatuses[i].title, TRUE, TRUE,
                                                 FALSE,
                                                 "message", _("Message"), purple_value_new(PURPLE_TYPE_STRING),
                                                 NULL);
        types = g_list_append(types, type);
    }

    return types;
}

// ---- ToxPRPL_synchronizeBuddyList -------------------------------------------------------------------------------------------

/*
 * Called by ToxPRPL_synchronizeBuddyList
 * Used to add users not yet present in the libpurple buddy list
 */
static void ToxPRPL_synchronizeBuddy(PurpleAccount* account, Tox* tox, int friend_number) {
    uint8_t alias[TOX_MAX_NAME_LENGTH + 1];
    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friend_number, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend #%d\n",
                          friend_number);
        return;
    }

    gchar* buddy_key = ToxPRPL_toxClientIdToString(client_id);


    PurpleBuddy* buddy;
    int ret = tox_get_name(tox, friend_number, alias);
    alias[TOX_MAX_NAME_LENGTH] = '\0';
    if ((ret == 0) && (strlen((const char*) alias) > 0)) {
        purple_debug_info("toxprpl", "Got friend alias %s\n", alias);
        buddy = purple_buddy_new(account, buddy_key, (const char*) alias);
    }
    else {
        purple_debug_info("toxprpl", "Adding [%s]\n", buddy_key);
        buddy = purple_buddy_new(account, buddy_key, NULL);
    }

    ToxPRPL_BuddyData* buddy_data = g_new0(ToxPRPL_BuddyData, 1);
    buddy_data->tox_friendlist_number = friend_number;
    purple_buddy_set_protocol_data(buddy, buddy_data);
    purple_blist_add_buddy(buddy, NULL, NULL, NULL);
    TOX_USERSTATUS userstatus = (TOX_USERSTATUS) tox_get_user_status(tox, friend_number);
    purple_debug_info("toxprpl", "Friend %s has status %d\n", buddy_key,
                      userstatus);
    purple_prpl_got_user_status(account, buddy_key,
                                ToxPRPL_ToxStatuses[
                                        ToxPRPL_getStatusTypeIndex(tox, friend_number, userstatus)].id,
                                NULL);
    g_free(buddy_key);
}

/*
 * Synchronize purple friends with tox friends
 */
static void ToxPRPL_synchronizeBuddyList(PurpleAccount* acct, Tox* tox) {
    uint32_t i;

    uint32_t fl_len = tox_count_friendlist(tox);
    int* friendlist = g_malloc0(fl_len * sizeof(int));

    fl_len = tox_get_friendlist(tox, friendlist, fl_len);
    if (fl_len != 0) {
        purple_debug_info("toxprpl", "got %u friends\n", fl_len);
        GSList* buddies = purple_find_buddies(acct, NULL);
        GSList* iterator;
        for (i = 0; i < fl_len; i++) {
            iterator = buddies;
            int fnum = friendlist[i];
            uint8_t bin_id[TOX_CLIENT_ID_SIZE];
            if (tox_get_client_id(tox, fnum, bin_id) == 0) {
                gchar* str_id = ToxPRPL_toxClientIdToString(bin_id);
                while (iterator != NULL) {
                    PurpleBuddy* buddy = iterator->data;
                    if (strcmp(buddy->name, str_id) == 0) {
                        ToxPRPL_BuddyData* buddy_data = g_new0(ToxPRPL_BuddyData, 1);
                        buddy_data->tox_friendlist_number = fnum;
                        purple_buddy_set_protocol_data(buddy, buddy_data);
                        friendlist[i] = -1;
                    }
                    iterator = iterator->next;
                }
                g_free(str_id);
            }
        }

        iterator = buddies;
        // all left without buddy_data were not present in Tox and must be
        // removed
        while (iterator != NULL) {
            PurpleBuddy* buddy = iterator->data;
            ToxPRPL_BuddyData* buddy_data =
                    purple_buddy_get_protocol_data(buddy);
            if (buddy_data == NULL) {
                purple_blist_remove_buddy(buddy);
            }
            iterator = iterator->next;
        }

        g_slist_free(buddies);
    }

    // all left in friendlist that were not reset are not yet in blist
    for (i = 0; i < fl_len; i++) {
        if (friendlist[i] != -1) {
            ToxPRPL_synchronizeBuddy(acct, tox, friendlist[i]);
        }
    }

    g_free(friendlist);
}

// ---- end ToxPRPL_synchronizeBuddyList ---------------------------------------------------------------------------------------


/*
 * Opens a dialog prompting the user to import a preexisting tox database
 */
static void ToxPRPL_promptAccountImport(PurpleAccount* acct) {
    purple_debug_info("toxprpl", "ask to import user account\n");
    PurpleConnection* gc = purple_account_get_connection(acct);

    purple_request_file(gc,
                        _("Import existing Tox account data"),
                        NULL,
                        FALSE,
                        G_CALLBACK(ToxPRPL_importUser),
                        G_CALLBACK(ToxPRPL_initializePRPL),
                        acct,
                        NULL,
                        NULL,
                        acct);
}

// ---- login/logout functions -----------------------------------------------------------------------------------------

/*
 * Start Tox and register all callbacks
 */
void ToxPRPL_configureToxAndConnect(PurpleAccount* acct) {
    purple_debug_info("toxprpl", "logging in...\n");

    PurpleConnection* gc = purple_account_get_connection(acct);

    Tox* tox = tox_new(0);
    if (tox == NULL) {
        purple_debug_info("toxprpl", "Fatal error, could not allocate memory for messenger!\n");
        return;
    }

    tox_callback_connection_status(tox, ToxPRPL_Tox_onUserConnectionStatusChange, gc);
    tox_callback_friend_request(tox, ToxPRPL_Tox_onFriendRequest, gc);
    tox_callback_friend_action(tox, ToxPRPL_Tox_onFriendAction, gc);
    tox_callback_friend_message(tox, ToxPRPL_Tox_onMessageReceived, gc);
    tox_callback_name_change(tox, ToxPRPL_Tox_onFriendChangeNickname, gc);
    tox_callback_user_status(tox, ToxPRPL_Tox_onFriendChangeStatus, gc);
    tox_callback_typing_change(tox, ToxPRPL_Tox_onUserTypingChange, gc);

    /*
     * Implemented in ``tox/group_chat.c''
     */

    /*
     * callback signature
     *
     *  (Tox* tox, int32_t friendNumber, uint8_t type,
     *   const uint8_t* data, uint16_t length, void* userData) => void
     *
     *   userData will be the PurpleConnection,
     *   `length` will be passed to join_groupchat,
     *   `type` is one of TOX_GROUPCHAT_TYPE_TEXT or TOX_GROUPCHAT_TYPE_AV,
     *      NOTE: for our purposes, we only want TOX_GROUPCHAT_TYPE_TEXT, as AV is not in the plans
     *
     */
    tox_callback_group_invite(tox, ToxPRPL_Tox_onGroupInvite, gc);

    /*
     * callback signature
     *
     *  (Tox* tox, int groupNumber, int peerNumber, const uint8_t* message,
     *   uint16_t length, void* userData) => void
     */
    tox_callback_group_message(tox, ToxPRPL_Tox_onGroupMessage, gc);

    /*
     * callback signature
     *
     *  (Tox* tox, int groupNumber, int peerNumber, const uint8_t* action,
     *   uint16_t length, void* userData) => void
     */
    tox_callback_group_action(tox, ToxPRPL_Tox_onGroupAction, gc);

    /*
     * callback signature
     *
     *  (Tox* tox, int groupNumber, int peerNumber, uint8_t* title,
     *   uint8_t length, void* userData) => void
     */
    tox_callback_group_title(tox, ToxPRPL_Tox_onGroupChangeTitle, gc);

    /*
     * callback signature
     *
     *  (Tox* tox, int groupNumber, int peerNumber, TOX_CHAT_CHANGE change,
     *   void* userData) => void
     */
    tox_callback_group_namelist_change(tox, ToxPRPL_Tox_onGroupNamelistChange, gc);

    /*
     * Implemented in ``tox/xfers.c''
     */
    tox_callback_file_send_request(tox, ToxPRPL_Tox_onFileRequest, gc);
    tox_callback_file_control(tox, ToxPRPL_Tox_onFileControl, gc);
    tox_callback_file_data(tox, ToxPRPL_Tox_onFileDataReceive, gc);

    purple_debug_info("toxprpl", "initialized tox callbacks\n");

    gc->flags |= PURPLE_CONNECTION_NO_FONTSIZE | PURPLE_CONNECTION_NO_URLDESC;
    gc->flags |= PURPLE_CONNECTION_NO_IMAGES | PURPLE_CONNECTION_NO_NEWLINES;

    purple_debug_info("toxprpl", "logging in %s\n", acct->username);

    const char* msg64 = purple_account_get_string(acct, "messenger", NULL);
    if ((msg64 != NULL) && (strlen(msg64) > 0)) {
        purple_debug_info("toxprpl", "found existing account data\n");
        gsize out_len;
        guchar* msg_data = g_base64_decode(msg64, &out_len);
        if (msg_data && (out_len > 0)) {
            if (tox_load(tox, msg_data, (uint32_t) out_len) != 0) {
                purple_debug_info("toxprpl", "Invalid account data\n");
                purple_account_set_string(acct, "messenger", NULL);
            }
            g_free(msg_data);
        }
    }
    else // write account into pidgin
    {
        ToxPRPL_saveAccount(acct, tox);
    }

    purple_connection_update_progress(gc, _("Connecting"),
                                      0,   /* which connection step this is */
                                      2);  /* total number of steps */


    const char* key = purple_account_get_string(acct, "dht_server_key",
                                                DEFAULT_SERVER_KEY);
    /// \todo add limits check to make sure the user did not enter something
    /// invalid
    uint16_t port = (uint16_t) purple_account_get_int(acct, "dht_server_port", DEFAULT_SERVER_PORT);

    const char* ip = purple_account_get_string(acct, "dht_server", DEFAULT_SERVER_IP);

    unsigned char* publicKey = ToxPRPL_hexStringToBin(key);

    purple_debug_info("toxprpl", "Will connect to %s:%d (%s)\n", ip, port, key);

    if (tox_bootstrap_from_address(tox, ip, port, publicKey) == 0) {
        purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("server invalid or not found"));
        g_free(publicKey);
        tox_kill(tox);
        return;
    }
    g_free(publicKey);

    ToxPRPL_synchronizeBuddyList(acct, tox);

    ToxPRPL_PluginData* plugin = g_new0(ToxPRPL_PluginData, 1);

    plugin->tox = tox;
    plugin->tox_timer = purple_timeout_add(80, ToxPRPL_updateConnectionState, gc);
    purple_debug_info("toxprpl", "added messenger timer as %d\n",
                      plugin->tox_timer);
    plugin->connection_timer = purple_timeout_add_seconds(2,
                                                          ToxPRPL_updateClientStatus,
                                                          gc);
    purple_debug_info("toxprpl", "added connection timer as %d\n",
                      plugin->connection_timer);


    gchar* myid_help = "myid  print your tox id which you can give to "
            "your friends";
    gchar* nick_help = "nick &lt;nickname&gt; set your nickname";

    plugin->myid_command_id = purple_cmd_register("myid", "",
                                                  PURPLE_CMD_P_DEFAULT, PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
                                                  TOXPRPL_ID, ToxPRPL_Command_myId, myid_help, gc);

    plugin->nick_command_id = purple_cmd_register("nick", "s",
                                                  PURPLE_CMD_P_DEFAULT, PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
                                                  TOXPRPL_ID, ToxPRPL_Command_nick, nick_help, gc);

    const char* nick = purple_account_get_string(acct, "nickname", NULL);
    if (!nick || (strlen(nick) == 0)) {
        nick = purple_account_get_username(acct);
        if (strlen(nick) == 0) {
            nick = purple_account_get_alias(acct);
        }

        if (strlen(nick) == 0) {
            nick = DEFAULT_NICKNAME;
        }
    }

    purple_connection_set_protocol_data(gc, plugin);
    ToxPRPL_Purple_onSetNickname(gc, nick);
}

/*
 * LibPurple login implementation
 * This is also used by ToxPRPL as the callback function for certain dialogues
 */
void ToxPRPL_initializePRPL(PurpleAccount* acct) {
    PurpleConnection* gc = purple_account_get_connection(acct);

    // check if we need to run first time setup
    if (purple_account_get_string(acct, "messenger", NULL) == NULL) {
        purple_request_action(gc,
                              _("Setup Tox account"),
                              _("This appears to be your first login to the Tox network, "
                                        "would you like to start with a new Tox user ID or would you "
                                        "like to import an existing one?"),
                              _("Note: you can export / backup your account via the account "
                                        "actions menu."),
                              PURPLE_DEFAULT_ACTION_NONE,
                              acct, NULL, NULL,
                              acct, // user data
                              2,    // 2 choices
                              _("Import existing Tox account"),
                              G_CALLBACK(ToxPRPL_promptAccountImport),
                              _("Create new Tox account"),
                              G_CALLBACK(ToxPRPL_configureToxAndConnect));

        purple_notify_warning(gc,
                              _("Development Version Warning"),
                              _("This plugin is based on a development version of the "
                                        "Tox library. There has not yet been an alpha nor a beta "
                                        "release, the library is still 'work in progress' in "
                                        "pre-alpha state.\n\n"
                                        "This means that your conversations MAY NOT YET BE "
                                        "SECURE!"), NULL);
    } else {
        ToxPRPL_configureToxAndConnect(acct);
    }
}

/*
 * LibPurple close callback
 * This should clean up and end the Tox session
 */
void ToxPRPL_shutdownPRPL(PurpleConnection* gc) {
    /* notify other toxprpl accounts */
    purple_debug_info("toxprpl", "Closing!\n");

    PurpleAccount* account = purple_connection_get_account(gc);
    ToxPRPL_PluginData* plugin = purple_connection_get_protocol_data(gc);
    if (plugin == NULL) {
        return;
    }

    if (plugin->tox == NULL) {
        g_free(plugin);
        purple_connection_set_protocol_data(gc, NULL);
        return;
    }

    purple_debug_info("toxprpl", "removing timers %d and %d\n",
                      plugin->tox_timer, plugin->connection_timer);
    purple_timeout_remove(plugin->tox_timer);
    purple_timeout_remove(plugin->connection_timer);

    purple_cmd_unregister(plugin->myid_command_id);
    purple_cmd_unregister(plugin->nick_command_id);

    if (!ToxPRPL_saveAccount(account, plugin->tox)) {
        purple_account_set_string(account, "messenger", "");
    }

    purple_debug_info("toxprpl", "shutting down\n");
    purple_connection_set_protocol_data(gc, NULL);
    tox_kill(plugin->tox);
    g_free(plugin);
}

// ---- end login/logout -----------------------------------------------------------------------------------------------

/*
 * LibPurple callback to destruct a buddy
 */
static void ToxPRPL_destroyBuddy(PurpleBuddy* buddy) {
    if (buddy->proto_data) {
        ToxPRPL_BuddyData* buddy_data = buddy->proto_data;
        g_free(buddy_data);
    }
}

// ---- plugin initialization ------------------------------------------------------------------------------------------

static PurplePluginProtocolInfo ToxPRPL_PRPL_Info = {
        // Protocol ----------------------------------------------------------------------------------------------------

        .options = OPT_PROTO_NO_PASSWORD | OPT_PROTO_REGISTER_NOSCREENNAME | OPT_PROTO_INVITE_MESSAGE,

        .login = ToxPRPL_initializePRPL,
        .close = ToxPRPL_shutdownPRPL,

        // .user_splits is assigned in ToxPRPL_initPRPL
        // .protocol_options is assign in ToxPRPL_initPRPL

        // Self --------------------------------------------------------------------------------------------------------

        /*
         * set_status lives inside ``purple/account.c''
         */
        .set_status = ToxPRPL_Purple_onSetStatus,

        // Buddy Icons, status -----------------------------------------------------------------------------------------

        /*
         *  Right now, this is a null implementation of PurpleBuddyIconSpec
         *  For tox support, this will need to describe
         *  - format: png
         *  - width constraints describing least and worst case image sizes from tox
         *  - largest possible filesize that can be returned by tox
         *  - scale: preserve ratio
         *
         *  How this is handled in reality depends on the frontent implementation.
         *  Pidgin should be pretty obediant.
         */
        .icon_spec = NO_BUDDY_ICONS,

        /*
         * If the icon spec is changed to support protocol icons,
         * this `toxpripl_list_icon` should return the name to pass to tox
         * in order to get the icon.
         * This does not return image data, just a string.
         */
        .list_icon = ToxPRPL_Purple_getListIconForUser,

        /*
         * Function which will return a buddy/user's status text
         */
        .status_text = NULL,

        /*
         * Possible states for any user
         */
        .status_types = ToxPRPL_getStatusTypes,

        /*
         * Buddy tooltip.
         * Perhaps the Tox id could go here, for quick reference
         */
        .tooltip_text = NULL,

        /*
         * Buddy menu additions,
         * perhaps a `copy id` function could go here
         */
        .blist_node_menu = NULL,

        // Buddy Management --------------------------------------------------------------------------------------------

        /*
         * These (and dependant) functions may be found in ``purple/buddy.c''
         */

        /*
         * Replace add_buddy, add_buddies respectively
         * Whether plural add should be implemented depends on further reading of the documentation
         */
        .add_buddy_with_invite = ToxPRPL_Purple_addBuddy,
        .add_buddies_with_invite = NULL,

        .remove_buddy = ToxPRPL_Purple_removeBuddy,
        .remove_buddies = NULL,

        /*
         * Blocking methods.
         * No significant documentation is available, but names lend themselves to being understood.
         */
        .add_permit = NULL,
        .add_deny = NULL,
        .rem_permit = NULL,
        .rem_deny = NULL,
        .set_permit_deny = NULL,

        // All Chats ---------------------------------------------------------------------------------------------------

        /*
         * These functions may be found in ``purple/chat.c''
         */

        .send_im = ToxPRPL_Purple_sendUserMessage,
        .send_typing = ToxPRPL_Purple_updateTypingState,
        .offline_message = NULL,

        // Group Chats -------------------------------------------------------------------------------------------------

        /*
         * Called when the frontend wants to join a chat
         * Parameter components is as `chat_info` unless the user has accepted an invite,
         * in which case it is as in `serv_got_chat_invite`
         *
         * TODO: Implement
         *
         * related tox functions: `tox_add_groupchat`, and `tox_join_groupchat`
         *
         * callback signature: (PurpleConnection*, GHashTable* components) => void
         */
        .join_chat = NULL,

        /*
         * Returns a list of chats in which the user is enrolled
         *
         * TODO: Implement
         *
         * related tox functions: `tox_count_chatlist`, and `tox_get_chatlist`
         *
         * callback signature: (PurpleConnection*) => GList* [struct proto_chat_entry]
         */
        .chat_info = NULL,

        /*
         * Returns a map representing default chat options
         *
         * TODO: Implement
         *
         * callback signature: (PurpleConnection*, const char* chatName)
         *                      => GHashTable* [(struct proto_chat_entry) -> char*]
         */
        .chat_info_defaults = NULL,

        /*
         * Called when the frontend rejects a chat invite
         *
         * TODO: Implement
         *
         * no related tox function, just clean up components if necessary
         *
         * callback signature: (PurpleConnection*, GHashTable* components) => void
         */
        .reject_chat = NULL,

        /*
         * Get the display name of a chat from the internal representation
         *
         * TODO: Implement
         *
         * related tox function: `tox_group_get_title`
         *
         * callback signature: (GHashTable* components) => char*
         */
        .get_chat_name = NULL,

        /*
         * Invite a user, `who`, to join chat `id`, with message `message`
         *
         * TODO: Implement
         *
         * related tox function: `tox_invite_friend`
         *
         * callback signature: (PurpleConnection*, int chatId, const char* inviteMessage,
         *                      const char* buddyName) => void
         */
        .chat_invite = NULL,

        /*
         * Leave chat `id`
         *
         * TODO: Implement
         *
         * related tox function: `tox_del_groupchat`
         *
         * callback signature: (PurpleConnection*, int chatId) => void
         */
        .chat_leave = NULL,

        /*
         * Send a message in a chat, `id`.
         *
         * TODO: Implement
         *
         * related tox function: `tox_group_message_send`
         *
         * callback signature: (PurpleConnection*, int chatId, const char* message,
         *                      PurpleMessageFlags flags) => int
         *
         *                     returns >= 0 if successful
         */
        .chat_send = NULL,

        /*
         * Set a group title
         *
         * TODO: Implement
         *
         * related tox function: `tox_group_get_title`
         *
         * callback signature: (PurpleConnection*, int chatId, const char* topic) => void
         */
        .set_chat_topic = NULL,

        // File transfers ----------------------------------------------------------------------------------------------

        // These and related functions are defined in ``common/xfers.c''

        .can_receive_file   = ToxPRPL_Purple_canReceiveFileCheck,
        .send_file          = ToxPRPL_Purple_sendFile,
        .new_xfer           = ToxPRPL_newXfer,

        // ToxAV -------------------------------------------------------------------------------------------------------

        .initiate_media = NULL,
        .get_media_caps = NULL,

        // API ---------------------------------------------------------------------------------------------------------

        .buddy_free = ToxPRPL_destroyBuddy,
        .struct_size = sizeof(PurplePluginProtocolInfo),
};

static PurplePluginInfo ToxPRPL_PRPL_Manifest = {
        .magic = PURPLE_PLUGIN_MAGIC,
        .major_version = PURPLE_MAJOR_VERSION,
        .minor_version = PURPLE_MINOR_VERSION,
        .type = PURPLE_PLUGIN_PROTOCOL,
        .priority = PURPLE_PRIORITY_DEFAULT,

        .id = TOXPRPL_ID,
        .name = "Tox",
        .version = VERSION,
        .summary = "Tox Protocol Plugin",
        .description = "Tox Protocol Plugin http://tox.im/",
        .author = "Sergey 'Jin' Bostandzhyan",
        .homepage = PACKAGE_URL,

        .prefs_info = NULL,

        .extra_info = &ToxPRPL_PRPL_Info,
        .actions = ToxPRPL_Purple_getAccountActions,
};

static void ToxPRPL_initPRPL(PurplePlugin* plugin) {
    purple_debug_info("toxprpl", "starting up\n");

    PurpleAccountOption* option = purple_account_option_string_new(
            _("Nickname"), "nickname", "");
    ToxPRPL_PRPL_Info.protocol_options = g_list_append(NULL, option);

    option = purple_account_option_string_new(
            _("Server"), "dht_server", DEFAULT_SERVER_IP);
    ToxPRPL_PRPL_Info.protocol_options = g_list_append(ToxPRPL_PRPL_Info.protocol_options, option);

    option = purple_account_option_int_new(_("Port"), "dht_server_port",
                                           DEFAULT_SERVER_PORT);
    ToxPRPL_PRPL_Info.protocol_options = g_list_append(ToxPRPL_PRPL_Info.protocol_options, option);

    option = purple_account_option_string_new(_("Server key"),
                                              "dht_server_key", DEFAULT_SERVER_KEY);
    ToxPRPL_PRPL_Info.protocol_options = g_list_append(ToxPRPL_PRPL_Info.protocol_options, option);

    purple_debug_info("toxprpl", "initialization complete\n");
}

PURPLE_INIT_PLUGIN(tox, ToxPRPL_initPRPL, ToxPRPL_PRPL_Manifest);
