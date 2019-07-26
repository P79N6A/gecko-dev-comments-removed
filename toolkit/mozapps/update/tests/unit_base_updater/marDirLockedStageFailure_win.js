



















function run_test() {
  if (APP_BIN_NAME == "xulrunner") {
    logTestInfo("Unable to run this test on xulrunner");
    return;
  }

  setupTestCommon(false);

  do_register_cleanup(end_test);

  logTestInfo("setting up environment for the update test...");

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
  let mar = getTestDirFile(FILE_SIMPLE_MAR);
  mar.copyTo(updatesPatchDir, FILE_UPDATE_ARCHIVE);

  
  
  let updaterIni = processDir.clone();
  updaterIni.append(FILE_UPDATER_INI);
  if (updaterIni.exists()) {
    updaterIni.moveTo(processDir, FILE_UPDATER_INI_BAK);
  }

  
  let updateSettingsIni = processDir.clone();
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  if (updateSettingsIni.exists()) {
    updateSettingsIni.moveTo(processDir, FILE_UPDATE_SETTINGS_INI_BAK);
  }
  updateSettingsIni = processDir.clone();
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  writeFile(updateSettingsIni, UPDATE_SETTINGS_CONTENTS);

  reloadUpdateManagerData();
  do_check_true(!!gUpdateManager.activeUpdate);

  Services.obs.addObserver(gUpdateStagedObserver, "update-staged", false);

  setEnvironment();

  
  logTestInfo("update preparation completed - calling processUpdate");
  AUS_Cc["@mozilla.org/updates/update-processor;1"].
    createInstance(AUS_Ci.nsIUpdateProcessor).
    processUpdate(gUpdateManager.activeUpdate);

  resetEnvironment();

  logTestInfo("processUpdate completed - waiting for update-staged notification");
}

function end_test() {
  logTestInfo("start - test cleanup");
  resetEnvironment();

  let processDir = getAppDir();
  
  let updaterIni = processDir.clone();
  updaterIni.append(FILE_UPDATER_INI_BAK);
  if (updaterIni.exists()) {
    updaterIni.moveTo(processDir, FILE_UPDATER_INI);
  }

  
  let updateSettingsIni = processDir.clone();
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI_BAK);
  if (updateSettingsIni.exists()) {
    updateSettingsIni.moveTo(processDir, FILE_UPDATE_SETTINGS_INI);
  }

  
  let updateTestDir = getUpdateTestDir();
  try {
    logTestInfo("removing update test directory " + updateTestDir.path);
    removeDirRecursive(updateTestDir);
  }
  catch (e) {
    logTestInfo("unable to remove directory - path: " + updateTestDir.path +
                ", exception: " + e);
  }

  if (IS_UNIX) {
    
    getLaunchScript();
  }

  cleanupTestCommon();
  logTestInfo("finish - test cleanup");
}

function shouldAdjustPathsOnMac() {
  
  
  let dir = getCurrentProcessDir();
  return (IS_MACOSX && dir.leafName != "MacOS");
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
    if (++gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whist waiting for pending state to finish");
    else
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateApplied);
    return;
  }

  do_timeout(TEST_CHECK_TIMEOUT, finishTest);
}

function finishTest() {
  
  let status = readStatusFile();
  do_check_eq(status, STATE_PENDING);

  unlockDirectory(getAppDir());

  removeCallbackCopy();
}
