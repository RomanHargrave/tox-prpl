/*
 * Utility functions
 */

#include <toxprpl.h>
#include <string.h>

const char* g_HEX_CHARS = "0123456789abcdef";

const toxprpl_status toxprpl_statuses[] =
        {
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

/*
 * Kitchen Sink
 */

char* toxprpl_data_to_hex_string(const unsigned char* data,
                                        const size_t len) {
    unsigned char* chars;
    unsigned char hi, lo;
    size_t i;
    char* buf = malloc((len * 2) + 1);
    char* p = buf;
    chars = (unsigned char*) data;
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

unsigned char* toxprpl_hex_string_to_data(const char* s) {
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

// stay independent from the lib
int toxprpl_get_status_index(Tox* tox, int fnum, TOX_USERSTATUS status) {
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

TOX_USERSTATUS toxprpl_get_tox_status_from_id(const char* status_id) {
    int i;
    for (i = 0; i < TOXPRPL_MAX_STATUS; i++) {
        if (strcmp(toxprpl_statuses[i].id, status_id) == 0) {
            return toxprpl_statuses[i].tox_status;
        }
    }
    return TOX_USERSTATUS_INVALID;
}

/*
 * Tox helpers
 */

gchar* toxprpl_tox_bin_id_to_string(const uint8_t* bin_id) {
    return toxprpl_data_to_hex_string(bin_id, TOX_CLIENT_ID_SIZE);
}

gchar* toxprpl_tox_friend_id_to_string(uint8_t* bin_id) {
    return toxprpl_data_to_hex_string(bin_id, TOX_FRIEND_ADDRESS_SIZE);
}