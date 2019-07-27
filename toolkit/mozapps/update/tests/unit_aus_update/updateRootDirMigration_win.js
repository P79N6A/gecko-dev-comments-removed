




const PREF_APP_UPDATE_MIGRATE_APP_DIR = "app.update.migrated.updateDir";

function clearTaskbarIDHash(exePath, appInfoName) {
  let registry = Cc["@mozilla.org/windows-registry-key;1"].
                 createInstance(Ci.nsIWindowsRegKey);
  try {
    registry.open(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                  "Software\\Mozilla\\" + appInfoName + "\\TaskBarIDs",
                  Ci.nsIWindowsRegKey.ACCESS_ALL);
    registry.removeValue(exePath);
  } catch (e) {
  }
  finally {
    registry.close();
  }
}

function setTaskbarIDHash(exePath, hash, appInfoName) {
  let registry = Cc["@mozilla.org/windows-registry-key;1"].
                 createInstance(Ci.nsIWindowsRegKey);
  try {
    registry.create(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                    "Software\\Mozilla\\" + appInfoName + "\\TaskBarIDs",
                    Ci.nsIWindowsRegKey.ACCESS_WRITE);
    registry.writeStringValue(exePath, hash);
  } catch (e) {
  }
  finally {
    registry.close();
  }
};

function getMigrated() {
  let migrated = 0;
  try {
    migrated = Services.prefs.getBoolPref(PREF_APP_UPDATE_MIGRATE_APP_DIR);
  } catch (e) {
  }
  return migrated;
}



function run_test() {
  setupTestCommon();

  standardInit();

  let appinfo = Cc["@mozilla.org/xre/app-info;1"].
                getService(Ci.nsIXULAppInfo).
                QueryInterface(Ci.nsIXULRuntime);

   
  let updateLeafName;
  let exeFile = FileUtils.getFile(XRE_EXECUTABLE_FILE, []);
  let programFiles = FileUtils.getFile("ProgF", []);
  if (exeFile.path.substring(0, programFiles.path.length).toLowerCase() ==
      programFiles.path.toLowerCase()) {
    updateLeafName = exeFile.parent.leafName;
  } else {
    updateLeafName = appinfo.name;
  }

  
  let oldUpdateRoot;
  if (appinfo.vendor) {
    oldUpdateRoot = FileUtils.getDir("LocalAppData", [appinfo.vendor,
                                                      appinfo.name,
                                                      updateLeafName], false);
  } else {
    oldUpdateRoot = FileUtils.getDir("LocalAppData", [appinfo.name,
                                                      updateLeafName], false);
  }
  
  let newUpdateRoot = FileUtils.getDir("UpdRootD", [], false);

  
  
  

  
  try {
    oldUpdateRoot.remove(true);
  } catch (e) {
  }
  try {
    newUpdateRoot.remove(true);
  } catch (e) {
  }
  Services.prefs.setBoolPref(PREF_APP_UPDATE_MIGRATE_APP_DIR, false);
  setTaskbarIDHash(exeFile.parent.path, "AAAAAAAA", appinfo.name);
  initUpdateServiceStub();
  do_check_eq(getMigrated(), 1);

  
  
  
  

  Services.prefs.setBoolPref(PREF_APP_UPDATE_MIGRATE_APP_DIR, false);
  clearTaskbarIDHash(exeFile.parent.path, appinfo.name);
  initUpdateServiceStub();
  do_check_eq(getMigrated(), 0);

  
  

  Services.prefs.setBoolPref(PREF_APP_UPDATE_MIGRATE_APP_DIR, false);
  setTaskbarIDHash(exeFile.parent.path, "AAAAAAAA", appinfo.name);
  let oldUpdateDirs = oldUpdateRoot.clone();
  oldUpdateDirs.append("updates");
  oldUpdateDirs.append("0");
  oldUpdateDirs.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  
  
  let filesToMigrate = [FILE_UPDATES_DB, FILE_UPDATE_ACTIVE,
                        ["updates", FILE_LAST_LOG], ["updates", FILE_BACKUP_LOG],
                        ["updates", "0", FILE_UPDATE_STATUS]];
  
  filesToMigrate.forEach(relPath => {
    let oldFile = oldUpdateRoot.clone();
    if (relPath instanceof Array) {
      relPath.forEach(relPathPart => {
        oldFile.append(relPathPart);
      });
    } else {
      oldFile.append(relPath);
    }
    oldFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
  });
  
  initUpdateServiceStub();
  doTestFinish();
  return;
  
  filesToMigrate.forEach(relPath => {
    let newFile = newUpdateRoot.clone();
    let oldFile = oldUpdateRoot.clone();
    if (relPath instanceof Array) {
      relPath.forEach(relPathPart => {
        newFile.append(relPathPart);
        oldFile.append(relPathPart);
      });
    } else {
      newFile.append(relPath);
      oldFile.append(relPath);
    }
    
    
    if (newFile.leafName != FILE_UPDATE_STATUS) {
      do_check_true(newFile.exists());
    }
    do_check_false(oldFile.exists());
  });

  doTestFinish();
}

function end_test() {
  let appinfo = Cc["@mozilla.org/xre/app-info;1"].
                getService(Ci.nsIXULAppInfo).
                QueryInterface(Ci.nsIXULRuntime);
  let exeFile = FileUtils.getFile(XRE_EXECUTABLE_FILE, []);
  clearTaskbarIDHash(exeFile.parent.path, appinfo.name);
}
