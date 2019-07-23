




































do_load_httpd_js();


const nsIBLS = Ci.nsIBlocklistService;
const URI_EXTENSION_BLOCKLIST_DIALOG = "chrome://mozapps/content/extensions/blocklist.xul";

var gBlocklist = null;
var gPrefs = null;
var gTestserver = null;

var gNextTestPart = null;


var PLUGINS = [{
  
  name: "test_bug514327_outdated",
  version: "5",
  disabled: false,
  blocklisted: false
}, {
  
  name: "test_bug514327_1",
  version: "5",
  disabled: false,
  blocklisted: false
}, {
  
  name: "test_bug514327_2",
  version: "5",
  disabled: false,
  blocklisted: false
} ];



var PluginHost = {
  getPluginTags: function(countRef) {
    countRef.value = PLUGINS.length;
    return PLUGINS;
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIPluginHost)
     || iid.equals(Ci.nsISupports))
      return this;
  
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

var PluginHostFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return PluginHost.QueryInterface(iid);
  }
};



var WindowWatcher = {
  openWindow: function(parent, url, name, features, arguments) {
    
    do_check_eq(url, URI_EXTENSION_BLOCKLIST_DIALOG);
    
    do_check_eq(arguments.wrappedJSObject.list.length, 1);
    
    var item = arguments.wrappedJSObject.list[0];
    do_check_true(item.item instanceof Ci.nsIPluginTag);
    do_check_neq(item.name, "test_bug514327_outdated");

    
    do_timeout(0, gNextTestPart);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIWindowWatcher)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}

var WindowWatcherFactory = {
  createInstance: function createInstance(outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return WindowWatcher.QueryInterface(iid);
  }
};

var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{721c3e73-969e-474b-a6dc-059fd288c428}"),
                          "Fake Plugin Host",
                          "@mozilla.org/plugin/host;1", PluginHostFactory);
registrar.registerFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                          "Fake Window Watcher",
                          "@mozilla.org/embedcomp/window-watcher;1", WindowWatcherFactory);


function do_update_blocklist(aDatafile, aNextPart) {
  gNextTestPart = aNextPart;
  
  gPrefs.setCharPref("extensions.blocklist.url", "http://localhost:4444/data/" + aDatafile);
  gBlocklist.QueryInterface(Ci.nsITimerCallback).notify(null);
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  
  gTestserver = new nsHttpServer();
  gTestserver.registerDirectory("/data/", do_get_file("data"));
  gTestserver.start(4444);


  
  var source = do_get_file("data/test_bug514327_3_empty.xml");
  source.copyTo(gProfD, "blocklist.xml");
  
  gPrefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch2);
  gBlocklist = Cc["@mozilla.org/extensions/blocklist;1"].getService(nsIBLS);
  
  
  do_check_true(gBlocklist.getPluginBlocklistState(PLUGINS[0], "1", "1.9") == nsIBLS.STATE_NOT_BLOCKED);
  
  do_test_pending();

  
  do_update_blocklist("test_bug514327_3_outdated_1.xml", test_part_1);
}

function test_part_1() {
  
  do_check_true(gBlocklist.getPluginBlocklistState(PLUGINS[0], "1", "1.9") == nsIBLS.STATE_OUTDATED);
  
  do_check_true(gPrefs.getBoolPref("plugins.update.notifyUser"));
  
  
  gPrefs.setBoolPref("plugins.update.notifyUser", false);
  
  
  do_update_blocklist("test_bug514327_3_outdated_2.xml", test_part_2);
}

function test_part_2() {
  
  do_check_true(gBlocklist.getPluginBlocklistState(PLUGINS[0], "1", "1.9") == nsIBLS.STATE_OUTDATED);
  
  do_check_false(gPrefs.getBoolPref("plugins.update.notifyUser"));

  finish();
}

function finish() {
  gTestserver.stop(do_test_finished);
}
