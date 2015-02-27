#pragma once
#include <toxprpl.h>

int toxprpl_tox_add_friend(Tox*, PurpleConnection*, const char*, gboolean, const char*);

void toxprpl_query_buddy_info(gpointer, gpointer);

void toxprpl_remove_buddy(PurpleConnection* gc, PurpleBuddy* buddy, PurpleGroup* group);

void toxprpl_add_buddy(PurpleConnection*, PurpleBuddy*, PurpleGroup*, const char*);

void toxprpl_add_to_buddylist(toxprpl_accept_friend_data*);

void toxprpl_do_not_add_to_buddylist(toxprpl_accept_friend_data*);

const char* toxprpl_list_icon(PurpleAccount*, PurpleBuddy*);


