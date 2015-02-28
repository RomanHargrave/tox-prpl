#pragma once

#define TOXPRPL_ID "prpl-rhargrave-tox"

/*
 * Default server settings.
 */
#define DEFAULT_SERVER_KEY "951C88B7E75C867418ACDB5D273821372BB5BD652740BCDF623A4FA293E75D2F"
#define DEFAULT_SERVER_PORT 33445
#define DEFAULT_SERVER_IP   "192.254.75.98"

#define DEFAULT_NICKNAME    "ToxedPidgin"

#define MAX_ACCOUNT_DATA_SIZE   1*1024*1024

#define toxprpl_return_val_if_fail(expr, val)     \
    if (!(expr))                                 \
    {                                            \
        return (val);                            \
    }

#define toxprpl_return_if_fail(expr)             \
    if (!(expr))                                 \
    {                                            \
        return;                                  \
    }


#define _(msg) msg // might add gettext later

#define DEFAULT_REQUEST_MESSAGE _("Please allow me to add you as a friend!")

// TODO -> enum
#define TOXPRPL_MAX_STATUS          4
#define TOXPRPL_STATUS_ONLINE       0
#define TOXPRPL_STATUS_AWAY         1
#define TOXPRPL_STATUS_BUSY         2
#define TOXPRPL_STATUS_OFFLINE      3

/*
 * Support for inferior operating systems
 * Defines O_BINARY (inferior and redundant FD mode used by inferior operating systems)
 */
#ifndef O_BINARY
    #ifdef _O_BINARY
            #define O_BINARY _O_BINARY
        #else
            #define O_BINARY 0
    #endif
#endif

/*
 * Pidgin bits
 */
#include <account.h>
#include <accountopt.h>
#include <blist.h>
#include <cmds.h>
#include <conversation.h>
#include <connection.h>
#include <debug.h>
#include <notify.h>
#include <privacy.h>
#include <prpl.h>
#include <roomlist.h>
#include <request.h>
#include <status.h>
#include <util.h>
#include <version.h>

#include <tox/tox.h>

#include <toxprpl_data.h>

// util.c start --------------------------------------------------------------------------------------------------------

extern const ToxPRPL_Status ToxPRPL_ToxStatuses[];

/*
 * Kitchen sink
 */

char* ToxPRPL_binToHexString(const unsigned char*, const size_t);
unsigned char* ToxPRPL_hexStringToBin(const char*);
int ToxPRPL_getStatusTypeIndex(Tox*, int, TOX_USERSTATUS);
TOX_USERSTATUS ToxPRPL_getStatusTypeById(const char*);

/*
 * Tox helpers
 */

gchar* ToxPRPL_toxClientIdToString(const uint8_t*);
gchar* ToxPRPL_toxFriendIdToString(uint8_t*);

// util.c end ----------------------------------------------------------------------------------------------------------