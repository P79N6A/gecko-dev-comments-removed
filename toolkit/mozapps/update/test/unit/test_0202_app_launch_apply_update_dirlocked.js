



















const TEST_ID = "0202";



const FILE_UPDATER_INI_BAK = "updater.ini.bak";


const CHECK_TIMEOUT_MILLI = 1000;

let gActiveUpdate;



function symlinkUpdateFilesIntoBundleDirectory() {
  if (!shouldAdjustPathsOnMac()) {
    return;
  }
  
  
  
  
  
  

  Components.utils.import("resource://gre/modules/ctypes.jsm");
  let libc = ctypes.open("/usr/lib/libc.dylib");
  
  
  let symlink = libc.declare("symlink", ctypes.default_abi, ctypes.int,
                             ctypes.char.ptr, ctypes.char.ptr);
  let unlink = libc.declare("unlink", ctypes.default_abi, ctypes.int,
                            ctypes.char.ptr);

  
  let dest = getAppDir();
  dest.append("active-update.xml");
  if (!dest.exists()) {
    dest.create(dest.NORMAL_FILE_TYPE, 0644);
  }
  do_check_true(dest.exists());
  let source = getUpdatesRootDir();
  source.append("active-update.xml");
  unlink(source.path);
  let ret = symlink(dest.path, source.path);
  do_check_eq(ret, 0);
  do_check_true(source.exists());

  
  let dest2 = getAppDir();
  dest2.append("updates");
  if (dest2.exists()) {
    dest2.remove(true);
  }
  dest2.create(dest.DIRECTORY_TYPE, 0755);
  do_check_true(dest2.exists());
  let source2 = getUpdatesRootDir();
  source2.append("updates");
  if (source2.exists()) {
    source2.remove(true);
  }
  ret = symlink(dest2.path, source2.path);
  do_check_eq(ret, 0);
  do_check_true(source2.exists());

  
  do_register_cleanup(function() {
    let ret = unlink(source.path);
    do_check_false(source.exists());
    let ret = unlink(source2.path);
    do_check_false(source2.exists());
  });

  
  
  
  getUpdatesRootDir = getAppDir;
}

function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);

  removeUpdateDirsAndFiles();

  symlinkUpdateFilesIntoBundleDirectory();
  if (IS_WIN) {
    adjustPathsOnWindows();
  }

  if (!gAppBinPath) {
    do_throw("Main application binary not found... expected: " +
             APP_BIN_NAME + APP_BIN_SUFFIX);
    return;
  }

  
  Services.prefs.setBoolPref(PREF_APP_UPDATE_SILENT, true);

  let channel = Services.prefs.getCharPref(PREF_APP_UPDATE_CHANNEL);
  let patches = getLocalPatchString(null, null, null, null, null, "true",
                                    STATE_PENDING);
  let updates = getLocalUpdateString(patches, null, null, null, null, null,
                                     null, null, null, null, null, null,
                                     null, "true", channel);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);

  
  let processDir = getAppDir();
  lockDirectory(processDir);
  let file = processDir.clone();
  file.append("application.ini");
  let ini = AUS_Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
            getService(AUS_Ci.nsIINIParserFactory).
            createINIParser(file);
  let version = ini.getString("App", "Version");
  writeVersionFile(version);
  writeStatusFile(STATE_PENDING);

  
  let oldUpdatedDir = processDir.clone();
  oldUpdatedDir.append(UPDATED_DIR_SUFFIX.replace("/", ""));
  if (oldUpdatedDir.exists()) {
    oldUpdatedDir.remove(true);
  }

  
  let updateTestDir = getUpdateTestDir();
  try {
    removeDirRecursive(updateTestDir);
  }
  catch (e) {
    logTestInfo("unable to remove directory - path: " + updateTestDir.path +
                ", exception: " + e);
  }

  let updatesPatchDir = getUpdatesDir();
  updatesPatchDir.append("0");
  let mar = do_get_file("data/simple.mar");
  mar.copyTo(updatesPatchDir, FILE_UPDATE_ARCHIVE);

  reloadUpdateManagerData();
  gActiveUpdate = gUpdateManager.activeUpdate;
  do_check_true(!!gActiveUpdate);

  
  
  let updaterIni = processDir.clone();
  updaterIni.append(FILE_UPDATER_INI);
  if (updaterIni.exists()) {
    updaterIni.moveTo(processDir, FILE_UPDATER_INI_BAK);
  }

  let updateSettingsIni = processDir.clone();
  updateSettingsIni.append(UPDATE_SETTINGS_INI_FILE);
  writeFile(updateSettingsIni, UPDATE_SETTINGS_CONTENTS);

  
  AUS_Cc["@mozilla.org/updates/update-processor;1"].
    createInstance(AUS_Ci.nsIUpdateProcessor).
    processUpdate(gActiveUpdate);

  checkUpdateApplied();
}

function end_test() {
  
  let updateTestDir = getUpdateTestDir();
  try {
    logTestInfo("removing update test directory " + updateTestDir.path);
    removeDirRecursive(updateTestDir);
  }
  catch (e) {
    logTestInfo("unable to remove directory - path: " + updateTestDir.path +
                ", exception: " + e);
  }

  let processDir = getAppDir();
  
  let updaterIni = processDir.clone();
  updaterIni.append(FILE_UPDATER_INI_BAK);
  if (updaterIni.exists()) {
    updaterIni.moveTo(processDir, FILE_UPDATER_INI);
  }

  if (IS_UNIX) {
    
    getLaunchScript();
    if (IS_MACOSX) {
      
      getVersionScriptAndFile();
    }
  }

  cleanUp();
}

function shouldAdjustPathsOnMac() {
  
  
  let dir = getCurrentProcessDir();
  return (IS_MACOSX && dir.leafName != "MacOS");
}






function adjustPathsOnWindows() {
  
  
  let tmpDir = do_get_profile();
  tmpDir.append("ExecutableDir.tmp");
  tmpDir.createUnique(tmpDir.DIRECTORY_TYPE, 0755);
  let procDir = getCurrentProcessDir();
  procDir.copyTo(tmpDir, "bin");
  let newDir = tmpDir.clone();
  newDir.append("bin");
  gWindowsBinDir = newDir;
  logTestInfo("Using this new bin directory: " + gWindowsBinDir.path);
  
  

  
  
  
  let dirProvider = {
    getFile: function DP_getFile(prop, persistent) {
      persistent.value = true;
      if (prop == NS_GRE_DIR)
        return getAppDir();
      return null;
    },
    QueryInterface: function(iid) {
      if (iid.equals(AUS_Ci.nsIDirectoryServiceProvider) ||
          iid.equals(AUS_Ci.nsISupports))
        return this;
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  let ds = Services.dirsvc.QueryInterface(AUS_Ci.nsIDirectoryService);
  ds.QueryInterface(AUS_Ci.nsIProperties).undefine(NS_GRE_DIR);
  ds.registerProvider(dirProvider);
  do_register_cleanup(function() {
    ds.unregisterProvider(dirProvider);
  });
}








function getUpdateTestDir() {
  let updateTestDir = getAppDir();
  if (IS_MACOSX) {
    updateTestDir = updateTestDir.parent.parent;
  }
  updateTestDir.append("update_test");
  return updateTestDir;
}




function checkUpdateApplied() {
  
  if (gUpdateManager.activeUpdate.state != STATE_PENDING) {
    do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateApplied);
    return;
  }

  
  let status = readStatusFile();
  do_check_eq(status, STATE_PENDING);

  unlockDirectory(getAppDir());

  removeCallbackCopy();
}
