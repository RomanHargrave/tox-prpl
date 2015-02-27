#include <toxprpl.h>
#include <toxprpl/xfers.h>

#include <string.h>

void ToxPRPL_Purple_prepareXfer(PurpleXfer* xfer) {
    purple_debug_info("toxprpl", "xfer_init\n");
    toxprpl_return_if_fail(xfer != NULL);

    toxprpl_xfer_data* xfer_data = xfer->data;
    toxprpl_return_if_fail(xfer_data != NULL);

    if (purple_xfer_get_type(xfer) == PURPLE_XFER_SEND) {
        PurpleAccount* account = purple_xfer_get_account(xfer);
        toxprpl_return_if_fail(account != NULL);

        PurpleConnection* gc = purple_account_get_connection(account);
        toxprpl_return_if_fail(gc != NULL);

        toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);
        toxprpl_return_if_fail(plugin != NULL && plugin->tox != NULL);

        const char* who = purple_xfer_get_remote_user(xfer);
        toxprpl_return_if_fail(who != NULL);

        PurpleBuddy* buddy = purple_find_buddy(account, who);
        toxprpl_return_if_fail(buddy != NULL);

        toxprpl_buddy_data* buddy_data = purple_buddy_get_protocol_data(buddy);
        toxprpl_return_if_fail(buddy_data != NULL);

        int friendnumber = buddy_data->tox_friendlist_number;
        size_t filesize = purple_xfer_get_size(xfer);
        const char* filename = purple_xfer_get_filename(xfer);

        purple_debug_info("toxprpl", "sending xfer request for file '%s'.\n",
                          filename);
        int filenumber = tox_new_file_sender(plugin->tox, friendnumber, filesize,
                                             (uint8_t*) filename, strlen(filename) + 1);
        toxprpl_return_if_fail(filenumber >= 0);

        xfer_data->tox = plugin->tox;
        xfer_data->friendnumber = buddy_data->tox_friendlist_number;
        xfer_data->filenumber = filenumber;
    }
    else if (purple_xfer_get_type(xfer) == PURPLE_XFER_RECEIVE) {
        tox_file_send_control(xfer_data->tox, xfer_data->friendnumber, 1,
                              xfer_data->filenumber, TOX_FILECONTROL_ACCEPT, NULL, 0);
        purple_xfer_start(xfer, -1, NULL, 0);
    }
}


gboolean ToxPRPL_writeIdleData(IdleWriteData* data) {
    toxprpl_return_val_if_fail(data != NULL, FALSE);
    // If running is false the transfer was stopped and data->xfer
    // may have been deleted already
    if (data->running != FALSE) {
        size_t bytes_remaining = purple_xfer_get_bytes_remaining(data->xfer);
        if (data->xfer != NULL &&
            bytes_remaining > 0 &&
            !purple_xfer_is_canceled(data->xfer)) {
            gssize wrote = purple_xfer_write(data->xfer, data->offset, bytes_remaining);
            if (wrote > 0) {
                purple_xfer_set_bytes_sent(data->xfer, data->offset - data->buffer + wrote);
                purple_xfer_update_progress(data->xfer);
                data->offset += wrote;
            }
            return TRUE;
        }
        purple_debug_info("toxprpl", "ending file transfer\n");
        purple_xfer_end(data->xfer);
    }
    purple_debug_info("toxprpl", "freeing buffer\n");
    g_free(data->buffer);
    g_free(data);
    return FALSE;
}

void ToxPRPL_Purple_startXfer(PurpleXfer* xfer) {
    purple_debug_info("toxprpl", "xfer_start\n");
    toxprpl_return_if_fail(xfer != NULL);
    toxprpl_return_if_fail(xfer->data != NULL);

    toxprpl_xfer_data* xfer_data = xfer->data;

    if (purple_xfer_get_type(xfer) == PURPLE_XFER_SEND) {
        //copy whole file into memory
        size_t bytes_remaining = purple_xfer_get_bytes_remaining(xfer);
        uint8_t* buffer = g_malloc(bytes_remaining);
        uint8_t* offset = buffer;

        toxprpl_return_if_fail(buffer != NULL);
        size_t read_bytes = fread(buffer, sizeof(uint8_t), bytes_remaining, xfer->dest_fp);
        if (read_bytes != bytes_remaining) {
            purple_debug_warning("toxprpl", "read_bytes != bytes_remaining\n");
            g_free(buffer);
            return;
        }

        IdleWriteData* data = g_new0(IdleWriteData, 1);
        if (data == NULL) {
            purple_debug_warning("toxprpl", "data == NULL");
            g_free(buffer);
            return;
        }
        data->xfer = xfer;
        data->buffer = buffer;
        data->offset = offset;
        data->running = TRUE;
        xfer_data->idle_write_data = data;

        g_idle_add((GSourceFunc) ToxPRPL_writeIdleData, data);
    }
}

/*
 * LibPurple function that is called during an outgoing file transfer in order to write
 * data to the recipient
 */
gssize ToxPRPL_Purple_writeXfer(const guchar* data, size_t len, PurpleXfer* xfer) {
    toxprpl_return_val_if_fail(data != NULL, -1);
    toxprpl_return_val_if_fail(len > 0, -1);
    toxprpl_return_val_if_fail(xfer != NULL, -1);
    toxprpl_xfer_data* xfer_data = xfer->data;
    toxprpl_return_val_if_fail(xfer_data != NULL, -1);

    toxprpl_return_val_if_fail(purple_xfer_get_type(xfer) == PURPLE_XFER_SEND, -1);

    len = MIN((size_t) tox_file_data_size(xfer_data->tox,
                                          xfer_data->friendnumber), len);
    int ret = tox_file_send_data(xfer_data->tox, xfer_data->friendnumber,
                                 xfer_data->filenumber, (guchar*) data, len);

    if (ret != 0) {
        tox_do(xfer_data->tox);
        return -1;
    }
    return len;
}

/*
 * LibPurple callback dummy
 */
gssize ToxPRPL_purpleDummyReadXfer(guchar** data, PurpleXfer* xfer) {
    //dummy callback
    return -1;
}

/*
 * General-purpose garbage collection function.
 * Frees up resources used during a file transfer
 */
void ToxPRPL_freeXfer(PurpleXfer* xfer) {
    purple_debug_info("toxprpl", "xfer_free\n");
    toxprpl_return_if_fail(xfer != NULL);
    toxprpl_return_if_fail(xfer->data != NULL);

    toxprpl_xfer_data* xfer_data = xfer->data;

    if (xfer_data->idle_write_data != NULL) {
        IdleWriteData* idle_write_data = xfer_data->idle_write_data;
        idle_write_data->running = FALSE;
        xfer_data->idle_write_data = NULL;
    }
    g_free(xfer_data);
    xfer->data = NULL;
}

/*
 * LibPurple callback that is invoked when the frontend declines an incoming request
 * to receive a file transfer
 */
void ToxPRPL_Purple_incomingTransferDenied(PurpleXfer* xfer) {
    purple_debug_info("toxprpl", "xfer_request_denied\n");
    toxprpl_return_if_fail(xfer != NULL);
    toxprpl_return_if_fail(xfer->data != NULL);

    toxprpl_xfer_data* xfer_data = xfer->data;
    if (xfer_data->tox != NULL) {
        tox_file_send_control(xfer_data->tox, xfer_data->friendnumber, 1,
                              xfer_data->filenumber, TOX_FILECONTROL_KILL, NULL, 0);
    }
    ToxPRPL_freeXfer(xfer);
}

/*
 * LibPurple callback that is invoked when the frontend wishes to cancel an incoming file
 * transfer
 */
void ToxPRPL_Purple_cancelIncomingTransfer(PurpleXfer* xfer) {
    purple_debug_info("toxprpl", "xfer_cancel_recv\n");
    toxprpl_return_if_fail(xfer != NULL);
    toxprpl_xfer_data* xfer_data = xfer->data;

    if (xfer_data->tox != NULL) {
        tox_file_send_control(xfer_data->tox, xfer_data->friendnumber,
                              1, xfer_data->filenumber, TOX_FILECONTROL_KILL, NULL, 0);
    }
    ToxPRPL_freeXfer(xfer);
}

/*
 * LibPurple callback referenced when configuring a new outgoing transfer.
 * Invoked when the frontend wishes to cancel an outgoing transfer.
 */
void ToxPRPL_Purple_cancelOutgoingXfer(PurpleXfer* xfer) {
    purple_debug_info("toxprpl", "xfer_cancel_send\n");
    toxprpl_return_if_fail(xfer != NULL);
    toxprpl_return_if_fail(xfer->data != NULL);

    toxprpl_xfer_data* xfer_data = xfer->data;

    if (xfer_data->tox != NULL) {
        tox_file_send_control(xfer_data->tox, xfer_data->friendnumber,
                              0, xfer_data->filenumber, TOX_FILECONTROL_KILL, NULL, 0);
    }
    ToxPRPL_freeXfer(xfer);
}

/*
 * LibPurple callback that is invoked when a file transfer is completed
 */
void ToxPRPL_Purple_onTransferCompleted(PurpleXfer* xfer) {
    purple_debug_info("toxprpl", "xfer_end\n");
    toxprpl_return_if_fail(xfer != NULL);
    toxprpl_xfer_data* xfer_data = xfer->data;

    if (purple_xfer_get_type(xfer) == PURPLE_XFER_SEND) {
        tox_file_send_control(xfer_data->tox, xfer_data->friendnumber,
                              0, xfer_data->filenumber, TOX_FILECONTROL_FINISHED, NULL, 0);
    }
    else {
        tox_file_send_control(xfer_data->tox, xfer_data->friendnumber,
                              1, xfer_data->filenumber, TOX_FILECONTROL_FINISHED, NULL, 0);
    }

    ToxPRPL_freeXfer(xfer);
}

PurpleXfer* ToxPRPL_Purple_onTransferReceive(PurpleConnection* gc, const char* who, int friendnumber, int filenumber,
                                             const goffset filesize, const char* filename) {

    purple_debug_info("toxprpl", "new_xfer_receive\n");
    toxprpl_return_val_if_fail(gc != NULL, NULL);
    toxprpl_return_val_if_fail(who != NULL, NULL);

    PurpleAccount* account = purple_connection_get_account(gc);
    toxprpl_return_val_if_fail(account != NULL, NULL);

    PurpleXfer* xfer = purple_xfer_new(account, PURPLE_XFER_RECEIVE, who);
    toxprpl_return_val_if_fail(xfer != NULL, NULL);

    toxprpl_xfer_data* xfer_data = g_new0(toxprpl_xfer_data, 1);
    toxprpl_return_val_if_fail(xfer_data != NULL, NULL);

    toxprpl_plugin_data* plugin_data = purple_connection_get_protocol_data(gc);
    toxprpl_return_val_if_fail(plugin_data != NULL, NULL);

    xfer_data->tox = plugin_data->tox;
    xfer_data->friendnumber = friendnumber;
    xfer_data->filenumber = filenumber;
    xfer->data = xfer_data;

    purple_xfer_set_filename(xfer, filename);
    purple_xfer_set_size(xfer, filesize);

    purple_xfer_set_init_fnc(xfer, ToxPRPL_Purple_prepareXfer);
    purple_xfer_set_start_fnc(xfer, ToxPRPL_Purple_startXfer);
    purple_xfer_set_write_fnc(xfer, ToxPRPL_Purple_writeXfer);
    purple_xfer_set_read_fnc(xfer, ToxPRPL_purpleDummyReadXfer);
    purple_xfer_set_request_denied_fnc(xfer, ToxPRPL_Purple_incomingTransferDenied);
    purple_xfer_set_cancel_recv_fnc(xfer, ToxPRPL_Purple_cancelIncomingTransfer);
    purple_xfer_set_end_fnc(xfer, ToxPRPL_Purple_onTransferCompleted);

    return xfer;
}

/*
 * LibPurple pre-file-send callback to determine whether or not a file can be sent
 * to a given user
 */
gboolean ToxPRPL_Purple_canReceiveFileCheck(PurpleConnection* gc, const char* who) {
    purple_debug_info("toxprpl", "can_receive_file\n");

    toxprpl_return_val_if_fail(gc != NULL, FALSE);
    toxprpl_return_val_if_fail(who != NULL, FALSE);

    toxprpl_plugin_data* plugin = purple_connection_get_protocol_data(gc);
    toxprpl_return_val_if_fail(plugin != NULL && plugin->tox != NULL, FALSE);

    PurpleAccount* account = purple_connection_get_account(gc);
    toxprpl_return_val_if_fail(account != NULL, FALSE);

    PurpleBuddy* buddy = purple_find_buddy(account, who);
    toxprpl_return_val_if_fail(buddy != NULL, FALSE);

    toxprpl_buddy_data* buddy_data = purple_buddy_get_protocol_data(buddy);
    toxprpl_return_val_if_fail(buddy_data != NULL, FALSE);

    return tox_get_friend_connection_status(plugin->tox,
                                            buddy_data->tox_friendlist_number) == 1;
}

/*
 * LibPurple callback invoked when the frontend wishes to send a file to a user
 */
void ToxPRPL_Purple_sendFile(PurpleConnection* gc, const char* who, const char* filename) {
    purple_debug_info("toxprpl", "send_file\n");

    toxprpl_return_if_fail(gc != NULL);
    toxprpl_return_if_fail(who != NULL);

    PurpleXfer* xfer = ToxPRPL_newXfer(gc, who);
    toxprpl_return_if_fail(xfer != NULL);

    if (filename != NULL) {
        purple_debug_info("toxprpl", "filename != NULL\n");
        purple_xfer_request_accepted(xfer, filename);
    }
    else {
        purple_debug_info("toxprpl", "filename == NULL\n");
        purple_xfer_request(xfer);
    }
}

