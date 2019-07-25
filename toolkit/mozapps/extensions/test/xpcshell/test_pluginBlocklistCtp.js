



const nsIBLS = Components.interfaces.nsIBlocklistService;

var PLUGINS = [{
  
  name: "test_plugin_0",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_plugin_1",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_plugin_2",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_plugin_3",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_plugin_4",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_plugin_5",
  version: "5",
  disabled: false,
  blocklisted: false
}];


function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  
  var blocklistFile = gProfD.clone();
  blocklistFile.append("blocklist.xml");
  if (blocklistFile.exists())
    blocklistFile.remove(false);
  var source = do_get_file("data/test_pluginBlocklistCtp.xml");
  source.copyTo(gProfD, "blocklist.xml");

  var blocklist = Components.classes["@mozilla.org/extensions/blocklist;1"]
                            .getService(nsIBLS);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[0], "1", "1.9") == nsIBLS.STATE_OUTDATED);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[1], "1", "1.9") == nsIBLS.STATE_VULNERABLE_UPDATE_AVAILABLE);
  
  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[2], "1", "1.9") == nsIBLS.STATE_VULNERABLE_NO_UPDATE);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[3], "1", "1.9") == nsIBLS.STATE_BLOCKED);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[4], "1", "1.9") == nsIBLS.STATE_SOFTBLOCKED);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[5], "1", "1.9") == nsIBLS.STATE_NOT_BLOCKED);
}
