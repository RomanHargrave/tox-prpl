#pragma once
#include <toxprpl.h>

int ToxPRPL_Purple_addFriend(Tox*, PurpleConnection*, const char*, gboolean, const char*);

void ToxPRPL_Purple_getBuddyInfo(gpointer, gpointer);

void ToxPRPL_Purple_removeBuddy(PurpleConnection* gc, PurpleBuddy* buddy, PurpleGroup* group);

void ToxPRPL_Purple_addBuddy(PurpleConnection*, PurpleBuddy*, PurpleGroup*, const char*);

void ToxPRPL_Action_acceptFriendRequest(toxprpl_accept_friend_data*);

void ToxPRPL_Action_rejectFriendRequest(toxprpl_accept_friend_data*);

const char* ToxPRPL_Purple_getListIconForUser(PurpleAccount*, PurpleBuddy*);


