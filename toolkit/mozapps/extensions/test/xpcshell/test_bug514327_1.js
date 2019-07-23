




































const Cc = Components.classes;
const Ci = Components.interfaces;

const nsIBLS = Ci.nsIBlocklistService;

var PLUGINS = [{
  
  name: "test_bug514327_1",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_bug514327_2",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_bug514327_3",
  version: "5",
  disabled: false,
  blocklisted: false
},
{
  
  name: "test_bug514327_4",
  version: "5",
  disabled: false,
  blocklisted: false,
  outdated: false
}];


function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  var source = do_get_file("data/test_bug514327_1.xml");
  source.copyTo(gProfD, "blocklist.xml");

  var blocklist = Cc["@mozilla.org/extensions/blocklist;1"].getService(nsIBLS);

  
  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[0], "1", "1.9") == nsIBLS.STATE_BLOCKED);

  
  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[1], "1", "1.9") == nsIBLS.STATE_OUTDATED);

  
  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[2], "1", "1.9") == nsIBLS.STATE_OUTDATED);

  
  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[3], "1", "1.9") == nsIBLS.STATE_NOT_BLOCKED);
}
