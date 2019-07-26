




const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const DIR_UPDATES         = "updates";
const FILE_UPDATES_DB     = "updates.xml";
const FILE_UPDATE_ACTIVE  = "active-update.xml";
const FILE_LAST_LOG       = "last-update.log";
const FILE_BACKUP_LOG     = "backup-update.log";
const FILE_UPDATE_STATUS  = "update.status";

const KEY_UPDROOT         = "UpdRootD";

#ifdef XP_WIN

const PREF_APP_UPDATE_MIGRATE_APP_DIR = "app.update.migrated.updateDir";


function getTaskbarIDHash(rootKey, exePath, appInfoName) {
  let registry = Cc["@mozilla.org/windows-registry-key;1"].
                 createInstance(Ci.nsIWindowsRegKey);
  try {
    registry.open(rootKey, "Software\\Mozilla\\" + appInfoName + "\\TaskBarIDs",
                  Ci.nsIWindowsRegKey.ACCESS_READ);
    if (registry.hasValue(exePath)) {
      return registry.readStringValue(exePath);
    }
  } catch (ex) {
  } finally {
    registry.close();
  }
  return undefined;
};





function migrateOldUpdateDir() {
  
  
  var appinfo = Components.classes["@mozilla.org/xre/app-info;1"].
                getService(Components.interfaces.nsIXULAppInfo).
                QueryInterface(Components.interfaces.nsIXULRuntime);
  var updateLeafName;
  var programFiles = FileUtils.getFile("ProgF", []);
  var exeFile = FileUtils.getFile("XREExeF", []);
  if (exeFile.path.substring(0, programFiles.path.length).toLowerCase() ==
      programFiles.path.toLowerCase()) {
    updateLeafName = exeFile.parent.leafName;
  } else {
    updateLeafName = appinfo.name;
  }

  
  var oldUpdateRoot;
  if (appinfo.vendor) {
    oldUpdateRoot = FileUtils.getDir("LocalAppData", [appinfo.vendor,
                                                      appinfo.name,
                                                      updateLeafName], false);
  } else {
    oldUpdateRoot = FileUtils.getDir("LocalAppData", [appinfo.name,
                                                      updateLeafName], false);
  }

  
  var newUpdateRoot = FileUtils.getDir("UpdRootD", [], true);

  
  
  var taskbarID = getTaskbarIDHash(Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                                   exeFile.parent.path, appinfo.name);
  if (!taskbarID) {
    taskbarID = getTaskbarIDHash(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                                 exeFile.parent.path, appinfo.name);
    if (!taskbarID) {
      return;
    }
  }

  Services.prefs.setBoolPref(PREF_APP_UPDATE_MIGRATE_APP_DIR, true);

  
  if (oldUpdateRoot.path.toLowerCase() == newUpdateRoot.path.toLowerCase() ||
      updateLeafName.length == 0) {
    return;
  }

  
  
  if (!oldUpdateRoot.exists()) {
    return;
  }

  
  
  var filesToMigrate = [FILE_UPDATES_DB, FILE_UPDATE_ACTIVE,
                        ["updates", FILE_LAST_LOG], ["updates", FILE_BACKUP_LOG],
                        ["updates", "0", FILE_UPDATE_STATUS]];

  
  filesToMigrate.forEach(relPath => {
    let oldFile = oldUpdateRoot.clone();
    let newFile = newUpdateRoot.clone();
    if (relPath instanceof Array) {
      relPath.forEach(relPathPart => {
        oldFile.append(relPathPart);
        newFile.append(relPathPart);
      });
    } else {
      oldFile.append(relPath);
      newFile.append(relPath);
    }

    try {
      if (!newFile.exists()) {
        oldFile.moveTo(newFile.parent, newFile.leafName);
      }
    } catch (e) {
      Components.utils.reportError(e);
    }
  });

  oldUpdateRoot.remove(true);
}
#endif









function getUpdateDirNoCreate(pathArray) {
  return FileUtils.getDir(KEY_UPDROOT, pathArray, false);
}

function UpdateServiceStub() {
#ifdef XP_WIN
  
  var migrated = 0;
  try {
    migrated = Services.prefs.getBoolPref(PREF_APP_UPDATE_MIGRATE_APP_DIR);
  } catch (e) {
  }

  if (!migrated) {
    try {
      migrateOldUpdateDir();
    } catch (e) {
      Components.utils.reportError(e);
    }
  }
#endif

  let statusFile = getUpdateDirNoCreate([DIR_UPDATES, "0"]);
  statusFile.append(FILE_UPDATE_STATUS);
  
  if (statusFile.exists()) {
    let aus = Components.classes["@mozilla.org/updates/update-service;1"].
              getService(Ci.nsIApplicationUpdateService).
              QueryInterface(Ci.nsIObserver);
    aus.observe(null, "post-update-processing", "");
  }
}
UpdateServiceStub.prototype = {
  observe: function(){},
  classID: Components.ID("{e43b0010-04ba-4da6-b523-1f92580bc150}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([UpdateServiceStub]);
