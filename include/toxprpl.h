#pragma once

#include <tox/tox.h>

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

#include <toxprpl_data.h>

/*
 * Switch option that determines whether or not IPv6 support should be enabled when bootstrapping Tox
 */
#define TOXPRPL_USE_IPV6 0

/*
 * Default server settings.
 * It may be wise to make this configurable by the end user
 */
#define TOXPRPL_ID "prpl-jin_eld-tox"
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

gchar* toxprpl_tox_bin_id_to_string(const uint8_t* bin_id);