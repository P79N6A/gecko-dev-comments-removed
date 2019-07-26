



















const TEST_ID = "0203";



const FILE_UPDATER_INI_BAK = "updater.ini.bak";


const CHECK_TIMEOUT_MILLI = 1000;


const MAX_TIMEOUT_RUNS = 300;



const APP_TIMER_TIMEOUT = 120000;

Components.utils.import("resource://gre/modules/ctypes.jsm");

let gAppTimer;
let gProcess;
let gActiveUpdate;
let gTimeoutRuns = 0;



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
  if (APP_BIN_NAME == "xulrunner") {
    logTestInfo("Unable to run this test on xulrunner");
    return;
  }

  do_test_pending();
  do_register_cleanup(end_test);

  if (IS_WIN) {
    Services.prefs.setBoolPref(PREF_APP_UPDATE_SERVICE_ENABLED, true);
  }

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

function switchApp() {
  let launchBin = getLaunchBin();
  let args = getProcessArgs();
  logTestInfo("launching " + launchBin.path + " " + args.join(" "));

  
  const LPCWSTR = ctypes.jschar.ptr;
  const DWORD = ctypes.uint32_t;
  const LPVOID = ctypes.voidptr_t;
  const GENERIC_READ = 0x80000000;
  const FILE_SHARE_READ = 1;
  const FILE_SHARE_WRITE = 2;
  const OPEN_EXISTING = 3;
  const FILE_FLAG_BACKUP_SEMANTICS = 0x02000000;
  const INVALID_HANDLE_VALUE = LPVOID(0xffffffff);
  let kernel32 = ctypes.open("kernel32");
  let CreateFile = kernel32.declare("CreateFileW", ctypes.default_abi,
                                    LPVOID, LPCWSTR, DWORD, DWORD,
                                    LPVOID, DWORD, DWORD, LPVOID);
  logTestInfo(gWindowsBinDir.path);
  let handle = CreateFile(gWindowsBinDir.path, GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, LPVOID(0),
                          OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, LPVOID(0));
  do_check_neq(handle.toString(), INVALID_HANDLE_VALUE.toString());
  kernel32.close();

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

  let processDir = getAppDir();
  
  let updaterIni = processDir.clone();
  updaterIni.append(FILE_UPDATER_INI_BAK);
  if (updaterIni.exists()) {
    updaterIni.moveTo(processDir, FILE_UPDATER_INI);
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

function shouldAdjustPathsOnMac() {
  
  
  let dir = getCurrentProcessDir();
  return (IS_MACOSX && dir.leafName != "MacOS");
}






function adjustPathsOnWindows() {
  
  
  let tmpDir = do_get_profile();
  tmpDir.append("ExecutableDir.tmp");
  tmpDir.createUnique(tmpDir.DIRECTORY_TYPE, 0755);
  let procDir = getCurrentProcessDir();
  copyDirRecursive(procDir, tmpDir, "bin");
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
  gTimeoutRuns++;
  
  if (gUpdateManager.activeUpdate.state != STATE_APPLIED_PLATFORM) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
      do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting for update to be " +
               "applied, current state is: " + gUpdateManager.activeUpdate.state);
    else
      do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateApplied);
    return;
  }

  let updatedDir = getAppDir();
  if (IS_MACOSX) {
    updatedDir = updatedDir.parent.parent;
  }
  updatedDir.append(UPDATED_DIR_SUFFIX.replace("/", ""));
  logTestInfo("testing " + updatedDir.path + " should exist");
  do_check_true(updatedDir.exists());

  let log;
  if (IS_WIN) {
    log = getUpdatesDir();
  } else {
    log = updatedDir.clone();
    if (IS_MACOSX) {
      log.append("Contents");
      log.append("MacOS");
    }
    log.append("updates");
  }
  log.append(FILE_LAST_LOG);
  if (!log.exists()) {
    do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateApplied);
    return;
  }

  
  let status = readStatusFile();
  do_check_eq(status, STATE_APPLIED_PLATFORM);

  
  
  if (IS_WIN) {
    writeStatusFile(STATE_APPLIED);
    status = readStatusFile();
    do_check_eq(status, STATE_APPLIED);
  }

  
  
  let contents = readFile(log);
  logTestInfo("contents of " + log.path + ":\n" +
              contents.replace(/\r\n/g, "\n"));

  let updateTestDir = getUpdateTestDir();
  logTestInfo("testing " + updateTestDir.path + " shouldn't exist");
  do_check_false(updateTestDir.exists());

  updateTestDir = updatedDir.clone();
  updateTestDir.append("update_test");
  let file = updateTestDir.clone();
  file.append("UpdateTestRemoveFile");
  logTestInfo("testing " + file.path + " shouldn't exist");
  do_check_false(file.exists());

  file = updateTestDir.clone();
  file.append("UpdateTestAddFile");
  logTestInfo("testing " + file.path + " should exist");
  do_check_true(file.exists());
  do_check_eq(readFileBytes(file), "UpdateTestAddFile\n");

  file = updateTestDir.clone();
  file.append("removed-files");
  logTestInfo("testing " + file.path + " should exist");
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
  if (IS_WIN) {
    
    
    logTestInfo("testing " + log.path + " should exist");
    do_check_true(log.exists());
  } else {
    logTestInfo("testing " + log.path + " shouldn't exist");
    do_check_false(log.exists());
  }

  log = updatesDir.clone();
  log.append(FILE_BACKUP_LOG);
  logTestInfo("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  updatesDir = updatedDir.clone();
  if (IS_MACOSX) {
    updatesDir.append("Contents");
    updatesDir.append("MacOS");
  }
  updatesDir.append("updates");
  log = updatesDir.clone();
  log.append("0");
  log.append(FILE_UPDATE_LOG);
  logTestInfo("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  if (!IS_WIN) {
    log = updatesDir.clone();
    log.append(FILE_LAST_LOG);
    logTestInfo("testing " + log.path + " should exist");
    do_check_true(log.exists());
  }

  log = updatesDir.clone();
  log.append(FILE_BACKUP_LOG);
  logTestInfo("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  updatesDir.append("0");
  logTestInfo("testing " + updatesDir.path + " shouldn't exist");
  do_check_false(updatesDir.exists());

  
  do_timeout(CHECK_TIMEOUT_MILLI, switchApp);
}





function checkUpdateFinished() {
  
  gTimeoutRuns++;
  try {
    let status = readStatusFile();
    if (status != STATE_SUCCEEDED) {
      if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
        do_throw("Exceeded MAX_TIMEOUT_RUNS whilst waiting for state to " +
                 "change to succeeded, current status: " + status);
      else
        do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateFinished);
      return;
    }
  } catch (e) {
    
  }

  try {
    
    getAppConsoleLogPath();
  } catch (e) {
    if (e.result == Components.results.NS_ERROR_FILE_IS_LOCKED) {
      
      
      if (gTimeoutRuns > MAX_TIMEOUT_RUNS)
        do_throw("Exceeded whilst waiting for file to be unlocked");
      else
        do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateFinished);
      return;
    } else {
      do_throw("getAppConsoleLogPath threw: " + e);
    }
  }

  

  let updatedDir = getAppDir();
  if (IS_MACOSX) {
    updatedDir = updatedDir.parent.parent;
  }
  updatedDir.append(UPDATED_DIR_SUFFIX.replace("/", ""));
  logTestInfo("testing " + updatedDir.path + " shouldn't exist");
  if (updatedDir.exists()) {
    do_timeout(CHECK_TIMEOUT_MILLI, checkUpdateFinished);
    return;
  }

  let updateTestDir = getUpdateTestDir();

  let file = updateTestDir.clone();
  file.append("UpdateTestRemoveFile");
  logTestInfo("testing " + file.path + " shouldn't exist");
  do_check_false(file.exists());

  file = updateTestDir.clone();
  file.append("UpdateTestAddFile");
  logTestInfo("testing " + file.path + " should exist");
  do_check_true(file.exists());
  do_check_eq(readFileBytes(file), "UpdateTestAddFile\n");

  file = updateTestDir.clone();
  file.append("removed-files");
  logTestInfo("testing " + file.path + " should exist");
  do_check_true(file.exists());
  do_check_eq(readFileBytes(file), "update_test/UpdateTestRemoveFile\n");

  let updatesDir = getUpdatesDir();
  log = updatesDir.clone();
  log.append("0");
  log.append(FILE_UPDATE_LOG);
  if (IS_WIN) {
    
    
    logTestInfo("testing " + log.path + " should exist");
    do_check_true(log.exists());
  } else {
    logTestInfo("testing " + log.path + " shouldn't exist");
    do_check_false(log.exists());
  }

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

  waitForFilesInUse();
}
