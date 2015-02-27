#pragma once

/*
 * From ``common/xfers.c''
 */

PurpleXfer* ToxPRPL_findXfer(PurpleConnection*, int, uint8_t);

PurpleXfer* ToxPRPL_Purple_onTransferReceive(PurpleConnection*, const char*, int, int, const goffset, const char*);

PurpleXfer* ToxPRPL_newXfer(PurpleConnection*, const gchar*);

/*
 * From ``purple/xfers.c''
 */

void ToxPRPL_Purple_prepareXfer(PurpleXfer*);

void ToxPRPL_Purple_startXfer(PurpleXfer*);

gssize ToxPRPL_Purple_writeXfer(const guchar*, size_t, PurpleXfer*);

gssize ToxPRPL_purpleDummyReadXfer(guchar**, PurpleXfer*);

void ToxPRPL_Purple_cancelOutgoingXfer(PurpleXfer*);

void ToxPRPL_Purple_onTransferCompleted(PurpleXfer*);

/*
 * LibPurple file transfer backend
 * - toxprpl_can_receive_file
 * - toxprpl_send_file
 * - toxprpl_new_xfer
 */
gboolean ToxPRPL_Purple_canReceiveFileCheck(PurpleConnection*, const char*);

void ToxPRPL_Purple_sendFile(PurpleConnection*, const char*, const char*);

PurpleXfer* ToxPRPL_newXfer(PurpleConnection*, const gchar*);

/*
 * From ``tox/xfers.c''
 */

/*
 * Tox file transfer callbacks
 * - on_file_control
 * - on_file_end_request
 * - on_file_data
 */
void ToxPRPL_Tox_onFileControl(Tox*, int32_t, uint8_t, uint8_t, uint8_t, const uint8_t*, uint16_t, void*);

void ToxPRPL_Tox_onFileRequest(Tox*, int32_t, uint8_t, uint64_t, const uint8_t*, uint16_t, void*);

void ToxPRPL_Tox_onFileDataReceive(Tox*, int32_t, uint8_t, const uint8_t*, uint16_t, void*);

