/*
 * Contains definitions for account management
 */

/*
 * Defined in ``purple/account.c''
 */

void ToxPRPL_exportUser(PurpleConnection*, const char*);
void ToxPRPL_importUser(PurpleAccount*, const char*);
void ToxPRPL_showExportDialog(PurplePluginAction*);
gboolean ToxPRPL_saveAccount(PurpleAccount* account, Tox* tox);
void ToxPRPL_showIDNumberDialog(PurplePluginAction*);
void ToxPRPL_showSitNicknameDialog(PurplePluginAction*);
GList* ToxPRPL_Purple_getAccountActions(PurplePlugin*, gpointer);
void ToxPRPL_Purple_onSetNickname(PurpleConnection*, const char*);
void ToxPRPL_Purple_onSetStatus(PurpleAccount*, PurpleStatus*);

