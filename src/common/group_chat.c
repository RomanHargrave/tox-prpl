#include <toxprpl/group_chat.h>
#include <conversation.h>

const char* TOXPRPL_BUDDY_PEERID = "tox.peer_id";

static gint compareByPeerId(gconstpointer a, gconstpointer b) {

    PurpleConvChatBuddy* firstBuddy  = (PurpleConvChatBuddy*) a;
    PurpleConvChatBuddy* secondBuddy = (PurpleConvChatBuddy*) b;

    if(!firstBuddy || !secondBuddy) {
        return -1;
    }

    const char* firstId = purple_conv_chat_cb_get_attribute(firstBuddy, TOXPRPL_BUDDY_PEERID);
    const char* secondId = purple_conv_chat_cb_get_attribute(secondBuddy, TOXPRPL_BUDDY_PEERID);

    return g_strcmp0(firstId, secondId);
}

GList* ToxPRPL_Purple_findByPeerId(PurpleConvChat* chat, int peerId) {

    GList* users = purple_conv_chat_get_users(chat);

    // Create a dummy chat buddy that has the peerId attribute we want

    char* peerIdString = g_strdup_printf("%i", peerId);

    PurpleConvChatBuddy* idealBuddy =
            purple_conv_chat_cb_new("nobody", "nobody", PURPLE_CBFLAGS_NONE);
    purple_conv_chat_cb_set_attribute(NULL, idealBuddy, TOXPRPL_BUDDY_PEERID, peerIdString);

    g_free(peerIdString);

    // Create the list of chat buddies that match

    GList* matchingUsers = g_list_find_custom(users, NULL, compareByPeerId);

    // Clean up the dummy user

    purple_conv_chat_cb_destroy(idealBuddy);

    // Return matching users

    return matchingUsers;
}