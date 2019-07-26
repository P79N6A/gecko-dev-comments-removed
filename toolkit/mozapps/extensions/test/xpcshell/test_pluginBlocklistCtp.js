



const nsIBLS = Components.interfaces.nsIBlocklistService;
Components.utils.import("resource://testing-common/httpd.js");

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

var gBlocklistService = null;
var gNotifier = null;
var gNextTest = null;
var gServer = null;
var gPluginHost = null;

function test_basic() {
  var blocklist = Components.classes["@mozilla.org/extensions/blocklist;1"].getService(nsIBLS);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[0], "1", "1.9") == nsIBLS.STATE_OUTDATED);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[1], "1", "1.9") == nsIBLS.STATE_VULNERABLE_UPDATE_AVAILABLE);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[2], "1", "1.9") == nsIBLS.STATE_VULNERABLE_NO_UPDATE);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[3], "1", "1.9") == nsIBLS.STATE_BLOCKED);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[4], "1", "1.9") == nsIBLS.STATE_SOFTBLOCKED);

  do_check_true(blocklist.getPluginBlocklistState(PLUGINS[5], "1", "1.9") == nsIBLS.STATE_NOT_BLOCKED);

  gNextTest = test_is_not_clicktoplay;
  do_execute_soon(gNextTest);
}

function get_test_plugin() {
  var pluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);
  for (var plugin of pluginHost.getPluginTags()) {
    if (plugin.name == "Test Plug-in")
      return plugin;
  }
  do_check_true(false);
  return null;
}



function test_is_not_clicktoplay() {
  var plugin = get_test_plugin();
  var blocklistState = gBlocklistService.getPluginBlocklistState(plugin, "1", "1.9");
  do_check_neq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_UPDATE_AVAILABLE);
  do_check_neq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_NO_UPDATE);
  do_check_false(gPluginHost.isPluginClickToPlayForType("application/x-test"));

  Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:4444/data/test_pluginBlocklistCtpUndo.xml");
  gNextTest = test_is_clicktoplay;
  gNotifier.notify(null);
}



function test_is_clicktoplay() {
  var plugin = get_test_plugin();
  var blocklistState = gBlocklistService.getPluginBlocklistState(plugin, "1", "1.9");
  do_check_eq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_NO_UPDATE);
  do_check_true(gPluginHost.isPluginClickToPlayForType("application/x-test"));

  Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:4444/data/test_pluginBlocklistCtp.xml");
  gNextTest = test_is_not_clicktoplay2;
  gNotifier.notify(null);
}



function test_is_not_clicktoplay2() {
  var plugin = get_test_plugin();
  var blocklistState = gBlocklistService.getPluginBlocklistState(plugin, "1", "1.9");
  do_check_neq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_UPDATE_AVAILABLE);
  do_check_neq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_NO_UPDATE);
  do_check_false(gPluginHost.isPluginClickToPlayForType("application/x-test"));

  Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:4444/data/test_pluginBlocklistCtpUndo.xml");
  gNextTest = test_disable_blocklist;
  gNotifier.notify(null);
}



function test_disable_blocklist() {
  var plugin = get_test_plugin();
  var blocklistState = gBlocklistService.getPluginBlocklistState(plugin, "1", "1.9");
  do_check_eq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_NO_UPDATE);
  do_check_true(gPluginHost.isPluginClickToPlayForType("application/x-test"));

  gNextTest = null;
  Services.prefs.setBoolPref("extensions.blocklist.enabled", false);
  blocklistState = gBlocklistService.getPluginBlocklistState(plugin, "1", "1.9");
  do_check_neq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_NO_UPDATE);
  do_check_neq(blocklistState, Components.interfaces.nsIBlocklistService.STATE_VULNERABLE_UPDATE_AVAILABLE);
  do_check_false(gPluginHost.isPluginClickToPlayForType("application/x-test"));

  
  
  Services.prefs.setBoolPref("plugins.click_to_play", true);
  let previousEnabledState = plugin.enabledState;
  plugin.enabledState = Components.interfaces.nsIPluginTag.STATE_CLICKTOPLAY;
  do_check_true(gPluginHost.isPluginClickToPlayForType("application/x-test"));
  
  plugin.enabledState = previousEnabledState;

  gServer.stop(do_test_finished);
}


function observer() {
  if (gNextTest)
    do_execute_soon(gNextTest);
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  gServer = new HttpServer();
  gServer.registerDirectory("/data/", do_get_file("data"));
  gServer.start(4444);

  Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:4444/data/test_pluginBlocklistCtp.xml");
  startupManager();

  gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);
  gBlocklistService = Components.classes["@mozilla.org/extensions/blocklist;1"].getService(Components.interfaces.nsIBlocklistService);
  gNotifier = Components.classes["@mozilla.org/extensions/blocklist;1"].getService(Components.interfaces.nsITimerCallback);
  Services.obs.addObserver(observer, "blocklist-updated", false);

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("extensions.blocklist.url");
    Services.prefs.clearUserPref("extensions.blocklist.enabled");
    Services.prefs.clearUserPref("plugins.click_to_play");
    Services.obs.removeObserver(observer, "blocklist-updated");
  });

  gNextTest = test_basic;
  do_test_pending();
  gNotifier.notify(null);
}
