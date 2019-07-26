



const Cc = Components.classes;
const Ci = Components.interfaces;

const nsIBLS = Ci.nsIBlocklistService;


function get_test_plugintag() {
  var host = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
  var tags = host.getPluginTags();
  for (let tag of tags) {
    if (tag.name == "Test Plug-in")
      return tag;
  }
  return null;
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  copyBlocklistToProfile(do_get_file("data/test_bug514327_2.xml"));

  var blocklist = Cc["@mozilla.org/extensions/blocklist;1"].getService(nsIBLS);
  var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

  var plugin = get_test_plugintag();
  if (!plugin)
    do_throw("Plugin tag not found");

  
  Services.obs.notifyObservers(null, "addon-blocklist-closed", null);
  do_execute_soon(function() {
    
    do_check_true(blocklist.getPluginBlocklistState(plugin, "1", "1.9") == nsIBLS.STATE_OUTDATED);

    
    do_check_true(prefs.getBoolPref("plugins.update.notifyUser"));
  });
}
