#pragma once

/*
 * From ``common/xfers.c''
 */

PurpleXfer* toxprpl_find_xfer(PurpleConnection*, int, uint8_t);

PurpleXfer* toxprpl_new_xfer_receive(PurpleConnection*, const char*, int, int, const goffset, const char*);

PurpleXfer* toxprpl_new_xfer(PurpleConnection*, const gchar*);

/*
 * From ``purple/xfers.c''
 */

void toxprpl_xfer_init(PurpleXfer*);

void toxprpl_xfer_start(PurpleXfer*);

gssize toxprpl_xfer_write(const guchar*, size_t, PurpleXfer*);

gssize toxprpl_xfer_read(guchar**, PurpleXfer*);

void toxprpl_xfer_cancel_send(PurpleXfer*);

void toxprpl_xfer_end(PurpleXfer*);

/*
 * LibPurple file transfer backend
 * - toxprpl_can_receive_file
 * - toxprpl_send_file
 * - toxprpl_new_xfer
 */
gboolean toxprpl_can_receive_file(PurpleConnection*, const char*);

void toxprpl_send_file(PurpleConnection*, const char*, const char*);

PurpleXfer* toxprpl_new_xfer(PurpleConnection*, const gchar*);

/*
 * From ``tox/xfers.c''
 */

/*
 * Tox file transfer callbacks
 * - on_file_control
 * - on_file_end_request
 * - on_file_data
 */
void on_file_control(Tox*, int32_t, uint8_t, uint8_t, uint8_t, const uint8_t*, uint16_t, void*);

void on_file_send_request(Tox*, int32_t, uint8_t, uint64_t, const uint8_t*, uint16_t, void*);

void on_file_data(Tox*, int32_t, uint8_t, const uint8_t*, uint16_t, void*);

