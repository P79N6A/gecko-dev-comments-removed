










let gAppTimer;
let gProcess;

function run_test() {
  if (APP_BIN_NAME == "xulrunner") {
    logTestInfo("Unable to run this test on xulrunner");
    return;
  }

  setupTestCommon(false);

  do_register_cleanup(end_test);

  if (!gAppBinPath) {
    do_throw("Main application binary not found... expected: " +
             APP_BIN_NAME + APP_BIN_SUFFIX);
    return;
  }

  gEnvSKipUpdateDirHashing = true;
  let channel = Services.prefs.getCharPref(PREF_APP_UPDATE_CHANNEL);
  let patches = getLocalPatchString(null, null, null, null, null, "true",
                                    STATE_PENDING);
  let updates = getLocalUpdateString(patches, null, null, null, null, null,
                                     null, null, null, null, null, null,
                                     null, "true", channel);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);

  
  let processDir = getCurrentProcessDir();
  let file = processDir.clone();
  file.append("application.ini");
  let ini = AUS_Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
            getService(AUS_Ci.nsIINIParserFactory).
            createINIParser(file);
  let version = ini.getString("App", "Version");
  writeVersionFile(version);
  writeStatusFile(STATE_PENDING);

  
  let updateTestDir = getUpdateTestDir();
  try {
    removeDirRecursive(updateTestDir);
  }
  catch (e) {
    logTestInfo("unable to remove directory - path: " + updateTestDir.path +
                ", exception: " + e);
  }

  
  
  if (!updateTestDir.exists()) {
    updateTestDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
  }
  logTestInfo("update test directory path: " + updateTestDir.path);

  file = updateTestDir.clone();
  file.append("UpdateTestRemoveFile");
  writeFile(file, "ToBeRemoved");

  file = updateTestDir.clone();
  file.append("UpdateTestAddFile");
  writeFile(file, "ToBeReplaced");

  file = updateTestDir.clone();
  file.append("removed-files");
  writeFile(file, "ToBeReplaced");

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

  let launchBin = getLaunchBin();
  let args = getProcessArgs();
  logTestInfo("launching " + launchBin.path + " " + args.join(" "));

  gProcess = AUS_Cc["@mozilla.org/process/util;1"].
                createInstance(AUS_Ci.nsIProcess);
  gProcess.init(launchBin);

  gAppTimer = AUS_Cc["@mozilla.org/timer;1"].createInstance(AUS_Ci.nsITimer);
  gAppTimer.initWithCallback(gTimerCallback, APP_TIMER_TIMEOUT,
                             AUS_Ci.nsITimer.TYPE_ONE_SHOT);

  setEnvironment();

  gProcess.runAsync(args, args.length, gProcessObserver);

  resetEnvironment();
}

function end_test() {
  if (gProcess.isRunning) {
    logTestInfo("attempt to kill process");
    gProcess.kill();
  }

  if (gAppTimer) {
    logTestInfo("cancelling timer");
    gAppTimer.cancel();
    gAppTimer = null;
  }

  resetEnvironment();

  let processDir = getCurrentProcessDir();
  
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

  if (IS_WIN) {
    
    
    let appBinCopy = processDir.clone();
    appBinCopy.append(FILE_WIN_TEST_EXE);
    if (appBinCopy.exists()) {
      appBinCopy.remove(false);
    }
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

  
  try {
    getAppConsoleLogPath();
  }
  catch (e) {
    logTestInfo("unable to remove file during end_test. Exception: " + e);
  }

  if (IS_UNIX) {
    
    getLaunchScript();
  }

  cleanupTestCommon();
}








function getUpdateTestDir() {
  let updateTestDir = getCurrentProcessDir();
  if (IS_MACOSX) {
    updateTestDir = updateTestDir.parent.parent;
  }
  updateTestDir.append("update_test");
  return updateTestDir;
}





function checkUpdateFinished() {
  gTimeoutRuns++;
  
  let updatesDir = getUpdatesDir();
  updatesDir.append("0");
  let log = updatesDir.clone();
  log.append(FILE_UPDATE_LOG);
  if (!log.exists()) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting for updates log to " +
               "be created at " + log.path);
    else
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
    return;
  }

  
  let status = readStatusFile();
  if (status == STATE_PENDING || status == STATE_APPLYING) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting for updates status " +
               "to not be pending or applying, current status is: " + status);
    else
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
    return;
  }

  
  
  try {
    log.moveTo(updatesDir, FILE_UPDATE_LOG + ".bak");
  }
  catch (e) {
    logTestInfo("unable to rename file " + FILE_UPDATE_LOG + " to " +
                FILE_UPDATE_LOG + ".bak. Exception: " + e);
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting to be able to " +
               "rename " + FILE_UPDATE_LOG + " to " + FILE_UPDATE_LOG +
               ".bak. Path: " + log.path);
    else
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
    return;
  }

  restoreLogFile();
}





function restoreLogFile() {
  gTimeoutRuns++;
  
  
  let updatesDir = getUpdatesDir();
  updatesDir.append("0");
  let log = updatesDir.clone();
  log.append(FILE_UPDATE_LOG + ".bak");
  try {
    log.moveTo(updatesDir, FILE_UPDATE_LOG);
  }
  catch (e) {
    logTestInfo("unable to rename file " + FILE_UPDATE_LOG + ".bak back to " +
                FILE_UPDATE_LOG + ". Exception: " + e);
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting to be able to " +
               "rename " + FILE_UPDATE_LOG + ".bak back to " + FILE_UPDATE_LOG +
               ". Path: " + log.path);
    else
      do_timeout(TEST_CHECK_TIMEOUT, restoreLogFile);
    return;
  }

  
  
  
  let contents = readFile(log);
  logTestInfo("contents of " + log.path + ":\n" +  
              contents.replace(/\r\n/g, "\n"));

  if (IS_WIN && contents.indexOf("NS_main: file in use") != -1) {
    do_throw("the application can't be in use when running this test");
  }

  let status = readStatusFile();
  do_check_eq(status, STATE_SUCCEEDED);

  standardInit();

  let update = gUpdateManager.getUpdateAt(0);
  do_check_eq(update.state, STATE_SUCCEEDED);

  let updateTestDir = getUpdateTestDir();

  let file = updateTestDir.clone();
  file.append("UpdateTestRemoveFile");
  do_check_false(file.exists());

  file = updateTestDir.clone();
  file.append("UpdateTestAddFile");
  do_check_true(file.exists());
  do_check_eq(readFileBytes(file), "UpdateTestAddFile\n");

  file = updateTestDir.clone();
  file.append("removed-files");
  do_check_true(file.exists());
  do_check_eq(readFileBytes(file), "update_test/UpdateTestRemoveFile\n");

  checkLogRenameFinished();
}





function checkLogRenameFinished() {
  gTimeoutRuns++;
  
  let log = getUpdatesDir();
  log.append("0");
  log.append(FILE_UPDATE_LOG);
  if (log.exists()) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting for update log to " +
               "be renamed to last-update.log at " + log.path);
    else
      do_timeout(TEST_CHECK_TIMEOUT, checkLogRenameFinished);
    return;
  }

  let updatesDir = getUpdatesDir();
  log = updatesDir.clone();
  log.append("0");
  log.append(FILE_UPDATE_LOG);
  logTestInfo("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  log = updatesDir.clone();
  log.append(FILE_LAST_LOG);
  logTestInfo("testing " + log.path + " should exist");
  do_check_true(log.exists());

  log = updatesDir.clone();
  log.append(FILE_BACKUP_LOG);
  logTestInfo("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  updatesDir.append("0");
  logTestInfo("testing " + updatesDir.path + " should exist");
  do_check_true(updatesDir.exists());

  removeCallbackCopy();
}
