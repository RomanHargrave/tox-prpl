/*
 * ToxPRPL plugin preferences.
 * This part of the LibPurple API does not seem to have much decent documentation.
 * So there are no guarantees that this conforms to whatever the best practices may be
 */

#include <toxprpl.h>
#include <toxprpl/preferences.h>

/*
 * Preference frame setup
 */

PluginPrefFrame* ToxPRPL_PluginPrefFrame;

static PurplePluginPrefFrame* ToxPRPL_Purple_getPrefFrame(PurplePlugin* plugin) {
        return ToxPRPL_PluginPrefFrame;
}

PurplePluginUiInfo ToxPRPL_PreferencesUI = { ToxPRPL_Purple_getPrefFrame };

/*
 * Preference setup
 */

void ToxPRPL_Preferences_init(void) {

        ToxPRPL_PluginPrefFrame = plugin_pref_frame_new();

}

void ToxPRPL_Preferences_cleanup(void) {

        if(ToxPRPL_PluginPrefFrame) {
                purple_plugin_pref_frame_destroy(ToxPRPL_PluginPrefFrame);
        }
}