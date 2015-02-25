#pragma once
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