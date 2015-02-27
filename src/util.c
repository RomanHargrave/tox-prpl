/*
 * Utility functions
 */

#include <toxprpl.h>
#include <string.h>

// Base Conversion (2 -> 16, 16 -> 2) ----------------------------------------------------------------------------------

const char* g_HEX_CHARS = "0123456789abcdef";

char* ToxPRPL_binToHexString(const unsigned char* data, const size_t len) {
    unsigned char* chars;
    unsigned char hi, lo;
    size_t i;
    char* buf = malloc((len * 2) + 1);
    char* p = buf;
    chars = (unsigned char*) data;
    for (i = 0; i < len; i++) {
        unsigned char c = chars[i];
        hi = c >> 4;
        lo = c & 0xF;
        *p = g_HEX_CHARS[hi];
        p++;
        *p = g_HEX_CHARS[lo];
        p++;
    }
    buf[len * 2] = '\0';
    return buf;
}

unsigned char* ToxPRPL_hexStringToBin(const char* s) {
    size_t len = strlen(s);
    unsigned char* buf = malloc(len / 2);
    unsigned char* p = buf;

    size_t i;
    for (i = 0; i < len; i += 2) {
        const char* chi = strchr(g_HEX_CHARS, g_ascii_tolower(s[i]));
        const char* clo = strchr(g_HEX_CHARS, g_ascii_tolower(s[i + 1]));
        int hi, lo;
        if (chi) {
            hi = chi - g_HEX_CHARS;
        }
        else {
            hi = 0;
        }

        if (clo) {
            lo = clo - g_HEX_CHARS;
        }
        else {
            lo = 0;
        }

        unsigned char ch = (unsigned char) (hi << 4 | lo);
        *p = ch;
        p++;
    }
    return buf;
}

// End Base Conversion -------------------------------------------------------------------------------------------------

// Tox status type abstraction -----------------------------------------------------------------------------------------
const toxprpl_status ToxPRPL_ToxStatuses[] = {
        {
                PURPLE_STATUS_AVAILABLE, TOXPRPL_STATUS_ONLINE,
                "tox_online", _("Online")
        },
        {
                PURPLE_STATUS_AWAY, TOXPRPL_STATUS_AWAY,
                "tox_away", _("Away")
        },
        {
                PURPLE_STATUS_UNAVAILABLE, TOXPRPL_STATUS_BUSY,
                "tox_busy", _("Busy")
        },
        {
                PURPLE_STATUS_OFFLINE, TOXPRPL_STATUS_OFFLINE,
                "tox_offline", _("Offline")
        }
};

// stay independent from the lib
int ToxPRPL_getStatusTypeIndex(Tox* tox, int fnum, TOX_USERSTATUS status) {
    switch (status) {
        case TOX_USERSTATUS_AWAY:
            return TOXPRPL_STATUS_AWAY;
        case TOX_USERSTATUS_BUSY:
            return TOXPRPL_STATUS_BUSY;
        case TOX_USERSTATUS_NONE:
        case TOX_USERSTATUS_INVALID:
        default:
            if (fnum != -1) {
                if (tox_get_friend_connection_status(tox, fnum) == 1) {
                    return TOXPRPL_STATUS_ONLINE;
                }
            }
    }
    return TOXPRPL_STATUS_OFFLINE;
}

TOX_USERSTATUS ToxPRPL_getStatusTypeById(const char* status_id) {
    int i;
    for (i = 0; i < TOXPRPL_MAX_STATUS; i++) {
        if (strcmp(ToxPRPL_ToxStatuses[i].id, status_id) == 0) {
            return ToxPRPL_ToxStatuses[i].tox_status;
        }
    }
    return TOX_USERSTATUS_INVALID;
}

// End status type abstraction -----------------------------------------------------------------------------------------

// Tox ID helpers ------------------------------------------------------------------------------------------------------

/*
 * Returns a Base 16 string representation of the given client ID
 */
gchar* ToxPRPL_toxClientIdToString(const uint8_t* bin_id) {
    return ToxPRPL_binToHexString(bin_id, TOX_CLIENT_ID_SIZE);
}

/*
 * Returns a Base 16 string representation of the given friend/user ID
 */
gchar* ToxPRPL_toxFriendIdToString(uint8_t* bin_id) {
    return ToxPRPL_binToHexString(bin_id, TOX_FRIEND_ADDRESS_SIZE);
}

// End ID helpers ------------------------------------------------------------------------------------------------------