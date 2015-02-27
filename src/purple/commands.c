/*
 * Contains commands provided by ToxPRPL for use in the frontend as
 * ``/'' commands.
 */

#include <toxprpl.h>
#include <toxprpl/account.h>

/*
 * /myid command
 */
PurpleCmdRet ToxPRPL_Command_myId(PurpleConversation* conv, const gchar* cmd, gchar** args, gchar** error, void* data) {
    purple_debug_info("toxprpl", "/myid command detected\n");
    PurpleConnection* gc = (PurpleConnection*) data;
    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);

    uint8_t bin_id[TOX_FRIEND_ADDRESS_SIZE];
    tox_get_address(plugin->tox, bin_id);
    gchar* id = toxprpl_tox_friend_id_to_string(bin_id);

    gchar* message = g_strdup_printf(_("If someone wants to add you, give them "
                                               "this id: %s"), id);

    purple_conversation_write(conv, NULL, message, PURPLE_MESSAGE_SYSTEM,
                              time(NULL));
    g_free(id);
    g_free(message);
    return PURPLE_CMD_RET_OK;
}

/*
 * /nick command
 */
PurpleCmdRet ToxPRPL_Command_nick(PurpleConversation* conv, const gchar* cmd, gchar** args, gchar** error, void* data) {
    purple_debug_info("toxprpl", "/nick %s command detected\n", args[0]);
    PurpleConnection* gc = (PurpleConnection*) data;
    ToxPRPL_Purple_onSetNickname(gc, args[0]);
    return PURPLE_CMD_RET_OK;
}

