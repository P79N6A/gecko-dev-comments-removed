




































const Cc = Components.classes;
const Ci = Components.interfaces;

const nsIBLS = Ci.nsIBlocklistService;


function get_test_plugintag() {
  var host = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
  var tags = host.getPluginTags();
  for (var i = 0; i < tags.length; i++) {
    if (tags[i].name == "Test Plug-in")
      return tags[i];
  }
  return null;
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  var source = do_get_file("data/test_bug514327_2.xml");
  source.copyTo(gProfD, "blocklist.xml");

  var blocklist = Cc["@mozilla.org/extensions/blocklist;1"].getService(nsIBLS);
  var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

  var plugin = get_test_plugintag();
  if (!plugin)
    do_throw("Plugin tag not found");

  
  do_check_true(blocklist.getPluginBlocklistState(plugin, "1", "1.9") == nsIBLS.STATE_OUTDATED);

  
  do_check_true(prefs.getBoolPref("plugins.update.notifyUser"));
}
