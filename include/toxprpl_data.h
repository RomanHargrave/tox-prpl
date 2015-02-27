#pragma once

typedef struct {
    PurpleStatusPrimitive primitive;
    uint8_t tox_status;
    gchar* id;
    gchar* title;
} toxprpl_status;

typedef struct {
    int tox_friendlist_number;
} toxprpl_buddy_data;

typedef struct {
    PurpleConnection* gc;
    char* buddy_key;
} toxprpl_accept_friend_data;

typedef struct {
    Tox* tox;
    guint tox_timer;
    guint connection_timer;
    guint connected;
    PurpleCmdId myid_command_id;
    PurpleCmdId nick_command_id;
} toxprpl_plugin_data;

typedef struct {
    PurpleXfer* xfer;
    uint8_t* buffer;
    uint8_t* offset;
    gboolean running;
} IdleWriteData;

typedef struct {
    Tox* tox;
    int friendnumber;
    uint8_t filenumber;
    IdleWriteData* idle_write_data;
} toxprpl_xfer_data;