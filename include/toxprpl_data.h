#pragma once

typedef struct _toxprpl_status {
    PurpleStatusPrimitive primitive;
    uint8_t tox_status;
    gchar* id;
    gchar* title;
} ToxPRPL_Status;

typedef struct _toxprpl_buddy_data {
    int tox_friendlist_number;
} ToxPRPL_BuddyData;

typedef struct _toxprpl_friend_accept_data {
    PurpleConnection* gc;
    char* buddy_key;
} ToxPRPL_FriendAcceptData;

typedef struct _toxprpl_plugin_data {
    Tox* tox;
    guint tox_timer;
    guint connection_timer;
    guint connected;
    PurpleCmdId myid_command_id;
    PurpleCmdId nick_command_id;
} ToxPRPL_PluginData;

typedef struct _toxprpl_idle_write_data {
    PurpleXfer* xfer;
    uint8_t* buffer;
    uint8_t* offset;
    gboolean running;
} ToxPRPL_IdleWriteData;

typedef struct _toxprpl_xfer_data {
    Tox* tox;
    int friendnumber;
    uint8_t filenumber;
    ToxPRPL_IdleWriteData* idle_write_data;
} ToxPRPL_XferData;