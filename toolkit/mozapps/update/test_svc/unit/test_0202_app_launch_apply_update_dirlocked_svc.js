



















const TEST_ID = "0202_svc";



const FILE_UPDATER_INI_BAK = "updater.ini.bak";


const CHECK_TIMEOUT_MILLI = 1000;


const MAX_TIMEOUT_RUNS = 300;

let gActiveUpdate;
let gTimeoutRuns = 0;

function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

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
                                    STATE_PENDING_SVC);
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
  writeStatusFile(STATE_PENDING_SVC);

  
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
  gActiveUpdate = gUpdateManager.activeUpdate;
  do_check_true(!!gActiveUpdate);

  setEnvironment();

  
  AUS_Cc["@mozilla.org/updates/update-processor;1"].
    createInstance(AUS_Ci.nsIUpdateProcessor).
    processUpdate(gActiveUpdate);

  resetEnvironment();

  checkUpdateApplied();
}

function end_test() {
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

  cleanUp();
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
      do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateApplied);
    return;
  }

  do_timeout(CHECK_TIMEOUT_MILLI, finishTest);
}

function finishTest() {
  
  let status = readStatusFile();
  do_check_eq(status, STATE_PENDING);

  unlockDirectory(getAppDir());

  removeCallbackCopy();
}
