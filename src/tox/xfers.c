#include <toxprpl.h>
#include <toxprpl/xfers.h>

/*
 * Tox file transfer progress callback
 */
void on_file_control(Tox* tox, int32_t friendnumber, uint8_t receive_send, uint8_t filenumber,
                     uint8_t control_type, const uint8_t* data, uint16_t length, void* userdata) {
    purple_debug_info("toxprpl", "file control: %i (%s) %i\n", friendnumber,
                      receive_send == 0 ? "rx" : "tx", filenumber);
    PurpleConnection* gc = userdata;
    toxprpl_return_if_fail(gc != NULL);

    PurpleXfer* xfer = toxprpl_find_xfer(gc, friendnumber, filenumber);
    toxprpl_return_if_fail(xfer != NULL);

    if (receive_send == 0) //receiving
    {
        switch (control_type) {
            case TOX_FILECONTROL_FINISHED:
                purple_xfer_set_completed(xfer, TRUE);
                purple_xfer_end(xfer);
                break;
            case TOX_FILECONTROL_KILL:
                purple_xfer_cancel_remote(xfer);
                break;
        }
    }
    else //sending
    {
        switch (control_type) {
            case TOX_FILECONTROL_ACCEPT:
                purple_xfer_start(xfer, -1, NULL, 0);
                break;
            case TOX_FILECONTROL_KILL:
                purple_xfer_cancel_remote(xfer);
                break;
        }
    }
}

/*
 * Tox file send request callback
 */
void on_file_send_request(Tox* tox, int32_t friendnumber,
                          uint8_t filenumber,
                          uint64_t filesize, const uint8_t* filename,
                          uint16_t filename_length, void* userdata) {
    purple_debug_info("toxprpl", "file_send_request: %i %i\n", friendnumber,
                      filenumber);
    PurpleConnection* gc = userdata;

    toxprpl_return_if_fail(gc != NULL);
    toxprpl_return_if_fail(filename != NULL);
    toxprpl_return_if_fail(tox != NULL);

    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    if (tox_get_client_id(tox, friendnumber, client_id) < 0) {
        purple_debug_info("toxprpl", "Could not get id of friend %d\n",
                          friendnumber);
        return;
    }
    gchar* buddy_key = toxprpl_tox_bin_id_to_string(client_id);

    PurpleXfer* xfer = toxprpl_new_xfer_receive(gc, buddy_key, friendnumber,
                                                filenumber, filesize, (const char*) filename);
    if (xfer == NULL) {
        purple_debug_warning("toxprpl", "could not create xfer\n");
        g_free(buddy_key);
        return;
    }
    toxprpl_return_if_fail(xfer != NULL);
    purple_xfer_request(xfer);
    g_free(buddy_key);
}

/*
 * Tox file transfer data callback
 */
void on_file_data(Tox* tox, int32_t friendnumber, uint8_t filenumber,
                  const uint8_t* data, uint16_t length, void* userdata) {
    PurpleConnection* gc = userdata;

    toxprpl_return_if_fail(gc != NULL);

    PurpleXfer* xfer = toxprpl_find_xfer(gc, friendnumber, filenumber);
    toxprpl_return_if_fail(xfer != NULL);
    toxprpl_return_if_fail(xfer->dest_fp != NULL);

    size_t written = fwrite(data, sizeof(uint8_t), length, xfer->dest_fp);
    if (written != length) {
        purple_debug_warning("toxprpl", "could not write whole buffer\n");
        purple_xfer_cancel_local(xfer);
        return;
    }

    if (purple_xfer_get_size(xfer) > 0) {
        xfer->bytes_remaining -= written;
        xfer->bytes_sent += written;
        purple_xfer_update_progress(xfer);
    }
}

