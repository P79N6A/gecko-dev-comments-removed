



const AM_Cc = Components.classes;
const AM_Ci = Components.interfaces;

const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");

Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

var gInternalManager = null;
var gAppInfo = null;
var gAddonsList;

function createAppInfo(id, name, version, platformVersion) {
  gAppInfo = {
    
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion,
    platformBuildID: "2007010101",

    
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",
    invalidateCachesOnRestart: function invalidateCachesOnRestart() {
      
    },

    QueryInterface: XPCOMUtils.generateQI([AM_Ci.nsIXULAppInfo,
                                           AM_Ci.nsIXULRuntime,
                                           AM_Ci.nsISupports])
  };
  
  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return gAppInfo.QueryInterface(iid);
    }
  };
  var registrar = Components.manager.QueryInterface(AM_Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}








function do_get_addon(name) {
  return do_get_file("addons/" + name + ".xpi");
}














function startupManager(expectedRestarts, appChanged) {
  if (gInternalManager)
    do_throw("Test attempt to startup manager that was already started.");

  if (appChanged === undefined)
    appChanged = true;

  
  loadAddonsList(appChanged);

  gInternalManager = AM_Cc["@mozilla.org/addons/integration;1"].
                     getService(AM_Ci.nsIObserver).
                     QueryInterface(AM_Ci.nsITimerCallback);

  gInternalManager.observe(null, "profile-after-change", "startup");

  let appStartup = AM_Cc["@mozilla.org/toolkit/app-startup;1"].
                   getService(AM_Ci.nsIAppStartup2);
  var restart = appChanged || appStartup.needsRestart;
  appStartup.needsRestart = false;

  if (restart) {
    if (expectedRestarts !== undefined)
      restartManager(expectedRestarts - 1);
    else
      restartManager();
  }
  else if (expectedRestarts !== undefined) {
    if (expectedRestarts > 0)
      do_throw("Expected to need to restart " + expectedRestarts + " more times");
    else if (expectedRestarts < 0)
      do_throw("Restarted " + (-expectedRestarts) + " more times than expected");
  }
}














function restartManager(expectedRestarts, newVersion) {
  shutdownManager();
  if (newVersion) {
    gAppInfo.version = newVersion;
    startupManager(expectedRestarts, true);
  }
  else {
    startupManager(expectedRestarts, false);
  }
}

function shutdownManager() {
  if (!gInternalManager)
    return;

  let obs = AM_Cc["@mozilla.org/observer-service;1"].
            getService(AM_Ci.nsIObserverService);
  obs.notifyObservers(null, "quit-application-granted", null);
  gInternalManager.observe(null, "xpcom-shutdown", null);
  gInternalManager = null;
}

function loadAddonsList(appChanged) {
  function readDirectories(section) {
    var dirs = [];
    var keys = parser.getKeys(section);
    while (keys.hasMore()) {
      let descriptor = parser.getString(section, keys.getNext());
      try {
        let file = AM_Cc["@mozilla.org/file/local;1"].
                   createInstance(AM_Ci.nsILocalFile);
        file.persistentDescriptor = descriptor;
        dirs.push(file);
      }
      catch (e) {
        
        
      }
    }
    return dirs;
  }

  gAddonsList = {
    extensions: [],
    themes: []
  };

  var file = gProfD.clone();
  file.append("extensions.ini");
  if (!file.exists())
    return;
  if (appChanged) {
    file.remove(true);
    return;
  }

  var factory = AM_Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
                getService(AM_Ci.nsIINIParserFactory);
  var parser = factory.createINIParser(file);
  gAddonsList.extensions = readDirectories("ExtensionDirs");
  gAddonsList.themes = readDirectories("ThemeDirs");
}

function isItemInAddonsList(type, dir, id) {
  var path = dir.clone();
  path.append(id);
  for (var i = 0; i < gAddonsList[type].length; i++) {
    if (gAddonsList[type][i].equals(path))
      return true;
  }
  return false;
}

function isThemeInAddonsList(dir, id) {
  return isItemInAddonsList("themes", dir, id);
}

function isExtensionInAddonsList(dir, id) {
  return isItemInAddonsList("extensions", dir, id);
}

function writeLocaleStrings(data) {
  let rdf = "";
  ["name", "description", "creator", "homepageURL"].forEach(function(prop) {
    if (prop in data)
      rdf += "<em:" + prop + ">" + data[prop] + "</em:" + prop + ">\n";
  });

  ["developer", "translator", "contributor"].forEach(function(prop) {
    if (prop in data) {
      data[prop].forEach(function(value) {
        rdf += "<em:" + prop + ">" + value + "</em:" + prop + ">\n";
      });
    }
  });
  return rdf;
}













function writeInstallRDFToDir(data, dir) {
  var rdf = '<?xml version="1.0"?>\n';
  rdf += '<RDF xmlns="http://www.w3.org/1999/02/22-rdf-syntax-ns#"\n' +
         '     xmlns:em="http://www.mozilla.org/2004/em-rdf#">\n';
  rdf += '<Description about="urn:mozilla:install-manifest">\n';

  ["id", "version", "type", "internalName", "updateURL", "updateKey",
   "optionsURL", "aboutURL", "iconURL"].forEach(function(prop) {
    if (prop in data)
      rdf += "<em:" + prop + ">" + data[prop] + "</em:" + prop + ">\n";
  });

  rdf += writeLocaleStrings(data);

  if ("targetApplications" in data) {
    data.targetApplications.forEach(function(app) {
      rdf += "<em:targetApplication><Description>\n";
      ["id", "minVersion", "maxVersion"].forEach(function(prop) {
        if (prop in app)
          rdf += "<em:" + prop + ">" + app[prop] + "</em:" + prop + ">\n";
      });
      rdf += "</Description></em:targetApplication>\n";
    });
  }

  rdf += "</Description>\n</RDF>\n";

  if (!dir.exists())
    dir.create(AM_Ci.nsIFile.DIRECTORY_TYPE, 0755);
  var file = dir.clone();
  file.append("install.rdf");
  if (file.exists())
    file.remove(true);
  var fos = AM_Cc["@mozilla.org/network/file-output-stream;1"].
            createInstance(AM_Ci.nsIFileOutputStream);
  fos.init(file,
           FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE | FileUtils.MODE_TRUNCATE,
           FileUtils.PERMS_FILE, 0);
  fos.write(rdf, rdf.length);
  fos.close();
}

function registerDirectory(key, dir) {
  var dirProvider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == key)
        return dir.clone();
      return null;
    },

    QueryInterface: XPCOMUtils.generateQI([AM_Ci.nsIDirectoryServiceProvider,
                                           AM_Ci.nsISupports])
  };
  Services.dirsvc.registerProvider(dirProvider);
}

var gExpectedEvents = {};
var gExpectedInstalls = [];
var gNext = null;

function getExpectedEvent(id) {
  if (!(id in gExpectedEvents))
    do_throw("Wasn't expecting events for " + id);
  if (gExpectedEvents[id].length == 0)
    do_throw("Too many events for " + id);
  let event = gExpectedEvents[id].shift();
  if (event instanceof Array)
    return event;
  return [event, true];
}

const AddonListener = {
  onEnabling: function(addon, requiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onEnabling", event);
    do_check_eq(requiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(addon.pendingOperations, AddonManager.PENDING_ENABLE));
    do_check_false(hasFlag(addon.permissions, AddonManager.PERM_CAN_ENABLE));
  },

  onEnabled: function(addon) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onEnabled", event);
    do_check_false(hasFlag(addon.permissions, AddonManager.PERM_CAN_ENABLE));
  },

  onDisabling: function(addon, requiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onDisabling", event);
    do_check_eq(requiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(addon.pendingOperations, AddonManager.PENDING_DISABLE));
    do_check_false(hasFlag(addon.permissions, AddonManager.PERM_CAN_DISABLE));
  },

  onDisabled: function(addon) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onDisabled", event);
    do_check_false(hasFlag(addon.permissions, AddonManager.PERM_CAN_DISABLE));
  },

  onInstalling: function(addon, requiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onInstalling", event);
    do_check_eq(requiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(addon.pendingOperations, AddonManager.PENDING_INSTALL));
  },

  onInstalled: function(addon) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onInstalled", event);
  },

  onUninstalling: function(addon, requiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onUninstalling", event);
    do_check_eq(requiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(addon.pendingOperations, AddonManager.PENDING_UNINSTALL));
  },

  onUninstalled: function(addon) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onUninstalled", event);
  },

  onOperationCancelled: function(addon) {
    let [event, expectedRestart] = getExpectedEvent(addon.id);
    do_check_eq("onOperationCancelled", event);
  }
};

const InstallListener = {
  onNewInstall: function(install) {
    if (install.state != AddonManager.STATE_DOWNLOADED &&
        install.state != AddonManager.STATE_AVAILABLE)
      do_throw("Bad install state " + install.state);
    do_check_eq("onNewInstall", gExpectedInstalls.shift());
  },

  onDownloadStarted: function(install) {
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADING);
    do_check_eq("onDownloadStarted", gExpectedInstalls.shift());
  },

  onDownloadEnded: function(install) {
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);
    do_check_eq("onDownloadEnded", gExpectedInstalls.shift());
    
    return gNext(install);
  },

  onDownloadFailed: function(install, status) {
    do_check_eq(install.state, AddonManager.STATE_DOWNLOAD_FAILED);
    do_check_eq("onDownloadFailed", gExpectedInstalls.shift());
    gNext(install);
  },

  onInstallStarted: function(install) {
    do_check_eq(install.state, AddonManager.STATE_INSTALLING);
    do_check_eq("onInstallStarted", gExpectedInstalls.shift());
  },

  onInstallEnded: function(install, newAddon) {
    do_check_eq(install.state, AddonManager.STATE_INSTALLED);
    do_check_eq("onInstallEnded", gExpectedInstalls.shift());
    gNext(install);
  },

  onInstallFailed: function(install, status) {
    do_check_eq(install.state, AddonManager.STATE_INSTALL_FAILED);
    do_check_eq("onInstallFailed", gExpectedInstalls.shift());
    gNext(install);
  },

  onExternalInstall: function(addon, existingAddon, requiresRestart) {
    do_check_eq("onExternalInstall", gExpectedInstalls.shift());
    do_check_false(requiresRestart);
    if (existingAddon)
      do_check_eq(existingAddon.pendingUpgrade, addon);
  }
};

function hasFlag(bits, flag) {
  return (bits & flag) != 0;
}


function prepare_test(expectedEvents, expectedInstalls, next) {
  AddonManager.addAddonListener(AddonListener);
  AddonManager.addInstallListener(InstallListener);

  gExpectedInstalls = expectedInstalls;
  gExpectedEvents = expectedEvents;
  gNext = next;
}


function ensure_test_completed() {
  for (let i in gExpectedEvents) {
    if (gExpectedEvents[i].length > 0)
      do_throw("Didn't see all the expected events for " + i);
  }
  gExpectedEvents = {};
  if (gExpectedInstalls)
    do_check_eq(gExpectedInstalls.length, 0);
}

const gProfD = do_get_profile().QueryInterface(AM_Ci.nsILocalFile);


Services.prefs.setBoolPref("extensions.logging.enabled", true);

do_register_cleanup(shutdownManager);
