Components.utils.import("resource://gre/modules/Services.jsm");

function run_test() {
  let pluginDefaultState = Services.prefs.getIntPref("plugin.default.state");
  
  do_check_neq(pluginDefaultState, Ci.nsIPluginTag.STATE_DISABLED);
  let nonDefaultState = (pluginDefaultState != Ci.nsIPluginTag.STATE_ENABLED ?
                         Ci.nsIPluginTag.STATE_ENABLED :
                         Ci.nsIPluginTag.STATE_CLICKTOPLAY);
  let ph = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
  let testPlugin = get_test_plugintag();
  
  do_check_eq(testPlugin.enabledState, pluginDefaultState);

  let secondTestPlugin = get_test_plugintag("Second Test Plug-in");
  
  secondTestPlugin.enabledState = Ci.nsIPluginTag.STATE_DISABLED;
  
  Services.prefs.setIntPref("plugin.default.state", nonDefaultState);
  
  do_check_eq(testPlugin.enabledState, nonDefaultState);
  
  do_check_eq(secondTestPlugin.enabledState, Ci.nsIPluginTag.STATE_DISABLED);

  
  testPlugin.enabledState = pluginDefaultState;
  secondTestPlugin.enabledState = pluginDefaultState;
  Services.prefs.clearUserPref("plugin.default.state");
  Services.prefs.clearUserPref("plugin.importedState");
}
