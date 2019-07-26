










const TEST_ID = "0200";



const FILE_UPDATER_INI_BAK = "updater.ini.bak";


const CHECK_TIMEOUT_MILLI = 1000;


const MAX_TIMEOUT_RUNS = 300;



const APP_TIMER_TIMEOUT = 20000;

let gAppTimer;
let gProcess;
let gTimeoutRuns = 0;

function run_test() {
  if (APP_BIN_NAME == "xulrunner") {
    logTestInfo("Unable to run this test on xulrunner");
    return;
  }

  do_test_pending();
  do_register_cleanup(end_test);

  removeUpdateDirsAndFiles();

  if (!gAppBinPath) {
    do_throw("Main application binary not found... expected: " +
             APP_BIN_NAME + APP_BIN_SUFFIX);
    return;
  }

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
  let mar = do_get_file("data/simple.mar");
  mar.copyTo(updatesPatchDir, FILE_UPDATE_ARCHIVE);

  
  let updaterIni = processDir.clone();
  updaterIni.append(FILE_UPDATER_INI);
  updaterIni.moveTo(processDir, FILE_UPDATER_INI_BAK);
  
  
  let updaterIniContents = "[Strings]\n" +
                           "Title=Update Test\n" +
                           "Info=Application Update XPCShell Test - " +
                           "test_0200_general.js\n";
  updaterIni = processDir.clone();
  updaterIni.append(FILE_UPDATER_INI);
  writeFile(updaterIni, updaterIniContents);

  let updateSettingsIni = processDir.clone();
  updateSettingsIni.append(UPDATE_SETTINGS_INI_FILE);
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
  updaterIni.moveTo(processDir, FILE_UPDATER_INI);

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

  
  getAppConsoleLogPath();

  if (IS_UNIX) {
    
    getLaunchScript();
    if (IS_MACOSX) {
      
      getVersionScriptAndFile();
    }
  }

  cleanUp();
}




let gProcessObserver = {
  observe: function PO_observe(subject, topic, data) {
    logTestInfo("topic " + topic + ", process exitValue " + gProcess.exitValue);
    if (gAppTimer) {
      gAppTimer.cancel();
      gAppTimer = null;
    }
    if (topic != "process-finished" || gProcess.exitValue != 0) {
      do_throw("Failed to launch application");
    }
    do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateFinished);
  },
  QueryInterface: XPCOMUtils.generateQI([AUS_Ci.nsIObserver])
};




let gTimerCallback = {
  notify: function TC_notify(aTimer) {
    gAppTimer = null;
    if (gProcess.isRunning) {
      gProcess.kill();
    }
    do_throw("launch application timer expired");
  },
  QueryInterface: XPCOMUtils.generateQI([AUS_Ci.nsITimerCallback])
};








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
  
  let log = getUpdatesDir();
  log.append("0");
  log.append(FILE_UPDATE_LOG);
  if (!log.exists()) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting for updates log to be created at " + log.path);
    else
      do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateFinished);
    return;
  }

  
  let status = readStatusFile();
  if (status == STATE_PENDING || status == STATE_APPLYING) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting for updates status to not be pending or applying, current status is: " + status);
    else
      do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateFinished);
    return;
  }

  
  
  
  let contents = readFile(log);
  logTestInfo("contents of " + log.path + ":\n" +  
              contents.replace(/\r\n/g, "\n"));

  if (IS_WIN && contents.indexOf("NS_main: file in use") != -1) {
    do_throw("the application can't be in use when running this test");
  }

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
