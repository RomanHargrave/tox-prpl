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


