/*
 * Contains definitions for account management
 */

/*
 * Defined in ``purple/account.c''
 */

void toxprpl_user_export(PurpleConnection*, const char*);
void toxprpl_user_import(PurpleAccount*, const char*);
void toxprpl_export_account_dialog(PurplePluginAction*);
gboolean toxprpl_save_account(PurpleAccount* account, Tox* tox);
void toxprpl_action_show_id_dialog(PurplePluginAction*);
void toxprpl_action_set_nick_dialog(PurplePluginAction*);
GList* toxprpl_account_actions(PurplePlugin*, gpointer);
void toxprpl_set_nick_action(PurpleConnection*, const char*);
void toxprpl_set_status(PurpleAccount*, PurpleStatus*);

