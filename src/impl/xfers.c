/*
 * Contains all protocol file-transfer glue for libpurple
 *
 * @author Sergey 'Jin' Bostandzhyan <jin at mediatomb dot cc>
 * @author Roman Hargrave <roman@hargrave.info>
 */

#include <toxprpl.h>
#include <toxprpl/xfers.h>

//TODO create an inverted table to speed this up
PurpleXfer* toxprpl_find_xfer(PurpleConnection* gc, int friendnumber, uint8_t filenumber) {
    PurpleAccount* account = purple_connection_get_account(gc);
    toxprpl_return_val_if_fail(account != NULL, NULL);
    GList* xfers = purple_xfers_get_all();
    toxprpl_return_val_if_fail(xfers != NULL, NULL);

    while (xfers != NULL && xfers->data != NULL) {
        PurpleXfer* xfer = xfers->data;
        toxprpl_xfer_data* xfer_data = xfer->data;
        if (xfer_data != NULL &&
            xfer_data->friendnumber == friendnumber &&
            xfer_data->filenumber == filenumber) {
            return xfer;
        }
        xfers = g_list_next(xfers);
    }
    return NULL;
}

/*
 * Purple callback to initiate a file transfer to a given user.
 * Also called by toxprpl_send_file when seeking to initiate a file transfer
 */
PurpleXfer* toxprpl_new_xfer(PurpleConnection* gc, const gchar* who) {
    purple_debug_info("toxprpl", "new_xfer\n");

    toxprpl_return_val_if_fail(gc != NULL, NULL);
    toxprpl_return_val_if_fail(who != NULL, NULL);

    PurpleAccount* account = purple_connection_get_account(gc);
    toxprpl_return_val_if_fail(account != NULL, NULL);

    PurpleXfer* xfer = purple_xfer_new(account, PURPLE_XFER_SEND, who);
    toxprpl_return_val_if_fail(xfer != NULL, NULL);

    toxprpl_xfer_data* xfer_data = g_new0(toxprpl_xfer_data, 1);
    toxprpl_return_val_if_fail(xfer_data != NULL, NULL);

    xfer->data = xfer_data;

    purple_xfer_set_init_fnc(xfer, toxprpl_xfer_init);
    purple_xfer_set_start_fnc(xfer, toxprpl_xfer_start);
    purple_xfer_set_write_fnc(xfer, toxprpl_xfer_write);
    purple_xfer_set_read_fnc(xfer, toxprpl_xfer_read);
    purple_xfer_set_cancel_send_fnc(xfer, toxprpl_xfer_cancel_send);
    purple_xfer_set_end_fnc(xfer, toxprpl_xfer_end);

    return xfer;
}

