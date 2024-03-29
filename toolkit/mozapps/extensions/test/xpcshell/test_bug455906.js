




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const URI_EXTENSION_BLOCKLIST_DIALOG = "chrome://mozapps/content/extensions/blocklist.xul";

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://testing-common/MockRegistrar.jsm");
var gTestserver = new HttpServer();
gTestserver.start(-1);
gPort = gTestserver.identity.primaryPort;


mapFile("/data/bug455906_warn.xml", gTestserver);
mapFile("/data/bug455906_start.xml", gTestserver);
mapFile("/data/bug455906_block.xml", gTestserver);
mapFile("/data/bug455906_empty.xml", gTestserver);


const PREF_BLOCKLIST_ITEM_URL = "extensions.blocklist.itemURL";
Services.prefs.setCharPref(PREF_BLOCKLIST_ITEM_URL, "http://localhost:" + gPort + "/blocklist/%blockID%");

var ADDONS = [{
  
  id: "test_bug455906_1@tests.mozilla.org",
  name: "Bug 455906 Addon Test 1",
  version: "5",
  appVersion: "3"
}, {
  
  id: "test_bug455906_2@tests.mozilla.org",
  name: "Bug 455906 Addon Test 2",
  version: "5",
  appVersion: "3"
}, {
  
  id: "test_bug455906_3@tests.mozilla.org",
  name: "Bug 455906 Addon Test 3",
  version: "5",
  appVersion: "3"
}, {
  
  id: "test_bug455906_4@tests.mozilla.org",
  name: "Bug 455906 Addon Test 4",
  version: "5",
  appVersion: "3"
}, {
  
  id: "test_bug455906_5@tests.mozilla.org",
  name: "Bug 455906 Addon Test 5",
  version: "5",
  appVersion: "3"
}, {
  
  id: "test_bug455906_6@tests.mozilla.org",
  name: "Bug 455906 Addon Test 6",
  version: "5",
  appVersion: "3"
}, {
  
  id: "test_bug455906_7@tests.mozilla.org",
  name: "Bug 455906 Addon Test 7",
  version: "5",
  appVersion: "2"
}, {
  
  id: "dummy_bug455906_1@tests.mozilla.org",
  name: "Dummy Addon 1",
  version: "5",
  appVersion: "3"
}, {
  
  id: "dummy_bug455906_2@tests.mozilla.org",
  name: "Dummy Addon 2",
  version: "5",
  appVersion: "3"
}];

function MockPlugin(name, version, enabledState) {
  this.name = name;
  this.version = version;
  this.enabledState = enabledState;
}
Object.defineProperty(MockPlugin.prototype, "blocklisted", {
  get: function MockPlugin_getBlocklisted() {
    let bls = Cc["@mozilla.org/extensions/blocklist;1"].getService(Ci.nsIBlocklistService);
    return bls.getPluginBlocklistState(this) == bls.STATE_BLOCKED;
  }
});
Object.defineProperty(MockPlugin.prototype, "disabled", {
  get: function MockPlugin_getDisabled() {
    return this.enabledState == Ci.nsIPluginTag.STATE_DISABLED;
  }
});

var PLUGINS = [
  
  new MockPlugin("test_bug455906_1", "5", Ci.nsIPluginTag.STATE_DISABLED),
  
  new MockPlugin("test_bug455906_2", "5", Ci.nsIPluginTag.STATE_ENABLED),
  
  new MockPlugin("test_bug455906_3", "5", Ci.nsIPluginTag.STATE_ENABLED),
  
  new MockPlugin("test_bug455906_4", "5", Ci.nsIPluginTag.STATE_DISABLED),
  
  new MockPlugin("test_bug455906_5", "5", Ci.nsIPluginTag.STATE_ENABLED),
  
  new MockPlugin("test_bug455906_6", "5", Ci.nsIPluginTag.STATE_ENABLED)
];

var gNotificationCheck = null;
var gTestCheck = null;


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



var WindowWatcher = {
  openWindow: function(parent, url, name, features, windowArguments) {
    
    do_check_eq(url, URI_EXTENSION_BLOCKLIST_DIALOG);

    if (gNotificationCheck) {
      var args = windowArguments.wrappedJSObject;
      gNotificationCheck(args);
    }

    
    Services.obs.notifyObservers(null, "addon-blocklist-closed", null);

    
    do_timeout(0, gTestCheck);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIWindowWatcher)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}

MockRegistrar.register("@mozilla.org/plugin/host;1", PluginHost);
MockRegistrar.register("@mozilla.org/embedcomp/window-watcher;1", WindowWatcher);

function create_addon(addon) {
  var installrdf = "<?xml version=\"1.0\"?>\n" +
                   "\n" +
                   "<RDF xmlns=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n" +
                   "     xmlns:em=\"http://www.mozilla.org/2004/em-rdf#\">\n" +
                   "  <Description about=\"urn:mozilla:install-manifest\">\n" +
                   "    <em:id>" + addon.id + "</em:id>\n" +
                   "    <em:version>" + addon.version + "</em:version>\n" +
                   "    <em:targetApplication>\n" +
                   "      <Description>\n" +
                   "        <em:id>xpcshell@tests.mozilla.org</em:id>\n" +
                   "        <em:minVersion>" + addon.appVersion + "</em:minVersion>\n" +
                   "        <em:maxVersion>" + addon.appVersion + "</em:maxVersion>\n" +
                   "      </Description>\n" +
                   "    </em:targetApplication>\n" +
                   "    <em:name>" + addon.name + "</em:name>\n" +
                   "  </Description>\n" +
                   "</RDF>\n";
  var target = gProfD.clone();
  target.append("extensions");
  target.append(addon.id);
  target.append("install.rdf");
  target.create(target.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  var stream = Cc["@mozilla.org/network/file-output-stream;1"].
               createInstance(Ci.nsIFileOutputStream);
  stream.init(target,
              FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE | FileUtils.MODE_TRUNCATE,
              FileUtils.PERMS_FILE, 0);
  stream.write(installrdf, installrdf.length);
  stream.close();
}

function load_blocklist(file) {
  Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:" + gPort + "/data/" + file);
  var blocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                  getService(Ci.nsITimerCallback);
  blocklist.notify(null);
}

function check_addon_state(addon) {
  return addon.userDisabled + "," + addon.softDisabled + "," + addon.appDisabled;
}

function check_plugin_state(plugin) {
  return plugin.disabled + "," + plugin.blocklisted;
}

function create_blocklistURL(blockID){
  let url = Services.urlFormatter.formatURLPref(PREF_BLOCKLIST_ITEM_URL);
  url = url.replace(/%blockID%/g, blockID);
  return url;
}


function run_test() {
  
  dump("Setting up tests\n");
  
  
  for (let addon of ADDONS)
    create_addon(addon);

  
  
  copyBlocklistToProfile(do_get_file("data/bug455906_start.xml"));

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "3", "8");
  startupManager();

  do_test_pending();
  check_test_pt1();
}


function check_initial_state(callback) {
  AddonManager.getAddonsByIDs([a.id for each (a in ADDONS)], function(addons) {
    do_check_eq(check_addon_state(addons[0]), "true,false,false");
    do_check_eq(check_addon_state(addons[1]), "false,false,false");
    do_check_eq(check_addon_state(addons[2]), "false,false,false");
    do_check_eq(check_addon_state(addons[3]), "true,true,false");
    do_check_eq(check_addon_state(addons[4]), "false,false,false");
    do_check_eq(check_addon_state(addons[5]), "false,false,true");
    do_check_eq(check_addon_state(addons[6]), "false,false,true");
  
    do_check_eq(check_plugin_state(PLUGINS[0]), "true,false");
    do_check_eq(check_plugin_state(PLUGINS[1]), "false,false");
    do_check_eq(check_plugin_state(PLUGINS[2]), "false,false");
    do_check_eq(check_plugin_state(PLUGINS[3]), "true,false");
    do_check_eq(check_plugin_state(PLUGINS[4]), "false,false");
    do_check_eq(check_plugin_state(PLUGINS[5]), "false,true");

    callback();
  });
}


function check_test_pt1() {
  dump("Checking pt 1\n");

  AddonManager.getAddonsByIDs([a.id for each (a in ADDONS)], callback_soon(function(addons) {
    for (var i = 0; i < ADDONS.length; i++) {
      if (!addons[i])
        do_throw("Addon " + (i + 1) + " did not get installed correctly");
    }
  
    do_check_eq(check_addon_state(addons[0]), "false,false,false");
    do_check_eq(check_addon_state(addons[1]), "false,false,false");
    do_check_eq(check_addon_state(addons[2]), "false,false,false");
  
    
    do_check_eq(check_addon_state(addons[3]), "true,true,false");
    do_check_eq(check_addon_state(addons[4]), "true,true,false");
  
    
    do_check_eq(check_addon_state(addons[5]), "false,false,true");
    do_check_eq(check_addon_state(addons[6]), "false,false,true");
  
    
    
  
    
    addons[0].userDisabled = true;
    addons[4].userDisabled = false;

    restartManager();
    check_initial_state(function() {
      gNotificationCheck = check_notification_pt2;
      gTestCheck = check_test_pt2;
      load_blocklist("bug455906_warn.xml");
    });
  }));
}

function check_notification_pt2(args) {
  dump("Checking notification pt 2\n");
  do_check_eq(args.list.length, 4);

  for (let addon of args.list) {
    if (addon.item instanceof Ci.nsIPluginTag) {
      switch (addon.item.name) {
        case "test_bug455906_2":
          do_check_false(addon.blocked);
          break;
        case "test_bug455906_3":
          do_check_false(addon.blocked);
          addon.disable = true;
          break;
        default:
          do_throw("Unknown addon: " + addon.item.name);
      }
    }
    else {
      switch (addon.item.id) {
        case "test_bug455906_2@tests.mozilla.org":
          do_check_false(addon.blocked);
          break;
        case "test_bug455906_3@tests.mozilla.org":
          do_check_false(addon.blocked);
          addon.disable = true;
          break;
        default:
          do_throw("Unknown addon: " + addon.item.id);
      }
    }
  }
}

function check_test_pt2() {
  restartManager();
  dump("Checking results pt 2\n");

  AddonManager.getAddonsByIDs([a.id for each (a in ADDONS)], callback_soon(function(addons) {
    
    do_check_eq(check_addon_state(addons[2]), "true,true,false");
    do_check_eq(check_plugin_state(PLUGINS[2]), "true,false");

    
    do_check_eq(check_addon_state(addons[5]), "true,true,false");
    do_check_eq(check_plugin_state(PLUGINS[5]), "true,false");

    
    do_check_eq(check_addon_state(addons[0]), "true,false,false");
    do_check_eq(check_addon_state(addons[1]), "false,false,false");
    do_check_eq(check_addon_state(addons[3]), "true,true,false");
    do_check_eq(check_addon_state(addons[4]), "false,false,false");
    do_check_eq(check_addon_state(addons[6]), "false,false,true");
    do_check_eq(check_plugin_state(PLUGINS[0]), "true,false");
    do_check_eq(check_plugin_state(PLUGINS[1]), "false,false");
    do_check_eq(check_plugin_state(PLUGINS[3]), "true,false");
    do_check_eq(check_plugin_state(PLUGINS[4]), "false,false");

    
    addons[2].userDisabled = false;
    addons[5].userDisabled = false;
    PLUGINS[2].enabledState = Ci.nsIPluginTag.STATE_ENABLED;
    PLUGINS[5].enabledState = Ci.nsIPluginTag.STATE_ENABLED;
    restartManager();
    gNotificationCheck = null;
    gTestCheck = run_test_pt3;
    load_blocklist("bug455906_start.xml");
  }));
}

function run_test_pt3() {
  restartManager();
  check_initial_state(function() {
    gNotificationCheck = check_notification_pt3;
    gTestCheck = check_test_pt3;
    load_blocklist("bug455906_block.xml");
  });
}

function check_notification_pt3(args) {
  dump("Checking notification pt 3\n");
  do_check_eq(args.list.length, 6);

  for (let addon of args.list) {
    if (addon.item instanceof Ci.nsIPluginTag) {
      switch (addon.item.name) {
        case "test_bug455906_2":
          do_check_true(addon.blocked);
          break;
        case "test_bug455906_3":
          do_check_true(addon.blocked);
          break;
        case "test_bug455906_5":
          do_check_true(addon.blocked);
          break;
        default:
          do_throw("Unknown addon: " + addon.item.name);
      }
    }
    else {
      switch (addon.item.id) {
        case "test_bug455906_2@tests.mozilla.org":
          do_check_true(addon.blocked);
          break;
        case "test_bug455906_3@tests.mozilla.org":
          do_check_true(addon.blocked);
          break;
        case "test_bug455906_5@tests.mozilla.org":
          do_check_true(addon.blocked);
          break;
        default:
          do_throw("Unknown addon: " + addon.item.id);
      }
    }
  }
}

function check_test_pt3() {
  restartManager();
  dump("Checking results pt 3\n");

  let blocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                  getService(Ci.nsIBlocklistService);

  AddonManager.getAddonsByIDs([a.id for each (a in ADDONS)], function(addons) {
    
    do_check_eq(check_addon_state(addons[0]), "true,false,true");
    do_check_eq(check_addon_state(addons[1]), "false,false,true");
    do_check_eq(check_addon_state(addons[2]), "false,false,true");
    do_check_eq(check_addon_state(addons[4]), "false,false,true");
    do_check_eq(check_plugin_state(PLUGINS[0]), "true,true");
    do_check_eq(check_plugin_state(PLUGINS[1]), "false,true");
    do_check_eq(check_plugin_state(PLUGINS[2]), "false,true");
    do_check_eq(check_plugin_state(PLUGINS[3]), "true,true");
    do_check_eq(check_plugin_state(PLUGINS[4]), "false,true");

    
    do_check_eq(check_addon_state(addons[3]), "false,false,true");

    
    do_check_eq(blocklist.getAddonBlocklistURL(addons[0]),create_blocklistURL(addons[0].id));
    do_check_eq(blocklist.getAddonBlocklistURL(addons[1]),create_blocklistURL(addons[1].id));
    do_check_eq(blocklist.getAddonBlocklistURL(addons[2]),create_blocklistURL(addons[2].id));
    do_check_eq(blocklist.getAddonBlocklistURL(addons[3]),create_blocklistURL(addons[3].id));
    do_check_eq(blocklist.getAddonBlocklistURL(addons[4]),create_blocklistURL(addons[4].id));

    
    do_check_eq(blocklist.getPluginBlocklistURL(PLUGINS[0]), create_blocklistURL('test_bug455906_plugin'));
    do_check_eq(blocklist.getPluginBlocklistURL(PLUGINS[1]), create_blocklistURL('test_bug455906_plugin'));
    do_check_eq(blocklist.getPluginBlocklistURL(PLUGINS[2]), create_blocklistURL('test_bug455906_plugin'));
    do_check_eq(blocklist.getPluginBlocklistURL(PLUGINS[3]), create_blocklistURL('test_bug455906_plugin'));
    do_check_eq(blocklist.getPluginBlocklistURL(PLUGINS[4]), create_blocklistURL('test_bug455906_plugin'));

    
    do_check_eq(check_addon_state(addons[5]), "false,false,true");
    do_check_eq(check_addon_state(addons[6]), "false,false,true");
    do_check_eq(check_plugin_state(PLUGINS[5]), "false,true");

    
    gNotificationCheck = null;
    gTestCheck = run_test_pt4;
    load_blocklist("bug455906_start.xml");
  });
}

function run_test_pt4() {
  AddonManager.getAddonByID(ADDONS[4].id, callback_soon(function(addon) {
    addon.userDisabled = false;
    PLUGINS[4].enabledState = Ci.nsIPluginTag.STATE_ENABLED;
    restartManager();
    check_initial_state(function() {
      gNotificationCheck = check_notification_pt4;
      gTestCheck = check_test_pt4;
      load_blocklist("bug455906_empty.xml");
    });
  }));
}

function check_notification_pt4(args) {
  dump("Checking notification pt 4\n");

  
  do_check_eq(args.list.length, 1);
  do_check_false(args.list[0].item instanceof Ci.nsIPluginTag);
  do_check_eq(args.list[0].item.id, "dummy_bug455906_2@tests.mozilla.org");
}

function check_test_pt4() {
  restartManager();
  dump("Checking results pt 4\n");

  AddonManager.getAddonsByIDs([a.id for each (a in ADDONS)], function(addons) {
    
    do_check_eq(check_addon_state(addons[5]), "false,false,false");
    do_check_eq(check_plugin_state(PLUGINS[5]), "false,false");

    
    do_check_eq(check_addon_state(addons[3]), "false,false,false");

    
    do_check_eq(check_addon_state(addons[0]), "true,false,false");
    do_check_eq(check_addon_state(addons[1]), "false,false,false");
    do_check_eq(check_addon_state(addons[2]), "false,false,false");
    do_check_eq(check_addon_state(addons[4]), "false,false,false");
    do_check_eq(check_addon_state(addons[6]), "false,false,true");
    do_check_eq(check_plugin_state(PLUGINS[0]), "true,false");
    do_check_eq(check_plugin_state(PLUGINS[1]), "false,false");
    do_check_eq(check_plugin_state(PLUGINS[2]), "false,false");
    do_check_eq(check_plugin_state(PLUGINS[3]), "true,false");
    do_check_eq(check_plugin_state(PLUGINS[4]), "false,false");

    finish();
  });
}

function finish() {
  gTestserver.stop(do_test_finished);
}
