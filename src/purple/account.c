/*
 * Contains the implementation for LibPurple account management callbacks
 *
 */

#include <toxprpl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib/gstdio.h>

#include <toxprpl/protocol.h>

// Account Overall ----------------------------------------------------------------------------

/*
 * Import a Tox account from `filename` into Purple account `acct`
 */
void ToxPRPL_importUser(PurpleAccount* acct, const char* filename) {
    purple_debug_info("toxprpl", "import user account: %s\n", filename);

    PurpleConnection* gc = purple_account_get_connection(acct);

    GStatBuf sb;
    if (g_stat(filename, &sb) != 0) {
        purple_notify_message(gc,
                              PURPLE_NOTIFY_MSG_ERROR,
                              _("Error"),
                              _("Could not access account data file:"),
                              filename,
                              (PurpleNotifyCloseCallback) ToxPRPL_initializePRPL,
                              acct);
        return;
    }

    if ((sb.st_size == 0) || (sb.st_size > MAX_ACCOUNT_DATA_SIZE)) {
        purple_notify_message(gc,
                              PURPLE_NOTIFY_MSG_ERROR,
                              _("Error"),
                              _("Account data file seems to be invalid"),
                              NULL,
                              (PurpleNotifyCloseCallback) ToxPRPL_initializePRPL,
                              acct);
        return;
    }

    int fd = open(filename, O_RDONLY | O_BINARY);
    if (fd == -1) {
        purple_notify_message(gc,
                              PURPLE_NOTIFY_MSG_ERROR,
                              _("Error"),
                              _("Could not open account data file:"),
                              strerror(errno),
                              (PurpleNotifyCloseCallback) ToxPRPL_initializePRPL,
                              acct);
        return;
    }

    guchar* account_data = g_malloc0(sb.st_size);
    guchar* p = account_data;
    size_t remaining = sb.st_size;
    while (remaining > 0) {
        ssize_t rb = read(fd, p, remaining);
        if (rb < 0) {
            purple_notify_message(gc,
                                  PURPLE_NOTIFY_MSG_ERROR,
                                  _("Error"),
                                  _("Could not read account data file:"),
                                  strerror(errno),
                                  (PurpleNotifyCloseCallback) ToxPRPL_initializePRPL,
                                  acct);
            g_free(account_data);
            close(fd);
            return;
        }
        remaining = remaining - rb;
        p = p + rb;
    }

    gchar* msg64 = g_base64_encode(account_data, sb.st_size);
    purple_account_set_string(acct, "messenger", msg64);
    g_free(msg64);
    g_free(account_data);
    ToxPRPL_initializePRPL(acct);
    close(fd);
}

/*
 * Export a Tox account to `filename`
 */
void ToxPRPL_exportUser(PurpleConnection* gc, const char* filename) {
    purple_debug_info("toxprpl", "export account to %s\n", filename);

    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);

    // When is this a problem?
    //  and why is there no debug statement?
    if(!plugin || !plugin->tox) return;

    PurpleAccount* account = purple_connection_get_account(gc);

    uint32_t msg_size = tox_size(plugin->tox);
    if (msg_size > 0) {
        uint8_t* account_data = g_malloc0(msg_size);
        tox_save(plugin->tox, account_data);
        guchar* p = account_data;

        int fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            g_free(account_data);
            purple_notify_message(gc,
                                  PURPLE_NOTIFY_MSG_ERROR,
                                  _("Error"),
                                  _("Could not save account data file:"),
                                  strerror(errno),
                                  NULL, NULL);
            return;
        }

        size_t remaining = (size_t) msg_size;
        while (remaining > 0) {
            ssize_t wb = write(fd, p, remaining);
            if (wb < 0) {
                purple_notify_message(gc,
                                      PURPLE_NOTIFY_MSG_ERROR,
                                      _("Error"),
                                      _("Could not save account data file:"),
                                      strerror(errno),
                                      (PurpleNotifyCloseCallback) ToxPRPL_initializePRPL,
                                      account);
                g_free(account_data);
                close(fd);
                return;
            }
            remaining = remaining - wb;
            p = p + wb;
        }

        g_free(account_data);
        close(fd);
    }
}

// TODO common/accounts?
gboolean ToxPRPL_saveAccount(PurpleAccount* account, Tox* tox) {
    uint32_t msg_size = tox_size(tox);
    if (msg_size > 0) {
        guchar* msg_data = g_malloc0(msg_size);
        tox_save(tox, msg_data);
        gchar* msg64 = g_base64_encode(msg_data, msg_size);
        purple_account_set_string(account, "messenger", msg64);
        g_free(msg64);
        g_free(msg_data);
        return TRUE;
    }

    return FALSE;
}

// Nickname -----------------------------

/*
 * Purple callback for the nickname settings dialog
 */
void ToxPRPL_Purple_onSetNickname(PurpleConnection* gc, const char* nickname) {
    PurpleAccount* account = purple_connection_get_account(gc);
    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);
    if (nickname != NULL) {
        purple_connection_set_display_name(gc, nickname);
        tox_set_name(plugin->tox, (uint8_t*) nickname, (uint16_t) (strlen(nickname) + 1));
        purple_account_set_string(account, "nickname", nickname);
    }
}

// Status ---------------------------------

/*
 * LibPurple callback that is invoked when the frontend wants to set the user status
 */
void ToxPRPL_Purple_onSetStatus(PurpleAccount* account, PurpleStatus* status) {
    const char* status_id = purple_status_get_id(status);
    const char* message = purple_status_get_attr_string(status, "message");

    PurpleConnection* gc = purple_account_get_connection(account);
    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);

    purple_debug_info("toxprpl", "setting status %s\n", status_id);

    TOX_USERSTATUS tox_status = ToxPRPL_getStatusTypeById(status_id);
    if (tox_status == TOX_USERSTATUS_INVALID) {
        purple_debug_info("toxprpl", "status %s is invalid\n", status_id);
        return;
    }

    tox_set_user_status(plugin->tox, tox_status);
    if ((message != NULL) && (strlen(message) > 0)) {
        tox_set_status_message(plugin->tox, (uint8_t*) message, (uint16_t) (strlen(message) + 1));
    }
}

// Account Protocol -----------------------------------

// Frontend Glue ---------------------------------

void ToxPRPL_showIDNumberDialog(PurplePluginAction* action) {
    PurpleConnection* gc = (PurpleConnection*) action->context;

    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);

    uint8_t bin_id[TOX_FRIEND_ADDRESS_SIZE];
    tox_get_address(plugin->tox, bin_id);
    gchar* id = ToxPRPL_toxFriendIdToString(bin_id);

    purple_notify_message(gc,
                          PURPLE_NOTIFY_MSG_INFO,
                          _("Account ID"),
                          _("If someone wants to add you, give them this Tox ID:"),
                          id,
                          (PurpleNotifyCloseCallback) g_free,
                          id);
}

void ToxPRPL_showSitNicknameDialog(PurplePluginAction* action) {
    PurpleConnection* gc = (PurpleConnection*) action->context;
    PurpleAccount* account = purple_connection_get_account(gc);

    purple_request_input(gc, _("Set nickname"),
                         _("New nickname:"),
                         NULL,
                         purple_account_get_string(account, "nickname", ""),
                         FALSE, FALSE, NULL,
                         _("_Set"), G_CALLBACK(ToxPRPL_Purple_onSetNickname),
                         _("_Cancel"), NULL,
                         account, account->username, NULL,
                         gc);
}

void ToxPRPL_showExportDialog(PurplePluginAction* action) {
    purple_debug_info("toxprpl", "ask to export account\n");

    PurpleConnection* gc = (PurpleConnection*) action->context;
    PurpleAccount* account = purple_connection_get_account(gc);
    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);

    // When is this a problem?
    //  and why is there no debug statement?
    if(!plugin || !plugin->tox) return;

    uint8_t bin_id[TOX_FRIEND_ADDRESS_SIZE];
    tox_get_address(plugin->tox, bin_id);
    gchar* id = ToxPRPL_toxFriendIdToString(bin_id);
    strcpy(id + TOX_CLIENT_ID_SIZE, ".tox\0"); // insert extension instead of nospam

    purple_request_file(gc,
                        _("Export existing Tox account data"),
                        id,
                        TRUE,
                        G_CALLBACK(ToxPRPL_exportUser),
                        NULL,
                        account,
                        NULL,
                        NULL,
                        gc);
    g_free(id);
}

/*
 * Returns a list of things that can be done with the account used by LibPurple
 * Sets up callbacks to
 * - toxprpl_acion_show_id_dialog
 * - ToxPRPL_showSitNicknameDialog
 * - ToxPRPL_showExportDialog
 */
GList* ToxPRPL_Purple_getAccountActions(PurplePlugin* plugin, gpointer context) {
    purple_debug_info("toxprpl", "setting up account actions\n");

    GList* actions = NULL;
    PurplePluginAction* action;

    action = purple_plugin_action_new(_("Show my id..."),
                                      ToxPRPL_showIDNumberDialog);
    actions = g_list_append(actions, action);

    action = purple_plugin_action_new(_("Set nickname..."),
                                      ToxPRPL_showSitNicknameDialog);
    actions = g_list_append(actions, action);

    action = purple_plugin_action_new(_("Export account data..."),
                                      ToxPRPL_showExportDialog);
    actions = g_list_append(actions, action);
    return actions;
}
