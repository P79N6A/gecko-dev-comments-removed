



const AM_Cc = Components.classes;
const AM_Ci = Components.interfaces;

const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");

const PREF_EM_CHECK_UPDATE_SECURITY   = "extensions.checkUpdateSecurity";
const PREF_EM_STRICT_COMPATIBILITY    = "extensions.strictCompatibility";
const PREF_EM_MIN_COMPAT_APP_VERSION      = "extensions.minCompatibleAppVersion";
const PREF_EM_MIN_COMPAT_PLATFORM_VERSION = "extensions.minCompatiblePlatformVersion";
const PREF_GETADDONS_BYIDS               = "extensions.getAddons.get.url";
const PREF_GETADDONS_BYIDS_PERFORMANCE   = "extensions.getAddons.getWithPerformance.url";


const TIMEOUT_MS = 900000;

Components.utils.import("resource://gre/modules/addons/AddonRepository.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/Promise.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");
Components.utils.import("resource://gre/modules/osfile.jsm");

Services.prefs.setBoolPref("toolkit.osfile.log", true);


let AMscope = Components.utils.import("resource://gre/modules/AddonManager.jsm");
let AddonManager = AMscope.AddonManager;
let AddonManagerInternal = AMscope.AddonManagerInternal;


let MockAsyncShutdown = {
  hook: null,
  profileBeforeChange: {
    addBlocker: function(aName, aBlocker) {
      do_print("Mock profileBeforeChange blocker for '" + aName + "'");
      MockAsyncShutdown.hook = aBlocker;
    }
  }
};
AMscope.AsyncShutdown = MockAsyncShutdown;

var gInternalManager = null;
var gAppInfo = null;
var gAddonsList;

var gPort = null;
var gUrlToFileMap = {};

var TEST_UNPACKED = false;

function isNightlyChannel() {
  var channel = "default";
  try {
    channel = Services.prefs.getCharPref("app.update.channel");
  }
  catch (e) { }

  return channel != "aurora" && channel != "beta" && channel != "release" && channel != "esr";
}

function createAppInfo(id, name, version, platformVersion) {
  gAppInfo = {
    
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion ? platformVersion : "1.0",
    platformBuildID: "2007010101",

    
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",
    invalidateCachesOnRestart: function invalidateCachesOnRestart() {
      
    },

    
    annotations: {},

    annotateCrashReport: function(key, data) {
      this.annotations[key] = data;
    },

    QueryInterface: XPCOMUtils.generateQI([AM_Ci.nsIXULAppInfo,
                                           AM_Ci.nsIXULRuntime,
                                           AM_Ci.nsICrashReporter,
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










function do_check_in_crash_annotation(aId, aVersion) {
  if (!("nsICrashReporter" in AM_Ci))
    return;

  if (!("Add-ons" in gAppInfo.annotations)) {
    do_check_false(true);
    return;
  }

  let addons = gAppInfo.annotations["Add-ons"].split(",");
  do_check_false(addons.indexOf(encodeURIComponent(aId) + ":" +
                                encodeURIComponent(aVersion)) < 0);
}










function do_check_not_in_crash_annotation(aId, aVersion) {
  if (!("nsICrashReporter" in AM_Ci))
    return;

  if (!("Add-ons" in gAppInfo.annotations)) {
    do_check_true(true);
    return;
  }

  let addons = gAppInfo.annotations["Add-ons"].split(",");
  do_check_true(addons.indexOf(encodeURIComponent(aId) + ":" +
                               encodeURIComponent(aVersion)) < 0);
}








function do_get_addon(aName) {
  return do_get_file("addons/" + aName + ".xpi");
}

function do_get_addon_hash(aName, aAlgorithm) {
  if (!aAlgorithm)
    aAlgorithm = "sha1";

  let file = do_get_addon(aName);

  let crypto = AM_Cc["@mozilla.org/security/hash;1"].
               createInstance(AM_Ci.nsICryptoHash);
  crypto.initWithString(aAlgorithm);
  let fis = AM_Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(AM_Ci.nsIFileInputStream);
  fis.init(file, -1, -1, false);
  crypto.updateFromStream(fis, file.fileSize);

  
  function toHexString(charCode)
    ("0" + charCode.toString(16)).slice(-2);

  let binary = crypto.finish(false);
  return aAlgorithm + ":" + [toHexString(binary.charCodeAt(i)) for (i in binary)].join("")
}








function do_get_addon_root_uri(aProfileDir, aId) {
  let path = aProfileDir.clone();
  path.append(aId);
  if (!path.exists()) {
    path.leafName += ".xpi";
    return "jar:" + Services.io.newFileURI(path).spec + "!/";
  }
  else {
    return Services.io.newFileURI(path).spec;
  }
}

function do_get_expected_addon_name(aId) {
  if (TEST_UNPACKED)
    return aId;
  return aId + ".xpi";
}












function do_check_addons(aActualAddons, aExpectedAddons, aProperties) {
  do_check_neq(aActualAddons, null);
  do_check_eq(aActualAddons.length, aExpectedAddons.length);
  for (let i = 0; i < aActualAddons.length; i++)
    do_check_addon(aActualAddons[i], aExpectedAddons[i], aProperties);
}











function do_check_addon(aActualAddon, aExpectedAddon, aProperties) {
  do_check_neq(aActualAddon, null);

  aProperties.forEach(function(aProperty) {
    let actualValue = aActualAddon[aProperty];
    let expectedValue = aExpectedAddon[aProperty];

    
    if (!(aProperty in aExpectedAddon)) {
      if (actualValue !== undefined && actualValue !== null) {
        do_throw("Unexpected defined/non-null property for add-on " +
                 aExpectedAddon.id + " (addon[" + aProperty + "] = " +
                 actualValue.toSource() + ")");
      }

      return;
    }
    else if (expectedValue && !actualValue) {
      do_throw("Missing property for add-on " + aExpectedAddon.id +
        ": expected addon[" + aProperty + "] = " + expectedValue);
      return;
    }

    switch (aProperty) {
      case "creator":
        do_check_author(actualValue, expectedValue);
        break;

      case "developers":
      case "translators":
      case "contributors":
        do_check_eq(actualValue.length, expectedValue.length);
        for (let i = 0; i < actualValue.length; i++)
          do_check_author(actualValue[i], expectedValue[i]);
        break;

      case "screenshots":
        do_check_eq(actualValue.length, expectedValue.length);
        for (let i = 0; i < actualValue.length; i++)
          do_check_screenshot(actualValue[i], expectedValue[i]);
        break;

      case "sourceURI":
        do_check_eq(actualValue.spec, expectedValue);
        break;

      case "updateDate":
        do_check_eq(actualValue.getTime(), expectedValue.getTime());
        break;

      case "compatibilityOverrides":
        do_check_eq(actualValue.length, expectedValue.length);
        for (let i = 0; i < actualValue.length; i++)
          do_check_compatibilityoverride(actualValue[i], expectedValue[i]);
        break;

      case "icons":
        do_check_icons(actualValue, expectedValue);
        break;

      default:
        if (remove_port(actualValue) !== remove_port(expectedValue))
          do_throw("Failed for " + aProperty + " for add-on " + aExpectedAddon.id +
                   " (" + actualValue + " === " + expectedValue + ")");
    }
  });
}









function do_check_author(aActual, aExpected) {
  do_check_eq(aActual.toString(), aExpected.name);
  do_check_eq(aActual.name, aExpected.name);
  do_check_eq(aActual.url, aExpected.url);
}









function do_check_screenshot(aActual, aExpected) {
  do_check_eq(aActual.toString(), aExpected.url);
  do_check_eq(aActual.url, aExpected.url);
  do_check_eq(aActual.width, aExpected.width);
  do_check_eq(aActual.height, aExpected.height);
  do_check_eq(aActual.thumbnailURL, aExpected.thumbnailURL);
  do_check_eq(aActual.thumbnailWidth, aExpected.thumbnailWidth);
  do_check_eq(aActual.thumbnailHeight, aExpected.thumbnailHeight);
  do_check_eq(aActual.caption, aExpected.caption);
}










function do_check_compatibilityoverride(aActual, aExpected) {
  do_check_eq(aActual.type, aExpected.type);
  do_check_eq(aActual.minVersion, aExpected.minVersion);
  do_check_eq(aActual.maxVersion, aExpected.maxVersion);
  do_check_eq(aActual.appID, aExpected.appID);
  do_check_eq(aActual.appMinVersion, aExpected.appMinVersion);
  do_check_eq(aActual.appMaxVersion, aExpected.appMaxVersion);
}

function do_check_icons(aActual, aExpected) {
  for (var size in aExpected) {
    do_check_eq(remove_port(aActual[size]), remove_port(aExpected[size]));
  }
}



let gXPISaveError = null;









function startupManager(aAppChanged) {
  if (gInternalManager)
    do_throw("Test attempt to startup manager that was already started.");

  if (aAppChanged || aAppChanged === undefined) {
    if (gExtensionsINI.exists())
      gExtensionsINI.remove(true);
  }

  gInternalManager = AM_Cc["@mozilla.org/addons/integration;1"].
                     getService(AM_Ci.nsIObserver).
                     QueryInterface(AM_Ci.nsITimerCallback);

  gInternalManager.observe(null, "addons-startup", null);

  
  loadAddonsList();
}









function restartManager(aNewVersion) {
  shutdownManager();
  if (aNewVersion) {
    gAppInfo.version = aNewVersion;
    startupManager(true);
  }
  else {
    startupManager(false);
  }
}

function shutdownManager() {
  if (!gInternalManager)
    return;

  let shutdownDone = false;

  Services.obs.notifyObservers(null, "quit-application-granted", null);
  MockAsyncShutdown.hook().then(
    () => shutdownDone = true,
    err => shutdownDone = true);

  let thr = Services.tm.mainThread;

  
  while (!shutdownDone) {
    thr.processNextEvent(true);
  }

  gInternalManager = null;

  
  loadAddonsList();

  
  gAppInfo.annotations = {};

  
  
  let XPIscope = Components.utils.import("resource://gre/modules/addons/XPIProvider.jsm");
  
  
  gXPISaveError = XPIscope.XPIProvider._shutdownError;
  do_print("gXPISaveError set to: " + gXPISaveError);
  AddonManagerPrivate.unregisterProvider(XPIscope.XPIProvider);
  Components.utils.unload("resource://gre/modules/addons/XPIProvider.jsm");
}

function loadAddonsList() {
  function readDirectories(aSection) {
    var dirs = [];
    var keys = parser.getKeys(aSection);
    while (keys.hasMore()) {
      let descriptor = parser.getString(aSection, keys.getNext());
      try {
        let file = AM_Cc["@mozilla.org/file/local;1"].
                   createInstance(AM_Ci.nsIFile);
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

  if (!gExtensionsINI.exists())
    return;

  var factory = AM_Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
                getService(AM_Ci.nsIINIParserFactory);
  var parser = factory.createINIParser(gExtensionsINI);
  gAddonsList.extensions = readDirectories("ExtensionDirs");
  gAddonsList.themes = readDirectories("ThemeDirs");
}

function isItemInAddonsList(aType, aDir, aId) {
  var path = aDir.clone();
  path.append(aId);
  var xpiPath = aDir.clone();
  xpiPath.append(aId + ".xpi");
  for (var i = 0; i < gAddonsList[aType].length; i++) {
    let file = gAddonsList[aType][i];
    if (!file.exists())
      do_throw("Non-existant path found in extensions.ini: " + file.path)
    if (file.isDirectory() && file.equals(path))
      return true;
    if (file.isFile() && file.equals(xpiPath))
      return true;
  }
  return false;
}

function isThemeInAddonsList(aDir, aId) {
  return isItemInAddonsList("themes", aDir, aId);
}

function isExtensionInAddonsList(aDir, aId) {
  return isItemInAddonsList("extensions", aDir, aId);
}

function check_startup_changes(aType, aIds) {
  var ids = aIds.slice(0);
  ids.sort();
  var changes = AddonManager.getStartupChanges(aType);
  changes = changes.filter(function(aEl) /@tests.mozilla.org$/.test(aEl));
  changes.sort();

  do_check_eq(JSON.stringify(ids), JSON.stringify(changes));
}








function escapeXML(aStr) {
  return aStr.toString()
             .replace(/&/g, "&amp;")
             .replace(/"/g, "&quot;")
             .replace(/</g, "&lt;")
             .replace(/>/g, "&gt;");
}

function writeLocaleStrings(aData) {
  let rdf = "";
  ["name", "description", "creator", "homepageURL"].forEach(function(aProp) {
    if (aProp in aData)
      rdf += "<em:" + aProp + ">" + escapeXML(aData[aProp]) + "</em:" + aProp + ">\n";
  });

  ["developer", "translator", "contributor"].forEach(function(aProp) {
    if (aProp in aData) {
      aData[aProp].forEach(function(aValue) {
        rdf += "<em:" + aProp + ">" + escapeXML(aValue) + "</em:" + aProp + ">\n";
      });
    }
  });
  return rdf;
}

function createInstallRDF(aData) {
  var rdf = '<?xml version="1.0"?>\n';
  rdf += '<RDF xmlns="http://www.w3.org/1999/02/22-rdf-syntax-ns#"\n' +
         '     xmlns:em="http://www.mozilla.org/2004/em-rdf#">\n';
  rdf += '<Description about="urn:mozilla:install-manifest">\n';

  ["id", "version", "type", "internalName", "updateURL", "updateKey",
   "optionsURL", "optionsType", "aboutURL", "iconURL", "icon64URL",
   "skinnable", "bootstrap", "strictCompatibility"].forEach(function(aProp) {
    if (aProp in aData)
      rdf += "<em:" + aProp + ">" + escapeXML(aData[aProp]) + "</em:" + aProp + ">\n";
  });

  rdf += writeLocaleStrings(aData);

  if ("targetPlatforms" in aData) {
    aData.targetPlatforms.forEach(function(aPlatform) {
      rdf += "<em:targetPlatform>" + escapeXML(aPlatform) + "</em:targetPlatform>\n";
    });
  }

  if ("targetApplications" in aData) {
    aData.targetApplications.forEach(function(aApp) {
      rdf += "<em:targetApplication><Description>\n";
      ["id", "minVersion", "maxVersion"].forEach(function(aProp) {
        if (aProp in aApp)
          rdf += "<em:" + aProp + ">" + escapeXML(aApp[aProp]) + "</em:" + aProp + ">\n";
      });
      rdf += "</Description></em:targetApplication>\n";
    });
  }

  if ("localized" in aData) {
    aData.localized.forEach(function(aLocalized) {
      rdf += "<em:localized><Description>\n";
      if ("locale" in aLocalized) {
        aLocalized.locale.forEach(function(aLocaleName) {
          rdf += "<em:locale>" + escapeXML(aLocaleName) + "</em:locale>\n";
        });
      }
      rdf += writeLocaleStrings(aLocalized);
      rdf += "</Description></em:localized>\n";
    });
  }

  rdf += "</Description>\n</RDF>\n";
  return rdf;
}















function writeInstallRDFToDir(aData, aDir, aExtraFile) {
  var rdf = createInstallRDF(aData);
  if (!aDir.exists())
    aDir.create(AM_Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  var file = aDir.clone();
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

  if (!aExtraFile)
    return;

  file = aDir.clone();
  file.append(aExtraFile);
  file.create(AM_Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
}


















function writeInstallRDFForExtension(aData, aDir, aId, aExtraFile) {
  var id = aId ? aId : aData.id

  var dir = aDir.clone();

  if (TEST_UNPACKED) {
    dir.append(id);
    writeInstallRDFToDir(aData, dir, aExtraFile);
    return dir;
  }

  if (!dir.exists())
    dir.create(AM_Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  dir.append(id + ".xpi");
  var rdf = createInstallRDF(aData);
  var stream = AM_Cc["@mozilla.org/io/string-input-stream;1"].
               createInstance(AM_Ci.nsIStringInputStream);
  stream.setData(rdf, -1);
  var zipW = AM_Cc["@mozilla.org/zipwriter;1"].
             createInstance(AM_Ci.nsIZipWriter);
  zipW.open(dir, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE | FileUtils.MODE_TRUNCATE);
  zipW.addEntryStream("install.rdf", 0, AM_Ci.nsIZipWriter.COMPRESSION_NONE,
                      stream, false);
  if (aExtraFile)
    zipW.addEntryStream(aExtraFile, 0, AM_Ci.nsIZipWriter.COMPRESSION_NONE,
                        stream, false);
  zipW.close();
  return dir;
}











function setExtensionModifiedTime(aExt, aTime) {
  aExt.lastModifiedTime = aTime;
  if (aExt.isDirectory()) {
    let entries = aExt.directoryEntries
                      .QueryInterface(AM_Ci.nsIDirectoryEnumerator);
    while (entries.hasMoreElements())
      setExtensionModifiedTime(entries.nextFile, aTime);
    entries.close();
  }
}
function promiseSetExtensionModifiedTime(aPath, aTime) {
  return Task.spawn(function* () {
    yield OS.File.setDates(aPath, aTime, aTime);
    let entries, iterator;
    try {
      let iterator = new OS.File.DirectoryIterator(aPath);
      entries = yield iterator.nextBatch();
    } catch (ex if ex instanceof OS.File.Error) {
      return;
    } finally {
      if (iterator) {
        iterator.close();
      }
    }
    for (let entry of entries) {
      yield promiseSetExtensionModifiedTime(entry.path, aTime);
    }
  });
}













function manuallyInstall(aXPIFile, aInstallLocation, aID) {
  if (TEST_UNPACKED) {
    let dir = aInstallLocation.clone();
    dir.append(aID);
    dir.create(AM_Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
    let zip = AM_Cc["@mozilla.org/libjar/zip-reader;1"].
              createInstance(AM_Ci.nsIZipReader);
    zip.open(aXPIFile);
    let entries = zip.findEntries(null);
    while (entries.hasMore()) {
      let entry = entries.getNext();
      let target = dir.clone();
      entry.split("/").forEach(function(aPart) {
        target.append(aPart);
      });
      zip.extract(entry, target);
    }
    zip.close();

    return dir;
  }
  else {
    let target = aInstallLocation.clone();
    target.append(aID + ".xpi");
    aXPIFile.copyTo(target.parent, target.leafName);
    return target;
  }
}










function manuallyUninstall(aInstallLocation, aID) {
  let file = getFileForAddon(aInstallLocation, aID);

  
  
  if (file.isFile())
    Services.obs.notifyObservers(file, "flush-cache-entry", null);

  file.remove(true);
}











function getFileForAddon(aDir, aId) {
  var dir = aDir.clone();
  dir.append(do_get_expected_addon_name(aId));
  return dir;
}

function registerDirectory(aKey, aDir) {
  var dirProvider = {
    getFile: function(aProp, aPersistent) {
      aPersistent.value = true;
      if (aProp == aKey)
        return aDir.clone();
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

function getExpectedEvent(aId) {
  if (!(aId in gExpectedEvents))
    do_throw("Wasn't expecting events for " + aId);
  if (gExpectedEvents[aId].length == 0)
    do_throw("Too many events for " + aId);
  let event = gExpectedEvents[aId].shift();
  if (event instanceof Array)
    return event;
  return [event, true];
}

function getExpectedInstall(aAddon) {
  if (gExpectedInstalls instanceof Array)
    return gExpectedInstalls.shift();
  if (!aAddon || !aAddon.id)
    return gExpectedInstalls["NO_ID"].shift();
  let id = aAddon.id;
  if (!(id in gExpectedInstalls) || !(gExpectedInstalls[id] instanceof Array))
    do_throw("Wasn't expecting events for " + id);
  if (gExpectedInstalls[id].length == 0)
    do_throw("Too many events for " + id);
  return gExpectedInstalls[id].shift();
}

const AddonListener = {
  onPropertyChanged: function(aAddon, aProperties) {
    let [event, properties] = getExpectedEvent(aAddon.id);
    do_check_eq("onPropertyChanged", event);
    do_check_eq(aProperties.length, properties.length);
    properties.forEach(function(aProperty) {
      
      
      if (aProperties.indexOf(aProperty) == -1)
        do_throw("Did not see property change for " + aProperty);
    });
    return check_test_completed(arguments);
  },

  onEnabling: function(aAddon, aRequiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onEnabling", event);
    do_check_eq(aRequiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(aAddon.pendingOperations, AddonManager.PENDING_ENABLE));
    do_check_false(hasFlag(aAddon.permissions, AddonManager.PERM_CAN_ENABLE));
    return check_test_completed(arguments);
  },

  onEnabled: function(aAddon) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onEnabled", event);
    do_check_false(hasFlag(aAddon.permissions, AddonManager.PERM_CAN_ENABLE));
    return check_test_completed(arguments);
  },

  onDisabling: function(aAddon, aRequiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onDisabling", event);
    do_check_eq(aRequiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(aAddon.pendingOperations, AddonManager.PENDING_DISABLE));
    do_check_false(hasFlag(aAddon.permissions, AddonManager.PERM_CAN_DISABLE));
    return check_test_completed(arguments);
  },

  onDisabled: function(aAddon) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onDisabled", event);
    do_check_false(hasFlag(aAddon.permissions, AddonManager.PERM_CAN_DISABLE));
    return check_test_completed(arguments);
  },

  onInstalling: function(aAddon, aRequiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onInstalling", event);
    do_check_eq(aRequiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(aAddon.pendingOperations, AddonManager.PENDING_INSTALL));
    return check_test_completed(arguments);
  },

  onInstalled: function(aAddon) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onInstalled", event);
    return check_test_completed(arguments);
  },

  onUninstalling: function(aAddon, aRequiresRestart) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onUninstalling", event);
    do_check_eq(aRequiresRestart, expectedRestart);
    if (expectedRestart)
      do_check_true(hasFlag(aAddon.pendingOperations, AddonManager.PENDING_UNINSTALL));
    return check_test_completed(arguments);
  },

  onUninstalled: function(aAddon) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onUninstalled", event);
    return check_test_completed(arguments);
  },

  onOperationCancelled: function(aAddon) {
    let [event, expectedRestart] = getExpectedEvent(aAddon.id);
    do_check_eq("onOperationCancelled", event);
    return check_test_completed(arguments);
  }
};

const InstallListener = {
  onNewInstall: function(install) {
    if (install.state != AddonManager.STATE_DOWNLOADED &&
        install.state != AddonManager.STATE_AVAILABLE)
      do_throw("Bad install state " + install.state);
    do_check_eq(install.error, 0);
    do_check_eq("onNewInstall", getExpectedInstall());
    return check_test_completed(arguments);
  },

  onDownloadStarted: function(install) {
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADING);
    do_check_eq(install.error, 0);
    do_check_eq("onDownloadStarted", getExpectedInstall());
    return check_test_completed(arguments);
  },

  onDownloadEnded: function(install) {
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);
    do_check_eq(install.error, 0);
    do_check_eq("onDownloadEnded", getExpectedInstall());
    return check_test_completed(arguments);
  },

  onDownloadFailed: function(install) {
    do_check_eq(install.state, AddonManager.STATE_DOWNLOAD_FAILED);
    do_check_eq("onDownloadFailed", getExpectedInstall());
    return check_test_completed(arguments);
  },

  onDownloadCancelled: function(install) {
    do_check_eq(install.state, AddonManager.STATE_CANCELLED);
    do_check_eq(install.error, 0);
    do_check_eq("onDownloadCancelled", getExpectedInstall());
    return check_test_completed(arguments);
  },

  onInstallStarted: function(install) {
    do_check_eq(install.state, AddonManager.STATE_INSTALLING);
    do_check_eq(install.error, 0);
    do_check_eq("onInstallStarted", getExpectedInstall(install.addon));
    return check_test_completed(arguments);
  },

  onInstallEnded: function(install, newAddon) {
    do_check_eq(install.state, AddonManager.STATE_INSTALLED);
    do_check_eq(install.error, 0);
    do_check_eq("onInstallEnded", getExpectedInstall(install.addon));
    return check_test_completed(arguments);
  },

  onInstallFailed: function(install) {
    do_check_eq(install.state, AddonManager.STATE_INSTALL_FAILED);
    do_check_eq("onInstallFailed", getExpectedInstall(install.addon));
    return check_test_completed(arguments);
  },

  onInstallCancelled: function(install) {
    
    
    let possibleStates = [AddonManager.STATE_CANCELLED,
                          AddonManager.STATE_DOWNLOADED];
    do_check_true(possibleStates.indexOf(install.state) != -1);
    do_check_eq(install.error, 0);
    do_check_eq("onInstallCancelled", getExpectedInstall(install.addon));
    return check_test_completed(arguments);
  },

  onExternalInstall: function(aAddon, existingAddon, aRequiresRestart) {
    do_check_eq("onExternalInstall", getExpectedInstall(aAddon));
    do_check_false(aRequiresRestart);
    return check_test_completed(arguments);
  }
};

function hasFlag(aBits, aFlag) {
  return (aBits & aFlag) != 0;
}


function prepare_test(aExpectedEvents, aExpectedInstalls, aNext) {
  AddonManager.addAddonListener(AddonListener);
  AddonManager.addInstallListener(InstallListener);

  gExpectedInstalls = aExpectedInstalls;
  gExpectedEvents = aExpectedEvents;
  gNext = aNext;
}


function check_test_completed(aArgs) {
  if (!gNext)
    return undefined;

  if (gExpectedInstalls instanceof Array &&
      gExpectedInstalls.length > 0)
    return undefined;
  else for each (let installList in gExpectedInstalls) {
    if (installList.length > 0)
      return undefined;
  }

  for (let id in gExpectedEvents) {
    if (gExpectedEvents[id].length > 0)
      return undefined;
  }

  return gNext.apply(null, aArgs);
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










function completeAllInstalls(aInstalls, aCallback) {
  let count = aInstalls.length;

  if (count == 0) {
    aCallback();
    return;
  }

  function installCompleted(aInstall) {
    aInstall.removeListener(listener);

    if (--count == 0)
      do_execute_soon(aCallback);
  }

  let listener = {
    onDownloadFailed: installCompleted,
    onDownloadCancelled: installCompleted,
    onInstallFailed: installCompleted,
    onInstallCancelled: installCompleted,
    onInstallEnded: installCompleted
  };

  aInstalls.forEach(function(aInstall) {
    aInstall.addListener(listener);
    aInstall.install();
  });
}













function installAllFiles(aFiles, aCallback, aIgnoreIncompatible) {
  let count = aFiles.length;
  let installs = [];
  function callback() {
    if (aCallback) {
      aCallback();
    }
  }
  aFiles.forEach(function(aFile) {
    AddonManager.getInstallForFile(aFile, function(aInstall) {
      if (!aInstall)
        do_throw("No AddonInstall created for " + aFile.path);
      do_check_eq(aInstall.state, AddonManager.STATE_DOWNLOADED);

      if (!aIgnoreIncompatible || !aInstall.addon.appDisabled)
        installs.push(aInstall);

      if (--count == 0)
        completeAllInstalls(installs, callback);
    });
  });
}

function promiseInstallAllFiles(aFiles, aIgnoreIncompatible) {
  let deferred = Promise.defer();
  installAllFiles(aFiles, deferred.resolve, aIgnoreIncompatible);
  return deferred.promise;

}

if ("nsIWindowsRegKey" in AM_Ci) {
  var MockRegistry = {
    LOCAL_MACHINE: {},
    CURRENT_USER: {},
    CLASSES_ROOT: {},

    getRoot: function(aRoot) {
      switch (aRoot) {
      case AM_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE:
        return MockRegistry.LOCAL_MACHINE;
      case AM_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER:
        return MockRegistry.CURRENT_USER;
      case AM_Ci.nsIWindowsRegKey.ROOT_KEY_CLASSES_ROOT:
        return MockRegistry.CLASSES_ROOT;
      default:
        do_throw("Unknown root " + aRootKey);
        return null;
      }
    },

    setValue: function(aRoot, aPath, aName, aValue) {
      let rootKey = MockRegistry.getRoot(aRoot);

      if (!(aPath in rootKey)) {
        rootKey[aPath] = [];
      }
      else {
        for (let i = 0; i < rootKey[aPath].length; i++) {
          if (rootKey[aPath][i].name == aName) {
            if (aValue === null)
              rootKey[aPath].splice(i, 1);
            else
              rootKey[aPath][i].value = aValue;
            return;
          }
        }
      }

      if (aValue === null)
        return;

      rootKey[aPath].push({
        name: aName,
        value: aValue
      });
    }
  };

  



  function MockWindowsRegKey() {
  }

  MockWindowsRegKey.prototype = {
    values: null,

    
    QueryInterface: XPCOMUtils.generateQI([AM_Ci.nsIWindowsRegKey]),

    
    open: function(aRootKey, aRelPath, aMode) {
      let rootKey = MockRegistry.getRoot(aRootKey);

      if (!(aRelPath in rootKey))
        rootKey[aRelPath] = [];
      this.values = rootKey[aRelPath];
    },

    close: function() {
      this.values = null;
    },

    get valueCount() {
      if (!this.values)
        throw Components.results.NS_ERROR_FAILURE;
      return this.values.length;
    },

    getValueName: function(aIndex) {
      if (!this.values || aIndex >= this.values.length)
        throw Components.results.NS_ERROR_FAILURE;
      return this.values[aIndex].name;
    },

    readStringValue: function(aName) {
      for (let value of this.values) {
        if (value.name == aName)
          return value.value;
      }
      return null;
    }
  };

  var WinRegFactory = {
    createInstance: function(aOuter, aIid) {
      if (aOuter != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

      var key = new MockWindowsRegKey();
      return key.QueryInterface(aIid);
    }
  };

  var registrar = Components.manager.QueryInterface(AM_Ci.nsIComponentRegistrar);
  registrar.registerFactory(Components.ID("{0478de5b-0f38-4edb-851d-4c99f1ed8eba}"),
                            "Mock Windows Registry Implementation",
                            "@mozilla.org/windows-registry-key;1", WinRegFactory);
}


const gProfD = do_get_profile();

const EXTENSIONS_DB = "extensions.json";
let gExtensionsJSON = gProfD.clone();
gExtensionsJSON.append(EXTENSIONS_DB);

const EXTENSIONS_INI = "extensions.ini";
let gExtensionsINI = gProfD.clone();
gExtensionsINI.append(EXTENSIONS_INI);


Services.prefs.setBoolPref("extensions.logging.enabled", true);


Services.prefs.setIntPref("extensions.enabledScopes", AddonManager.SCOPE_PROFILE);


Services.prefs.setIntPref("extensions.autoDisableScopes", 0);


Services.prefs.setBoolPref("extensions.getAddons.cache.enabled", false);


Services.prefs.setBoolPref("extensions.showMismatchUI", false);


Services.prefs.setCharPref("extensions.update.url", "http://127.0.0.1/updateURL");
Services.prefs.setCharPref("extensions.update.background.url", "http://127.0.0.1/updateBackgroundURL");
Services.prefs.setCharPref("extensions.blocklist.url", "http://127.0.0.1/blocklistURL");


Services.prefs.setBoolPref("extensions.installDistroAddons", false);


Services.prefs.setBoolPref("extensions.strictCompatibility", true);


Services.prefs.setCharPref("extensions.hotfix.id", "");


Services.prefs.setCharPref(PREF_EM_MIN_COMPAT_APP_VERSION, "0");
Services.prefs.setCharPref(PREF_EM_MIN_COMPAT_PLATFORM_VERSION, "0");


const gTmpD = gProfD.clone();
gTmpD.append("temp");
gTmpD.create(AM_Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
registerDirectory("TmpD", gTmpD);



var blockFile = gProfD.clone();
blockFile.append("blocklist.xml");
var stream = AM_Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(AM_Ci.nsIFileOutputStream);
stream.init(blockFile, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE | FileUtils.MODE_TRUNCATE,
            FileUtils.PERMS_FILE, 0);

var data = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
           "<blocklist xmlns=\"http://www.mozilla.org/2006/addons-blocklist\">\n" +
           "</blocklist>\n";
stream.write(data, data.length);
stream.close();



function timeout() {
  timer = null;
  do_throw("Test ran longer than " + TIMEOUT_MS + "ms");

  
  do_test_finished();
}

var timer = AM_Cc["@mozilla.org/timer;1"].createInstance(AM_Ci.nsITimer);
timer.init(timeout, TIMEOUT_MS, AM_Ci.nsITimer.TYPE_ONE_SHOT);


function pathShouldntExist(aPath) {
  if (aPath.exists()) {
    do_throw("Test cleanup: path " + aPath.path + " exists when it should not");
  }
}

do_register_cleanup(function addon_cleanup() {
  if (timer)
    timer.cancel();

  
  var dirEntries = gTmpD.directoryEntries
                        .QueryInterface(AM_Ci.nsIDirectoryEnumerator);
  var entry;
  while ((entry = dirEntries.nextFile)) {
    do_throw("Found unexpected file in temporary directory: " + entry.leafName);
  }
  dirEntries.close();

  var testDir = gProfD.clone();
  testDir.append("extensions");
  testDir.append("trash");
  pathShouldntExist(testDir);

  testDir.leafName = "staged";
  pathShouldntExist(testDir);

  testDir.leafName = "staged-xpis";
  pathShouldntExist(testDir);

  shutdownManager();

  
  try {
    Services.prefs.clearUserPref(PREF_EM_CHECK_UPDATE_SECURITY);
  } catch (e) {}
  try {
    Services.prefs.clearUserPref(PREF_EM_STRICT_COMPATIBILITY);
  } catch (e) {}
});







function interpolateAndServeFile(request, response) {
  try {
    let file = gUrlToFileMap[request.path];
    var data = "";
    var fstream = Components.classes["@mozilla.org/network/file-input-stream;1"].
    createInstance(Components.interfaces.nsIFileInputStream);
    var cstream = Components.classes["@mozilla.org/intl/converter-input-stream;1"].
    createInstance(Components.interfaces.nsIConverterInputStream);
    fstream.init(file, -1, 0, 0);
    cstream.init(fstream, "UTF-8", 0, 0);

    let (str = {}) {
      let read = 0;
      do {
        
        read = cstream.readString(0xffffffff, str);
        data += str.value;
      } while (read != 0);
    }
    data = data.replace(/%PORT%/g, gPort);

    response.write(data);
  } catch (e) {
    do_throw("Exception while serving interpolated file.");
  } finally {
    cstream.close(); 
  }
}










function mapUrlToFile(url, file, server) {
  server.registerPathHandler(url, interpolateAndServeFile);
  gUrlToFileMap[url] = file;
}

function mapFile(path, server) {
  mapUrlToFile(path, do_get_file(path), server);
}







function remove_port(url) {
  if (typeof url === "string")
    return url.replace(/:\d+/, "");
  return url;
}

function do_exception_wrap(func) {
  return function() {
    try {
      func.apply(null, arguments);
    }
    catch(e) {
      do_report_unexpected_exception(e);
    }
  };
}




function changeXPIDBVersion(aNewVersion) {
  let jData = loadJSON(gExtensionsJSON);
  jData.schemaVersion = aNewVersion;
  saveJSON(jData, gExtensionsJSON);
}




function loadFile(aFile) {
  let data = "";
  let fstream = Components.classes["@mozilla.org/network/file-input-stream;1"].
          createInstance(Components.interfaces.nsIFileInputStream);
  let cstream = Components.classes["@mozilla.org/intl/converter-input-stream;1"].
          createInstance(Components.interfaces.nsIConverterInputStream);
  fstream.init(aFile, -1, 0, 0);
  cstream.init(fstream, "UTF-8", 0, 0);
  let (str = {}) {
    let read = 0;
    do {
      read = cstream.readString(0xffffffff, str); 
      data += str.value;
    } while (read != 0);
  }
  cstream.close();
  return data;
}




function loadJSON(aFile) {
  let data = loadFile(aFile);
  do_print("Loaded JSON file " + aFile.path);
  return(JSON.parse(data));
}




function saveJSON(aData, aFile) {
  do_print("Starting to save JSON file " + aFile.path);
  let stream = FileUtils.openSafeFileOutputStream(aFile);
  let converter = AM_Cc["@mozilla.org/intl/converter-output-stream;1"].
    createInstance(AM_Ci.nsIConverterOutputStream);
  converter.init(stream, "UTF-8", 0, 0x0000);
  
  converter.writeString(JSON.stringify(aData, null, 2));
  converter.flush();
  
  FileUtils.closeSafeFileOutputStream(stream);
  converter.close();
  do_print("Done saving JSON file " + aFile.path);
}




function callback_soon(aFunction) {
  return function(...args) {
    do_execute_soon(function() {
      aFunction.apply(null, args);
    }, aFunction.name ? "delayed callback " + aFunction.name : "delayed callback");
  }
}









function promiseAddonsByIDs(list) {
  let deferred = Promise.defer();
  AddonManager.getAddonsByIDs(list, deferred.resolve);
  return deferred.promise;
}
