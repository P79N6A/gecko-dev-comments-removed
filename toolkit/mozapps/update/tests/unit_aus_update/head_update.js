



const INSTALL_LOCALE = "@AB_CD@";
const APP_BIN_NAME = "@MOZ_APP_NAME@";
const BIN_SUFFIX = "@BIN_SUFFIX@";

const APP_INFO_NAME = "XPCShell";
const APP_INFO_VENDOR = "Mozilla";

#ifdef XP_UNIX
const APP_BIN_SUFFIX = "-bin";
#else
const APP_BIN_SUFFIX = "@BIN_SUFFIX@";
#endif

#ifdef XP_WIN
const IS_WIN = true;
#else
const IS_WIN = false;
#endif

#ifdef XP_OS2
const IS_OS2 = true;
#else
const IS_OS2 = false;
#endif

#ifdef XP_MACOSX
const IS_MACOSX = true;
#ifdef MOZ_SHARK
const IS_SHARK = true;
#else
const IS_SHARK = false;
#endif
#else
const IS_MACOSX = false;
#endif

#ifdef XP_UNIX
const IS_UNIX = true;
#else
const IS_UNIX = false;
#endif

#ifdef ANDROID
const IS_ANDROID = true;
#else
const IS_ANDROID = false;
#endif

#ifdef MOZ_WIDGET_GONK
const IS_TOOLKIT_GONK = true;
#else
const IS_TOOLKIT_GONK = false;
#endif

const USE_EXECV = IS_UNIX && !IS_MACOSX;

#ifdef MOZ_VERIFY_MAR_SIGNATURE
const IS_MAR_CHECKS_ENABLED = true;
#else
const IS_MAR_CHECKS_ENABLED = false;
#endif

const URL_HOST = "http://localhost";

const APPLY_TO_DIR_SUFFIX = "_applyToDir/";
const UPDATES_DIR_SUFFIX = "_mar";
#ifdef XP_MACOSX
const UPDATED_DIR_SUFFIX = "Updated.app/";
#else
const UPDATED_DIR_SUFFIX = "updated/";
#endif

const FILE_COMPLETE_MAR = "complete.mar";
const FILE_COMPLETE_WIN_MAR = "complete_win.mar";
const FILE_HELPER_BIN = "TestAUSHelper" + BIN_SUFFIX;
const FILE_MAINTENANCE_SERVICE_BIN = "maintenanceservice.exe";
const FILE_MAINTENANCE_SERVICE_INSTALLER_BIN = "maintenanceservice_installer.exe";
const FILE_OLD_VERSION_MAR = "old_version.mar";
const FILE_PARTIAL_MAR = "partial.mar";
const FILE_PARTIAL_WIN_MAR = "partial_win.mar";
const FILE_UPDATER_BIN = "updater" + BIN_SUFFIX;
const FILE_UPDATER_INI_BAK = "updater.ini.bak";
const FILE_WRONG_CHANNEL_MAR = "wrong_product_channel.mar";

const LOG_COMPLETE_SUCCESS = "complete_log_success";
const LOG_COMPLETE_SWITCH_SUCCESS = "complete_log_switch_success"
const LOG_COMPLETE_CC_SUCCESS = "complete_cc_log_success";
const LOG_COMPLETE_CC_SWITCH_SUCCESS = "complete_cc_log_switch_success";

const LOG_PARTIAL_SUCCESS = "partial_log_success";
const LOG_PARTIAL_SWITCH_SUCCESS = "partial_log_switch_success";
const LOG_PARTIAL_FAILURE = "partial_log_failure";

const ERR_CALLBACK_FILE_IN_USE = "NS_main: file in use - failed to " +
                                 "exclusively open executable file:"

const ERR_RENAME_FILE = "rename_file: failed to rename file";
const ERR_UNABLE_OPEN_DEST = "unable to open destination file";
const ERR_BACKUP_DISCARD = "backup_discard: unable to remove";

const LOG_SVC_SUCCESSFUL_LAUNCH = "Process was started... waiting on result.";




const MAC_MAX_TIME_DIFFERENCE = 60000;


const TEST_HELPER_TIMEOUT = 100;


const TEST_CHECK_TIMEOUT = 100;


const MAX_TIMEOUT_RUNS = 1000;



const APP_TIMER_TIMEOUT = 120000;



const FILE_WIN_TEST_EXE = "_aus_test_app.exe";


var gURLData = URL_HOST + "/";

var gTestID;

var gTestserver;

var gXHR;
var gXHRCallback;

var gUpdatePrompt;
var gUpdatePromptCallback;

var gCheckFunc;
var gResponseBody;
var gResponseStatusCode = 200;
var gRequestURL;
var gUpdateCount;
var gUpdates;
var gStatusCode;
var gStatusText;
var gStatusResult;


var gCallbackBinFile = "callback_app" + BIN_SUFFIX;
var gCallbackArgs = ["./", "callback.log", "Test Arg 2", "Test Arg 3"];
var gBackgroundUpdate = false;
var gSwitchApp = false;
var gDisableReplaceFallback = false;

var gTimeoutRuns = 0;
























































var TEST_DIRS = [
{
  relPathDir   : "a/b/3/",
  dirRemoved   : false,
  files        : ["3text0", "3text1"],
  filesRemoved : true
}, {
  relPathDir   : "a/b/4/",
  dirRemoved   : true,
  files        : ["4text0", "4text1"],
  filesRemoved : true
}, {
  relPathDir   : "a/b/5/",
  dirRemoved   : true,
  files        : ["5test.exe", "5text0", "5text1"],
  filesRemoved : true
}, {
  relPathDir   : "a/b/6/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/7/",
  dirRemoved   : true,
  files        : ["7text0", "7text1"],
  subDirs      : ["70/", "71/"],
  subDirFiles  : ["7xtest.exe", "7xtext0", "7xtext1"]
}, {
  relPathDir   : "a/b/8/",
  dirRemoved   : false
}, {
  relPathDir   : "a/b/8/80/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/8/81/",
  dirRemoved   : false,
  files        : ["81text0", "81text1"]
}, {
  relPathDir   : "a/b/8/82/",
  dirRemoved   : false,
  subDirs      : ["820/", "821/"]
}, {
  relPathDir   : "a/b/8/83/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/8/84/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/8/85/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/8/86/",
  dirRemoved   : true,
  files        : ["86text0", "86text1"]
}, {
  relPathDir   : "a/b/8/87/",
  dirRemoved   : true,
  subDirs      : ["870/", "871/"],
  subDirFiles  : ["87xtext0", "87xtext1"]
}, {
  relPathDir   : "a/b/8/88/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/8/89/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/9/90/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/9/91/",
  dirRemoved   : false,
  files        : ["91text0", "91text1"]
}, {
  relPathDir   : "a/b/9/92/",
  dirRemoved   : false,
  subDirs      : ["920/", "921/"]
}, {
  relPathDir   : "a/b/9/93/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/9/94/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/9/95/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/9/96/",
  dirRemoved   : true,
  files        : ["96text0", "96text1"]
}, {
  relPathDir   : "a/b/9/97/",
  dirRemoved   : true,
  subDirs      : ["970/", "971/"],
  subDirFiles  : ["97xtext0", "97xtext1"]
}, {
  relPathDir   : "a/b/9/98/",
  dirRemoved   : true
}, {
  relPathDir   : "a/b/9/99/",
  dirRemoved   : true
}];


var ADDITIONAL_TEST_DIRS = [];




var DEBUG_AUS_TEST = true;

#include ../shared.js

#ifdef MOZ_MAINTENANCE_SERVICE
const STATE_APPLIED_PLATFORM = STATE_APPLIED_SVC;
#else
const STATE_APPLIED_PLATFORM = STATE_APPLIED;
#endif



if (APP_BIN_NAME == "xulrunner") {
  try {
    gDefaultPrefBranch.getCharPref(PREF_APP_UPDATE_CHANNEL);
  }
  catch (e) {
    setUpdateChannel("test_channel");
  }
}

function setupTestCommon(aAdjustGeneralPaths) {
  do_test_pending();

  if (gTestID) {
    do_throw("should only be called once!");
  }

  let caller = Components.stack.caller;
  gTestID = caller.filename.toString().split("/").pop().split(".")[0];

  if (aAdjustGeneralPaths) {
     
     adjustGeneralPaths();
  }

  removeUpdateDirsAndFiles();
}




function cleanupTestCommon() {
  logTestInfo("start - general test cleanup");
  removeUpdateDirsAndFiles();

  
  
  reloadUpdateManagerData();

  if (gChannel) {
    gPrefRoot.removeObserver(PREF_APP_UPDATE_CHANNEL, observer);
  }

  
  
  
  
  
  gAUS.observe(null, "xpcom-shutdown", "");

  if (gXHR) {
    gXHRCallback     = null;

    gXHR.responseXML = null;
    
    gXHR.onerror     = null;
    gXHR.onload      = null;
    gXHR.onprogress  = null;

    gXHR             = null;
  }

  gTestserver = null;
  logTestInfo("finish - general test cleanup");
}




function setDefaultPrefs() {
  Services.prefs.setBoolPref(PREF_APP_UPDATE_ENABLED, true);
  Services.prefs.setBoolPref(PREF_APP_UPDATE_METRO_ENABLED, true);
  
  
  Services.prefs.setBoolPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI, false);
  
  Services.prefs.setBoolPref(PREF_APP_UPDATE_LOG, true);
}





function standardInit() {
  createAppInfo("xpcshell@tests.mozilla.org", APP_INFO_NAME, "1.0", "2.0");
  setDefaultPrefs();
  
  initUpdateServiceStub();
}


function pathHandler(metadata, response) {
  response.setHeader("Content-Type", "text/xml", false);
  response.setStatusLine(metadata.httpVersion, gResponseStatusCode, "OK");
  response.bodyOutputStream.write(gResponseBody, gResponseBody.length);
}












function getApplyDirPath() {
  return gTestID + APPLY_TO_DIR_SUFFIX + "appdir/";
}













function getApplyDirFile(aRelPath, allowNonexistent) {
  let relpath = getApplyDirPath() + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, allowNonexistent);
}








function getTestDirPath() {
  return "../data/";
}







function getTestDirFile(aRelPath) {
  let relpath = getTestDirPath() + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, false);
}




function getUpdatedDirPath() {
  let suffix = "";
  if (gBackgroundUpdate) {
    suffix = UPDATED_DIR_SUFFIX;
  }
  return getApplyDirPath() + suffix;
}













function getTargetDirFile(aRelPath, allowNonexistent) {
  let relpath = getUpdatedDirPath() + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, allowNonexistent);
}

if (IS_WIN) {
  const kLockFileName = "updated.update_in_progress.lock";
  


  function lockDirectory(aDir) {
    var file = aDir.clone();
    file.append(kLockFileName);
    file.create(file.NORMAL_FILE_TYPE, 4 * 64 + 4 * 8 + 4); 
    file.QueryInterface(AUS_Ci.nsILocalFileWin);
    file.fileAttributesWin |= file.WFA_READONLY;
    file.fileAttributesWin &= ~file.WFA_READWRITE;
    logTestInfo("testing the successful creation of the lock file");
    do_check_true(file.exists());
    do_check_false(file.isWritable());
  }
  


  function unlockDirectory(aDir) {
    var file = aDir.clone();
    file.append(kLockFileName);
    file.QueryInterface(AUS_Ci.nsILocalFileWin);
    file.fileAttributesWin |= file.WFA_READWRITE;
    file.fileAttributesWin &= ~file.WFA_READONLY;
    logTestInfo("removing and testing the successful removal of the lock file");
    file.remove(false);
    do_check_false(file.exists());
  }
}












function copyMinimumAppFiles(aSrcDir, aDestDir, aDestLeafName) {
  let destDir = aDestDir.clone();
  destDir.append(aDestLeafName);
  if (!destDir.exists()) {
    try {
      destDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
    }
    catch (e) {
      logTestInfo("unable to create directory, path: " + destDir.path +
                  ", exception: " + e);
      do_throw(e);
    }
  }

  
  
  let fileLeafNames = [APP_BIN_NAME + APP_BIN_SUFFIX, FILE_UPDATER_BIN,
                       FILE_UPDATE_SETTINGS_INI, "application.ini",
                       "dependentlibs.list"];

  
  
  let deplibsFile = aSrcDir.clone();
  deplibsFile.append("dependentlibs.list");
  let istream = AUS_Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(AUS_Ci.nsIFileInputStream);
  istream.init(deplibsFile, 0x01, 4 * 64 + 4 * 8 + 4, 0); 
  istream.QueryInterface(AUS_Ci.nsILineInputStream);

  let hasMore;
  let line = {};
  do {
    hasMore = istream.readLine(line);
    fileLeafNames.push(line.value);
  } while(hasMore);

  istream.close();

  fileLeafNames.forEach(function CMAF_FLN_FE(aLeafName) {
    let srcFile = aSrcDir.clone();
    srcFile.append(aLeafName);
    try {
      srcFile.copyTo(destDir, aLeafName);
    }
    catch (e) {
      logTestInfo("unable to copy file, src path: " + srcFile.path +
                  ", dest path: " + destFile.path + ", exception: " + e);
      do_throw(e);
    }
  });
}







function runUpdate() {
  
  let binDir = getGREDir();
  let updater = binDir.clone();
  updater.append("updater.app");
  if (!updater.exists()) {
    updater = binDir.clone();
    updater.append(FILE_UPDATER_BIN);
    if (!updater.exists()) {
      do_throw("Unable to find updater binary!");
    }
  }

  let updatesDir = do_get_file(gTestID + UPDATES_DIR_SUFFIX, true);
  updater.copyTo(updatesDir, updater.leafName);
  let updateBin = updatesDir.clone();
  updateBin.append(updater.leafName);
  if (updateBin.leafName == "updater.app") {
    updateBin.append("Contents");
    updateBin.append("MacOS");
    updateBin.append("updater");
    if (!updateBin.exists())
      do_throw("Unable to find the updater executable!");
  }

  let updatesDirPath = updatesDir.path;
  if (/ /.test(updatesDirPath))
    updatesDirPath = '"' + updatesDirPath + '"';

  let applyToDir = getApplyDirFile();
  let applyToDirPath = applyToDir.path;
  if (gBackgroundUpdate || gSwitchApp) {
    applyToDirPath += "/" + UPDATED_DIR_SUFFIX;
  }
  if (IS_WIN) {
    
    applyToDirPath = applyToDirPath.replace(/\//g, "\\");
  }
  if (/ /.test(applyToDirPath))
    applyToDirPath = '"' + applyToDirPath + '"';

  let callbackApp = getApplyDirFile("a/b/" + gCallbackBinFile);
  callbackApp.permissions = PERMS_DIRECTORY;

  let cwdPath = callbackApp.parent.path;
  if (/ /.test(cwdPath))
    cwdPath = '"' + cwdPath + '"';

  let callbackAppPath = callbackApp.path;
  if (/ /.test(callbackAppPath))
    callbackAppPath = '"' + callbackAppPath + '"';

  
  let updateSettingsIni = getApplyDirFile(null, true);
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  if (updateSettingsIni.exists()) {
    updateSettingsIni.moveTo(updateSettingsIni.parent, FILE_UPDATE_SETTINGS_INI_BAK);
  }
  updateSettingsIni = getApplyDirFile(null, true);
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  writeFile(updateSettingsIni, UPDATE_SETTINGS_CONTENTS);

  let args = [updatesDirPath, applyToDirPath, 0];
  if (gBackgroundUpdate) {
    args[2] = -1;
  } else {
    if (gSwitchApp) {
      args[2] = "0/replace";
    }
    args = args.concat([cwdPath, callbackAppPath]);
    args = args.concat(gCallbackArgs);
  }
  logTestInfo("Running the updater: " + updateBin.path + " " + args.join(" "));

  let env = AUS_Cc["@mozilla.org/process/environment;1"].
            getService(AUS_Ci.nsIEnvironment);
  if (gDisableReplaceFallback) {
    env.set("MOZ_NO_REPLACE_FALLBACK", "1");
  }

  let process = AUS_Cc["@mozilla.org/process/util;1"].
                createInstance(AUS_Ci.nsIProcess);
  process.init(updateBin);
  process.run(true, args, args.length);

  if (gDisableReplaceFallback) {
    env.set("MOZ_NO_REPLACE_FALLBACK", "");
  }

  
  let updateSettingsIni = getApplyDirFile(null, true);
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI_BAK);
  if (updateSettingsIni.exists()) {
    updateSettingsIni.moveTo(updateSettingsIni.parent, FILE_UPDATE_SETTINGS_INI);
  }

  return process.exitValue;
}

let gServiceLaunchedCallbackLog = null;
let gServiceLaunchedCallbackArgs = null;







function shouldRunServiceTest(aFirstTest) {
  
  
  
  attemptServiceInstall();

  const REG_PATH = "SOFTWARE\\Mozilla\\MaintenanceService\\" +
                   "3932ecacee736d366d6436db0f55bce4";

  let key = AUS_Cc["@mozilla.org/windows-registry-key;1"].
            createInstance(AUS_Ci.nsIWindowsRegKey);
  try {
    key.open(AUS_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE, REG_PATH,
             AUS_Ci.nsIWindowsRegKey.ACCESS_READ | key.WOW64_64);
  }
  catch (e) {
    logTestInfo("this test can only run on the buildbot build system at this " +
                "time.");
    return false;
  }

  let binDir = getGREDir();
  let updaterBin = binDir.clone();
  updaterBin.append(FILE_UPDATER_BIN);
  if (!updaterBin.exists()) {
    do_throw("Unable to find updater binary!");
  }

  let updaterBinPath = updaterBin.path;
  if (/ /.test(updaterBinPath)) {
    updaterBinPath = '"' + updaterBinPath + '"';
  }

  
  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let args = ["wait-for-service-stop", "MozillaMaintenance", "10"];
  let process = AUS_Cc["@mozilla.org/process/util;1"].
                createInstance(AUS_Ci.nsIProcess);
  process.init(helperBin);
  logTestInfo("Checking if the service exists on this machine.");
  process.run(true, args, args.length);
  if (process.exitValue == 0xEE) {
    logTestInfo("this test can only run when the service is installed.");
    return false;
  } else {
    logTestInfo("Service exists, return value: " + process.exitValue);
  }

  
  
  
  if (aFirstTest && process.exitValue != 0) {
    do_throw("First test, check for service stopped state returned error " +
             process.exitValue);
  }

#ifdef DISABLE_UPDATER_AUTHENTICODE_CHECK
  
  return true;
#else
  
  args = ["check-signature", updaterBinPath];
  process = AUS_Cc["@mozilla.org/process/util;1"].
            createInstance(AUS_Ci.nsIProcess);
  process.init(helperBin);
  process.run(true, args, args.length);
  if (process.exitValue == 0) {
    return true;
  }
  logTestInfo("this test can only run on builds with signed binaries. " +
              FILE_HELPER_BIN + " returned " + process.exitValue)
  return false;
#endif
}







function copyBinToApplyToDir(filename) {
  let binDir = getGREDir();
  let fileToCopy = binDir.clone();
  fileToCopy.append(filename);
  if (!fileToCopy.exists()) {
    do_throw("Unable to copy binary: " + filename);
  }
  let applyToUpdater = getApplyDirFile(null, true);
  if (applyToUpdater.path != binDir.path) {
    do_print("copying " + fileToCopy.path + " to: " + applyToUpdater.path);
    fileToCopy.copyTo(applyToUpdater, filename);
  }
}







function attemptServiceInstall() {
  var version = AUS_Cc["@mozilla.org/system-info;1"]
                .getService(AUS_Ci.nsIPropertyBag2)
                .getProperty("version");
  var isVistaOrHigher = (parseFloat(version) >= 6.0);
  if (isVistaOrHigher) {
    return;
  }

  let binDir = getGREDir();
  let installerFile = binDir.clone();
  installerFile.append(FILE_MAINTENANCE_SERVICE_INSTALLER_BIN);
  if (!installerFile.exists()) {
    do_throw(FILE_MAINTENANCE_SERVICE_INSTALLER_BIN + " not found.");
  }
  let installerProcess = AUS_Cc["@mozilla.org/process/util;1"].
                         createInstance(AUS_Ci.nsIProcess);
  installerProcess.init(installerFile);
  logTestInfo("Starting installer process...");
  installerProcess.run(true, [], 0);
}











function runUpdateUsingService(aInitialStatus, aExpectedStatus,
                               aCallback, aUpdatesDir, aCheckSvcLog) {
  
  function checkServiceLogs(aOriginalContents) {
    let contents = readServiceLogFile();
    logTestInfo("The contents of maintenanceservice.log:\n" + contents + "\n");
    do_check_neq(contents, aOriginalContents);
    do_check_neq(contents.indexOf(LOG_SVC_SUCCESSFUL_LAUNCH), -1);
  }
  function readServiceLogFile() {
    let file = AUS_Cc["@mozilla.org/file/directory_service;1"].
               getService(AUS_Ci.nsIProperties).
               get("CmAppData", AUS_Ci.nsIFile);
    file.append("Mozilla");
    file.append("logs");
    file.append("maintenanceservice.log");
    return readFile(file);
  }
  function waitServiceApps() {
    
    waitForApplicationStop("maintenanceservice_installer.exe");
    
    waitForApplicationStop("maintenanceservice_tmp.exe");
    
    waitForApplicationStop("maintenanceservice.exe");
  }
  function waitForServiceStop(aFailTest) {
    waitServiceApps();
    logTestInfo("Waiting for service to stop if necessary...");
    
    
    let helperBin = getTestDirFile(FILE_HELPER_BIN);
    let helperBinArgs = ["wait-for-service-stop",
                         "MozillaMaintenance",
                         "120"];
    let helperBinProcess = AUS_Cc["@mozilla.org/process/util;1"].
                           createInstance(AUS_Ci.nsIProcess);
    helperBinProcess.init(helperBin);
    logTestInfo("Stopping service...");
    helperBinProcess.run(true, helperBinArgs, helperBinArgs.length);
    if (helperBinProcess.exitValue == 0xEE) {
      do_throw("The service does not exist on this machine.  Return value: " +
               helperBinProcess.exitValue);
    } else if (helperBinProcess.exitValue != 0) {
      if (aFailTest) {
        do_throw("maintenance service did not stop, last state: " +
                 helperBinProcess.exitValue + ". Forcing test failure.");
      } else {
        logTestInfo("maintenance service did not stop, last state: " +
                    helperBinProcess.exitValue + ".  May cause failures.");
      }
    } else {
      logTestInfo("Service stopped.");
    }
    waitServiceApps();
  }
  function waitForApplicationStop(application) {
    logTestInfo("Waiting for " + application + " to stop if " +
                "necessary...");
    
    
    let helperBin = getTestDirFile(FILE_HELPER_BIN);
    let helperBinArgs = ["wait-for-application-exit",
                         application,
                         "120"];
    let helperBinProcess = AUS_Cc["@mozilla.org/process/util;1"].
                           createInstance(AUS_Ci.nsIProcess);
    helperBinProcess.init(helperBin);
    helperBinProcess.run(true, helperBinArgs, helperBinArgs.length);
    if (helperBinProcess.exitValue != 0) {
      do_throw(application + " did not stop, last state: " +
               helperBinProcess.exitValue + ". Forcing test failure.");
    }
  }

  
  waitForServiceStop(true);

  
  if (typeof(gRegisteredServiceCleanup) === "undefined") {
    gRegisteredServiceCleanup = true;

    do_register_cleanup(function serviceCleanup() {
      resetEnvironment();

      
      try {
        getAppConsoleLogPath();
      }
      catch (e) {
        logTestInfo("unable to remove file during cleanup. Exception: " + e);
      }

      
      try {
        getAppArgsLogPath();
      }
      catch (e) {
        logTestInfo("unable to remove file during cleanup. Exception: " + e);
      }
    });
  }

  if (aCheckSvcLog === undefined) {
    aCheckSvcLog = true; 
  }

  let svcOriginalLog;
  if (aCheckSvcLog) {
    svcOriginalLog = readServiceLogFile();
  }

  let appArgsLogPath = getAppArgsLogPath();
  gServiceLaunchedCallbackLog = appArgsLogPath.replace(/^"|"$/g, "");

  let updatesDir = aUpdatesDir || do_get_file(gTestID + UPDATES_DIR_SUFFIX);
  let file = updatesDir.clone();
  file.append(FILE_UPDATE_STATUS);
  writeFile(file, aInitialStatus + "\n");

  
  do_check_eq(readStatusFile(updatesDir), aInitialStatus);

  file = updatesDir.clone();
  file.append(FILE_UPDATE_VERSION);
  writeFile(file, DEFAULT_UPDATE_VERSION + "\n");

  gServiceLaunchedCallbackArgs = [
    "-no-remote",
    "-process-updates",
    "-dump-args",
    appArgsLogPath
  ];

  let launchBin = getLaunchBin();
  let args = getProcessArgs(["-dump-args", appArgsLogPath]);
  logTestInfo("launching " + launchBin.path + " " + args.join(" "));

  let process = AUS_Cc["@mozilla.org/process/util;1"].
                   createInstance(AUS_Ci.nsIProcess);
  process.init(launchBin);

  
  gEnvUpdateRootOverride = updatesDir.path;
  gEnvAppDirOverride = getApplyDirFile(null).path;
  gEnvSKipUpdateDirHashing = true;

  if (gSwitchApp) {
    
    gShouldResetEnv = undefined;
  }

  setEnvironment();

  
  
  
  copyBinToApplyToDir(FILE_UPDATER_BIN);

  
  
  
  
  copyBinToApplyToDir(FILE_MAINTENANCE_SERVICE_BIN);
  copyBinToApplyToDir(FILE_MAINTENANCE_SERVICE_INSTALLER_BIN);

  
  let updateSettingsIni = getApplyDirFile(null, true);
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  if (updateSettingsIni.exists()) {
    updateSettingsIni.moveTo(updateSettingsIni.parent, FILE_UPDATE_SETTINGS_INI_BAK);
  }
  updateSettingsIni = getApplyDirFile(null, true);
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  writeFile(updateSettingsIni, UPDATE_SETTINGS_CONTENTS);

  
  
  
  
  
  process.run(true, args, args.length);

  resetEnvironment();

  function timerCallback(timer) {
    
    let status = readStatusFile(updatesDir);
    
    if (aExpectedStatus == STATE_FAILED) {
      status = status.split(": ")[0];
    }
    
    
    
    if (status == STATE_APPLYING ||
        status == STATE_PENDING_SVC) {
      logTestInfo("Still waiting to see the " + aExpectedStatus +
                  " status, got " + status + " for now...");
      return;
    }

    
    waitForServiceStop(false);

    
    let updateSettingsIni = getApplyDirFile(null, true);
    updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI_BAK);
    if (updateSettingsIni.exists()) {
      updateSettingsIni.moveTo(updateSettingsIni.parent, FILE_UPDATE_SETTINGS_INI);
    }

    do_check_eq(status, aExpectedStatus);

    timer.cancel();
    timer = null;

    if (aCheckSvcLog) {
      checkServiceLogs(svcOriginalLog);
    }
    aCallback();
  }

  let timer = AUS_Cc["@mozilla.org/timer;1"].createInstance(AUS_Ci.nsITimer);
  timer.initWithCallback(timerCallback, 1000, timer.TYPE_REPEATING_SLACK);
}








function getLaunchBin() {
  let launchBin;
  if (IS_WIN) {
    launchBin = Services.dirsvc.get("WinD", AUS_Ci.nsIFile);
    launchBin.append("System32");
    launchBin.append("cmd.exe");
  }
  else {
    launchBin = AUS_Cc["@mozilla.org/file/local;1"].
                createInstance(AUS_Ci.nsILocalFile);
    launchBin.initWithPath("/bin/sh");
  }

  if (!launchBin.exists())
    do_throw(launchBin.path + " must exist to run this test!");

  return launchBin;
}

function waitForHelperSleep() {
  
  
  let output = getApplyDirFile("a/b/output", true);
  if (readFile(output) != "sleeping\n") {
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
    return;
  }
  output.remove(false);
  doUpdate();
}

function waitForHelperFinished() {
  
  
  let output = getApplyDirFile("a/b/output", true);
  if (readFile(output) != "finished\n") {
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperFinished);
    return;
  }
  
  
  waitForHelperFinishFileUnlock();
}

function waitForHelperFinishFileUnlock() {
  try {
    let output = getApplyDirFile("a/b/output", true);
    if (output.exists()) {
      output.remove(false);
    }
    let input = getApplyDirFile("a/b/input", true);
    if (input.exists()) {
      input.remove(false);
    }
  }
  catch (e) {
    
    
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperFinishFileUnlock);
    return;
  }
  checkUpdate();
}

function setupHelperFinish() {
  let input = getApplyDirFile("a/b/input", true);
  writeFile(input, "finish\n");
  waitForHelperFinished();
}








function setupUpdaterTest(aMarFile) {
  
  let updatesDir = do_get_file(gTestID + UPDATES_DIR_SUFFIX, true);
  try {
    removeDirRecursive(updatesDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + updatesDir.path + "\n" +
         "Exception: " + e + "\n");
  }
  if (!updatesDir.exists()) {
    updatesDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
  }

  
  let applyToDir = getApplyDirFile(null, true);
  try {
    removeDirRecursive(applyToDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + applyToDir.path + "\n" +
         "Exception: " + e + "\n");
  }
  logTestInfo("testing successful removal of the directory used to apply the " +
              "mar file");
  do_check_false(applyToDir.exists());

  
  
  TEST_FILES.forEach(function SUT_TF_FE(aTestFile) {
    if (aTestFile.originalFile || aTestFile.originalContents) {
      let testDir = getApplyDirFile(aTestFile.relPathDir, true);
      if (!testDir.exists())
        testDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);

      let testFile;
      if (aTestFile.originalFile) {
        testFile = getTestDirFile(aTestFile.originalFile);
        testFile.copyTo(testDir, aTestFile.fileName);
        testFile = getApplyDirFile(aTestFile.relPathDir + aTestFile.fileName);
      }
      else {
        testFile = getApplyDirFile(aTestFile.relPathDir + aTestFile.fileName, true);
        writeFile(testFile, aTestFile.originalContents);
      }

      
      
      if (!IS_WIN && !IS_OS2 && aTestFile.originalPerms) {
        testFile.permissions = aTestFile.originalPerms;
        
        
        if (!aTestFile.comparePerms)
          aTestFile.comparePerms = testFile.permissions;
      }
    }
  });

  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let afterApplyBinDir = getApplyDirFile("a/b/", true);
  helperBin.copyTo(afterApplyBinDir, gCallbackBinFile);

  
  let mar = getTestDirFile(aMarFile);
  mar.copyTo(updatesDir, FILE_UPDATE_ARCHIVE);

  
  
  var testDirs = TEST_DIRS.concat(ADDITIONAL_TEST_DIRS);
  testDirs.forEach(function SUT_TD_FE(aTestDir) {
    let testDir = getApplyDirFile(aTestDir.relPathDir, true);
    if (!testDir.exists()) {
      testDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
    }

    if (aTestDir.files) {
      aTestDir.files.forEach(function SUT_TD_F_FE(aTestFile) {
        let testFile = getApplyDirFile(aTestDir.relPathDir + aTestFile, true);
        if (!testFile.exists()) {
          testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
        }
      });
    }

    if (aTestDir.subDirs) {
      aTestDir.subDirs.forEach(function SUT_TD_SD_FE(aSubDir) {
        let testSubDir = getApplyDirFile(aTestDir.relPathDir + aSubDir, true);
        if (!testSubDir.exists()) {
          testSubDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
        }

        if (aTestDir.subDirFiles) {
          aTestDir.subDirFiles.forEach(function SUT_TD_SDF_FE(aTestFile) {
            let testFile = getApplyDirFile(aTestDir.relPathDir + aSubDir + aTestFile, true);
            if (!testFile.exists()) {
              testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
            }
          });
        }
      });
    }
  });
}





function cleanupUpdaterTest() {
  logTestInfo("start - updater test cleanup");
  let updatesDir = do_get_file(gTestID + UPDATES_DIR_SUFFIX, true);
  try {
    removeDirRecursive(updatesDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + updatesDir.path + "\n" +
         "Exception: " + e + "\n");
  }

  
  let applyToDir = getApplyDirFile(null, true).parent;
  try {
    removeDirRecursive(applyToDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + applyToDir.path + "\n" +
         "Exception: " + e + "\n");
  }

  cleanupTestCommon();
  logTestInfo("finish - updater test cleanup");
}





function checkUpdateLogContents(aCompareLogFile) {
  let updateLog = do_get_file(gTestID + UPDATES_DIR_SUFFIX, true);
  updateLog.append(FILE_UPDATE_LOG);
  let updateLogContents = readFileBytes(updateLog);
  if (gBackgroundUpdate) {
    
    updateLogContents = updateLogContents.replace(/Performing a background update/, "");
  } else if (gSwitchApp) {
    
    updateLogContents = updateLogContents.replace(/Performing a background update/, "");
    updateLogContents = updateLogContents.replace(/Performing a replace request/, "");
  }
  
  updateLogContents = updateLogContents.replace(/SOURCE DIRECTORY.*/g, "");
  updateLogContents = updateLogContents.replace(/DESTINATION DIRECTORY.*/g, "");
  
  updateLogContents = updateLogContents.replace(/NS_main: callback app open attempt .*/g, "");
  if (gSwitchApp) {
    
    updateLogContents = updateLogContents.replace(/^Begin moving.*$/mg, "");
#ifdef XP_MACOSX
    
    
    updateLogContents = updateLogContents.replace(/\n/g, "%%%EOL%%%");
    updateLogContents = updateLogContents.replace(/Moving the precomplete file.*Finished moving the precomplete file/, "");
    updateLogContents = updateLogContents.replace(/%%%EOL%%%/g, "\n");
#endif
  }
  updateLogContents = updateLogContents.replace(/\r/g, "");
  
  updateLogContents = updateLogContents.replace(/, err:.*\n/g, "\n");
  
  updateLogContents = updateLogContents.replace(/non-fatal error /g, "");
  
  
  updateLogContents = updateLogContents.replace(/.* a\/b\/7\/7text.*\n/g, "");
  
  updateLogContents = updateLogContents.replace(/\n+/g, "\n");
  
  updateLogContents = updateLogContents.replace(/^\n|\n$/g, "");

  let compareLog = getTestDirFile(aCompareLogFile);
  let compareLogContents = readFileBytes(compareLog);
  
  compareLogContents = compareLogContents.replace(/^\n|\n$/g, "");

  
  
  if (compareLogContents == updateLogContents) {
    logTestInfo("log contents are correct");
    do_check_true(true);
  }
  else {
    logTestInfo("log contents are not correct");
    do_check_eq(compareLogContents, updateLogContents);
  }
}

function checkUpdateLogContains(aCheckString) {
  let updateLog = do_get_file(gTestID + UPDATES_DIR_SUFFIX, true);
  updateLog.append(FILE_UPDATE_LOG);
  let updateLogContents = readFileBytes(updateLog);
  if (updateLogContents.indexOf(aCheckString) != -1) {
    logTestInfo("log file does contain: " + aCheckString);
    do_check_true(true);
  }
  else {
    logTestInfo("log file does not contain: " + aCheckString);
    logTestInfo("log file contents:\n" + updateLogContents);
    do_check_true(false);
  }
}





function checkFilesAfterUpdateSuccess() {
  logTestInfo("testing contents of files after a successful update");
  TEST_FILES.forEach(function CFAUS_TF_FE(aTestFile) {
    let testFile = getTargetDirFile(aTestFile.relPathDir + aTestFile.fileName, true);
    logTestInfo("testing file: " + testFile.path);
    if (aTestFile.compareFile || aTestFile.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && !IS_OS2 && aTestFile.comparePerms) {
        
        let logPerms = "testing file permissions - ";
        if (aTestFile.originalPerms) {
          logPerms += "original permissions: " + aTestFile.originalPerms.toString(8) + ", ";
        }
        logPerms += "compare permissions : " + aTestFile.comparePerms.toString(8) + ", ";
        logPerms += "updated permissions : " + testFile.permissions.toString(8);
        logTestInfo(logPerms);
        do_check_eq(testFile.permissions & 0xfff, aTestFile.comparePerms & 0xfff);
      }

      let fileContents1 = readFileBytes(testFile);
      let fileContents2 = aTestFile.compareFile ?
                          readFileBytes(getTestDirFile(aTestFile.compareFile)) :
                          aTestFile.compareContents;
      
      
      if (fileContents1 == fileContents2) {
        logTestInfo("file contents are correct");
        do_check_true(true);
      }
      else {
        logTestInfo("file contents are not correct");
        do_check_eq(fileContents1, fileContents2);
      }
    }
    else {
      do_check_false(testFile.exists());
    }
  });

  logTestInfo("testing operations specified in removed-files were performed " +
              "after a successful update");
  var testDirs = TEST_DIRS.concat(ADDITIONAL_TEST_DIRS);
  testDirs.forEach(function CFAUS_TD_FE(aTestDir) {
    let testDir = getTargetDirFile(aTestDir.relPathDir, true);
    logTestInfo("testing directory: " + testDir.path);
    if (aTestDir.dirRemoved) {
      do_check_false(testDir.exists());
    }
    else {
      do_check_true(testDir.exists());

      if (aTestDir.files) {
        aTestDir.files.forEach(function CFAUS_TD_F_FE(aTestFile) {
          let testFile = getTargetDirFile(aTestDir.relPathDir + aTestFile, true);
          logTestInfo("testing directory file: " + testFile.path);
          if (aTestDir.filesRemoved) {
            do_check_false(testFile.exists());
          }
          else {
            do_check_true(testFile.exists());
          }
        });
      }

      if (aTestDir.subDirs) {
        aTestDir.subDirs.forEach(function CFAUS_TD_SD_FE(aSubDir) {
          let testSubDir = getTargetDirFile(aTestDir.relPathDir + aSubDir, true);
          logTestInfo("testing sub-directory: " + testSubDir.path);
          do_check_true(testSubDir.exists());
          if (aTestDir.subDirFiles) {
            aTestDir.subDirFiles.forEach(function CFAUS_TD_SDF_FE(aTestFile) {
              let testFile = getTargetDirFile(aTestDir.relPathDir + aSubDir + aTestFile, true);
              logTestInfo("testing sub-directory file: " + testFile.path);
              do_check_true(testFile.exists());
            });
          }
        });
      }
    }
  });

  checkFilesAfterUpdateCommon();
}








function checkFilesAfterUpdateFailure(aGetDirectory) {
  let getdir = aGetDirectory || getTargetDirFile;
  logTestInfo("testing contents of files after a failed update");
  TEST_FILES.forEach(function CFAUF_TF_FE(aTestFile) {
    let testFile = getdir(aTestFile.relPathDir + aTestFile.fileName, true);
    logTestInfo("testing file: " + testFile.path);
    if (aTestFile.compareFile || aTestFile.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && !IS_OS2 && aTestFile.comparePerms) {
        
        let logPerms = "testing file permissions - ";
        if (aTestFile.originalPerms) {
          logPerms += "original permissions: " + aTestFile.originalPerms.toString(8) + ", ";
        }
        logPerms += "compare permissions : " + aTestFile.comparePerms.toString(8) + ", ";
        logPerms += "updated permissions : " + testFile.permissions.toString(8);
        logTestInfo(logPerms);
        do_check_eq(testFile.permissions & 0xfff, aTestFile.comparePerms & 0xfff);
      }

      let fileContents1 = readFileBytes(testFile);
      let fileContents2 = aTestFile.compareFile ?
                          readFileBytes(getTestDirFile(aTestFile.compareFile)) :
                          aTestFile.compareContents;
      
      
      if (fileContents1 == fileContents2) {
        logTestInfo("file contents are correct");
        do_check_true(true);
      }
      else {
        logTestInfo("file contents are not correct");
        do_check_eq(fileContents1, fileContents2);
      }
    }
    else {
      do_check_false(testFile.exists());
    }
  });

  logTestInfo("testing operations specified in removed-files were not " +
              "performed after a failed update");
  TEST_DIRS.forEach(function CFAUF_TD_FE(aTestDir) {
    let testDir = getdir(aTestDir.relPathDir, true);
    logTestInfo("testing directory file: " + testDir.path);
    do_check_true(testDir.exists());

    if (aTestDir.files) {
      aTestDir.files.forEach(function CFAUS_TD_F_FE(aTestFile) {
        let testFile = getdir(aTestDir.relPathDir + aTestFile, true);
        logTestInfo("testing directory file: " + testFile.path);
        do_check_true(testFile.exists());
      });
    }

    if (aTestDir.subDirs) {
      aTestDir.subDirs.forEach(function CFAUS_TD_SD_FE(aSubDir) {
        let testSubDir = getdir(aTestDir.relPathDir + aSubDir, true);
        logTestInfo("testing sub-directory: " + testSubDir.path);
        do_check_true(testSubDir.exists());
        if (aTestDir.subDirFiles) {
          aTestDir.subDirFiles.forEach(function CFAUS_TD_SDF_FE(aTestFile) {
            let testFile = getdir(aTestDir.relPathDir + aSubDir + aTestFile, true);
            logTestInfo("testing sub-directory file: " + testFile.path);
            do_check_true(testFile.exists());
          });
        }
      });
    }
  });

  checkFilesAfterUpdateCommon();
}





function checkFilesAfterUpdateCommon() {
  logTestInfo("testing patch files should not be left behind");
  let updatesDir = do_get_file(gTestID + UPDATES_DIR_SUFFIX, true);
  let entries = updatesDir.QueryInterface(AUS_Ci.nsIFile).directoryEntries;
  while (entries.hasMoreElements()) {
    let entry = entries.getNext().QueryInterface(AUS_Ci.nsIFile);
    do_check_neq(getFileExtension(entry), "patch");
  }

  logTestInfo("testing backup files should not be left behind");
  let applyToDir = getTargetDirFile(null, true);
  checkFilesInDirRecursive(applyToDir, checkForBackupFiles);
}






function checkCallbackAppLog() {
  let appLaunchLog = getApplyDirFile("a/b/" + gCallbackArgs[1], true);
  if (!appLaunchLog.exists()) {
    do_timeout(TEST_HELPER_TIMEOUT, checkCallbackAppLog);
    return;
  }

  let expectedLogContents = gCallbackArgs.join("\n") + "\n";
  let logContents = readFile(appLaunchLog);
  
  
  
  
  if (logContents != expectedLogContents) {
    do_timeout(TEST_HELPER_TIMEOUT, checkCallbackAppLog);
    return;
  }

  if (logContents == expectedLogContents) {
    logTestInfo("callback log file contents are correct");
    do_check_true(true);
  }
  else {
    logTestInfo("callback log file contents are not correct");
    do_check_eq(logContents, expectedLogContents);
  }

  removeCallbackCopy();
}






function checkCallbackServiceLog() {
  do_check_neq(gServiceLaunchedCallbackLog, null);

  let expectedLogContents = gServiceLaunchedCallbackArgs.join("\n") + "\n";
  let logFile = AUS_Cc["@mozilla.org/file/local;1"].createInstance(AUS_Ci.nsILocalFile);
  logFile.initWithPath(gServiceLaunchedCallbackLog);
  let logContents = readFile(logFile);

  
  
  
  
  if (logContents != expectedLogContents) {
    logTestInfo("callback service log not expected value, waiting longer");
    do_timeout(TEST_HELPER_TIMEOUT, checkCallbackServiceLog);
    return;
  }

  logTestInfo("testing that the callback application successfully launched " +
              "and the expected command line arguments passed to it");
  do_check_eq(logContents, expectedLogContents);

  removeCallbackCopy();
}

function removeCallbackCopy() {
  
  
  let appBinCopy = getAppDir();
  appBinCopy.append(gTestID + FILE_WIN_TEST_EXE);
  if (appBinCopy.exists()) {
    try {
      logTestInfo("attempting removal of file: " + appBinCopy.path);
      appBinCopy.remove(false);
    }
    catch (e) {
      logTestInfo("non-fatal error removing file after test finished (will " +
                  "try again). File: " + appBinCopy.path + " Exception: " + e);
      do_timeout(TEST_HELPER_TIMEOUT, removeCallbackCopy);
      return;
    }
  }
  logTestInfo("attempting removal of the updater binary");
  removeUpdater();
}






function removeUpdater() {
  if (IS_WIN) {
    
    
    let updater = getUpdatesDir();
    updater.append("0");
    updater.append(FILE_UPDATER_BIN);
    if (updater.exists()) {
      try {
        updater.remove(false);
      }
      catch (e) {
        logTestInfo("non-fatal error removing file after test finished (will " +
                    "try again). File: " + updater.path + " Exception: " + e);
        do_timeout(TEST_HELPER_TIMEOUT, removeUpdater);
        return;
      }
    }
    else {
      logTestInfo("updater doesn't exist, path: " + updater.path);
    }
  }
  logTestInfo("calling do_test_finished");
  do_test_finished();
}



function waitForFilesInUse() {
  let maintSvcInstaller = getAppDir();
  maintSvcInstaller.append("FILE_MAINTENANCE_SERVICE_INSTALLER_BIN");

  let helper = getAppDir();
  helper.append("uninstall");
  helper.append("helper.exe");

  let files = [maintSvcInstaller, helper];
  for (let i = 0; i < files.length; ++i) {
    let file = files[i];
    let fileBak = file.parent.clone();
    if (file.exists()) {
      fileBak.append(file.leafName + ".bak");
      try {
        if (fileBak.exists()) {
          fileBak.remove(false);
        }
        file.copyTo(fileBak.parent, fileBak.leafName);
        file.remove(false);
        fileBak.moveTo(file.parent, file.leafName);
        logTestInfo("file is not in use. path: " + file.path);
      }
      catch (e) {
        logTestInfo("file in use, will try again after " + TEST_CHECK_TIMEOUT +
                    " ms, path: " + file.path + ", exception: " + e);
        try {
          if (fileBak.exists()) {
            fileBak.remove(false);
          }
        }
        catch (e) {
          logTestInfo("unable to remove file, this should never happen! " +
                      "path: " + fileBak.path + ", exception: " + e);
        }
        do_timeout(TEST_CHECK_TIMEOUT, waitForFilesInUse);
        return;
      }
    }
  }

  removeCallbackCopy();
}








function checkForBackupFiles(aFile) {
  do_check_neq(getFileExtension(aFile), "moz-backup");
}












function checkFilesInDirRecursive(aDir, aCallback) {
  if (!aDir.exists())
    do_throw("Directory must exist!");

  let dirEntries = aDir.directoryEntries;
  while (dirEntries.hasMoreElements()) {
    let entry = dirEntries.getNext().QueryInterface(AUS_Ci.nsIFile);

    if (entry.isDirectory()) {
      checkFilesInDirRecursive(entry, aCallback);
    }
    else {
      aCallback(entry);
    }
  }
}
















function overrideXHR(callback) {
  gXHRCallback = callback;
  gXHR = new xhr();
  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(gXHR.classID, gXHR.classDescription,
                            gXHR.contractID, gXHR);
}






function makeHandler(val) {
  if (typeof val == "function")
    return ({ handleEvent: val });
  return val;
}
function xhr() {
}
xhr.prototype = {
  overrideMimeType: function(mimetype) { },
  setRequestHeader: function(header, value) { },
  status: null,
  channel: { set notificationCallbacks(val) { } },
  _url: null,
  _method: null,
  open: function (method, url) {
    gXHR.channel.originalURI = Services.io.newURI(url, null, null);
    gXHR._method = method; gXHR._url = url;
  },
  responseXML: null,
  responseText: null,
  send: function(body) {
    do_execute_soon(gXHRCallback); 
  },
  _onprogress: null,
  set onprogress(val) { gXHR._onprogress = makeHandler(val); },
  get onprogress() { return gXHR._onprogress; },
  _onerror: null,
  set onerror(val) { gXHR._onerror = makeHandler(val); },
  get onerror() { return gXHR._onerror; },
  _onload: null,
  set onload(val) { gXHR._onload = makeHandler(val); },
  get onload() { return gXHR._onload; },
  addEventListener: function(event, val, capturing) {
    eval("gXHR._on" + event + " = val");
  },
  flags: AUS_Ci.nsIClassInfo.SINGLETON,
  implementationLanguage: AUS_Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage: function(language) null,
  getInterfaces: function(count) {
    var interfaces = [AUS_Ci.nsISupports];
    count.value = interfaces.length;
    return interfaces;
  },
  classDescription: "XMLHttpRequest",
  contractID: "@mozilla.org/xmlextras/xmlhttprequest;1",
  classID: Components.ID("{c9b37f43-4278-4304-a5e0-600991ab08cb}"),
  createInstance: function (outer, aIID) {
    if (outer == null)
      return gXHR.QueryInterface(aIID);
    throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
  },
  QueryInterface: function(aIID) {
    if (aIID.equals(AUS_Ci.nsIClassInfo) ||
        aIID.equals(AUS_Ci.nsISupports))
      return gXHR;
    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  },
  get wrappedJSObject() { return this; }
};

function overrideUpdatePrompt(callback) {
  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  gUpdatePrompt = new UpdatePrompt();
  gUpdatePromptCallback = callback;
  registrar.registerFactory(gUpdatePrompt.classID, gUpdatePrompt.classDescription,
                            gUpdatePrompt.contractID, gUpdatePrompt);
}

function UpdatePrompt() {
  var fns = ["checkForUpdates", "showUpdateAvailable", "showUpdateDownloaded",
             "showUpdateError", "showUpdateHistory", "showUpdateInstalled"];

  fns.forEach(function(promptFn) {
    UpdatePrompt.prototype[promptFn] = function() {
      if (!gUpdatePromptCallback) {
        return;
      }

      var callback = gUpdatePromptCallback[promptFn];
      if (!callback) {
        return;
      }

      callback.apply(gUpdatePromptCallback,
                     Array.prototype.slice.call(arguments));
    }
  });
}

UpdatePrompt.prototype = {
  flags: AUS_Ci.nsIClassInfo.SINGLETON,
  implementationLanguage: AUS_Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage: function(language) null,
  getInterfaces: function(count) {
    var interfaces = [AUS_Ci.nsISupports, AUS_Ci.nsIUpdatePrompt];
    count.value = interfaces.length;
    return interfaces;
  },
  classDescription: "UpdatePrompt",
  contractID: "@mozilla.org/updates/update-prompt;1",
  classID: Components.ID("{8c350a15-9b90-4622-93a1-4d320308664b}"),
  createInstance: function (outer, aIID) {
    if (outer == null)
      return gUpdatePrompt.QueryInterface(aIID);
    throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
  },
  QueryInterface: function(aIID) {
    if (aIID.equals(AUS_Ci.nsIClassInfo) ||
        aIID.equals(AUS_Ci.nsISupports) ||
        aIID.equals(AUS_Ci.nsIUpdatePrompt))
      return gUpdatePrompt;
    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  },
};




const updateCheckListener = {
  onProgress: function UCL_onProgress(request, position, totalSize) {
  },

  onCheckComplete: function UCL_onCheckComplete(request, updates, updateCount) {
    gRequestURL = request.channel.originalURI.spec;
    gUpdateCount = updateCount;
    gUpdates = updates;
    logTestInfo("url = " + gRequestURL + ", " +
                "request.status = " + request.status + ", " +
                "update.statusText = " + request.statusText + ", " +
                "updateCount = " + updateCount);
    
    do_execute_soon(gCheckFunc);
  },

  onError: function UCL_onError(request, update) {
    gRequestURL = request.channel.originalURI.spec;
    gStatusCode = request.status;

    gStatusText = update.statusText;
    logTestInfo("url = " + gRequestURL + ", " +
                "request.status = " + gStatusCode + ", " +
                "update.statusText = " + gStatusText);
    
    do_execute_soon(gCheckFunc.bind(null, request, update));
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};


const downloadListener = {
  onStartRequest: function DL_onStartRequest(request, context) {
  },

  onProgress: function DL_onProgress(request, context, progress, maxProgress) {
  },

  onStatus: function DL_onStatus(request, context, status, statusText) {
  },

  onStopRequest: function DL_onStopRequest(request, context, status) {
    gStatusResult = status;
    
    do_execute_soon(gCheckFunc);
  },

  QueryInterface: function DL_QueryInterface(aIID) {
    if (!aIID.equals(AUS_Ci.nsIRequestObserver) &&
        !aIID.equals(AUS_Ci.nsIProgressEventSink) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};




function start_httpserver() {
  let dir = getTestDirFile();
  logTestInfo("http server file path: " + dir.path);

  if (!dir.isDirectory()) {
    do_throw("A file instead of a directory was specified for HttpServer " +
             "registerDirectory! path: " + dir.path + "\n");
  }

  Components.utils.import("resource://testing-common/httpd.js");
  gTestserver = new HttpServer();
  gTestserver.registerDirectory("/", dir);
  gTestserver.start(-1);
  let testserverPort = gTestserver.identity.primaryPort;
  gURLData = URL_HOST + ":" + testserverPort + "/";
  logTestInfo("http server port = " + testserverPort);
}


function stop_httpserver(callback) {
  do_check_true(!!callback);
  gTestserver.stop(callback);
}













function createAppInfo(id, name, version, platformVersion) {
  const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
  const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");
  var XULAppInfo = {
    vendor: APP_INFO_VENDOR,
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

    QueryInterface: function QueryInterface(iid) {
      if (iid.equals(AUS_Ci.nsIXULAppInfo) ||
          iid.equals(AUS_Ci.nsIXULRuntime) ||
#ifdef XP_WIN
          iid.equals(AUS_Ci.nsIWinAppHelper) ||
#endif
          iid.equals(AUS_Ci.nsISupports))
        return this;
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    }
  };

  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer == null)
        return XULAppInfo.QueryInterface(iid);
      throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
    }
  };

  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}
























function getProcessArgs(aExtraArgs) {
  if (!aExtraArgs) {
    aExtraArgs = [];
  }

  
  
  let appConsoleLogPath = getAppConsoleLogPath();

  let args;
  if (IS_UNIX) {
    let launchScript = getLaunchScript();
    
    launchScript.create(AUS_Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_DIRECTORY);

    let scriptContents = "#! /bin/sh\n";
    scriptContents += gAppBinPath + " -no-remote -process-updates " +
                      aExtraArgs.join(" ") + " 1> " +
                      appConsoleLogPath + " 2>&1";
    writeFile(launchScript, scriptContents);
    logTestInfo("created " + launchScript.path + " containing:\n" +
                scriptContents);
    args = [launchScript.path];
  }
  else {
    args = ["/D", "/Q", "/C", gAppBinPath, "-no-remote", "-process-updates"].
           concat(aExtraArgs).
           concat(["1>", appConsoleLogPath, "2>&1"]);
  }
  return args;
}







function getAppConsoleLogPath() {
  let appConsoleLog = do_get_file("/", true);
  appConsoleLog.append(gTestID + "_app_console_log");
  if (appConsoleLog.exists()) {
    appConsoleLog.remove(false);
  }
  let appConsoleLogPath = appConsoleLog.path;
  if (/ /.test(appConsoleLogPath)) {
    appConsoleLogPath = '"' + appConsoleLogPath + '"';
  }
  return appConsoleLogPath;
}







function getAppArgsLogPath() {
  let appArgsLog = do_get_file("/", true);
  appArgsLog.append(gTestID + "_app_args_log");
  if (appArgsLog.exists()) {
    appArgsLog.remove(false);
  }
  let appArgsLogPath = appArgsLog.path;
  if (/ /.test(appArgsLogPath)) {
    appArgsLogPath = '"' + appArgsLogPath + '"';
  }
  return appArgsLogPath;
}







function getLaunchScript() {
  let launchScript = do_get_file("/", true);
  launchScript.append(gTestID + "_launch.sh");
  if (launchScript.exists()) {
    launchScript.remove(false);
  }
  return launchScript;
}









XPCOMUtils.defineLazyGetter(this, "gAppBinPath", function test_gAppBinPath() {
  let processDir = getAppDir();
  let appBin = processDir.clone();
  appBin.append(APP_BIN_NAME + APP_BIN_SUFFIX);
  if (appBin.exists()) {
    if (IS_WIN) {
      let appBinCopy = processDir.clone();
      appBinCopy.append(gTestID + FILE_WIN_TEST_EXE);
      if (appBinCopy.exists()) {
        appBinCopy.remove(false);
      }
      appBin.copyTo(processDir, gTestID + FILE_WIN_TEST_EXE);
      appBin = processDir.clone();
      appBin.append(gTestID + FILE_WIN_TEST_EXE);
    }
    let appBinPath = appBin.path;
    if (/ /.test(appBinPath)) {
      appBinPath = '"' + appBinPath + '"';
    }
    return appBinPath;
  }
  return null;
});





function shouldAdjustPathsOnMac() {
  return false;
}



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
    dest.create(dest.NORMAL_FILE_TYPE, 0o644);
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
  dest2.create(dest.DIRECTORY_TYPE, 0o755);
  do_check_true(dest2.exists());
  let source2 = getUpdatesRootDir();
  source2.append("updates");
  if (source2.exists()) {
    source2.remove(true);
  }
  ret = symlink(dest2.path, source2.path);
  do_check_eq(ret, 0);
  do_check_true(source2.exists());

  
  do_register_cleanup(function AUFIBD_cleanup() {
    logTestInfo("start - unlinking symlinks");
    let ret = unlink(source.path);
    do_check_false(source.exists());
    let ret = unlink(source2.path);
    do_check_false(source2.exists());
    logTestInfo("finish - unlinking symlinks");
  });

  
  
  
  getUpdatesRootDir = getAppDir;
}






function adjustPathsOnWindows() {
  logTestInfo("start - setup new process directory");
  
  
  let tmpDir = do_get_profile();
  tmpDir.append("ExecutableDir.tmp");
  tmpDir.createUnique(tmpDir.DIRECTORY_TYPE, 0o755);
  let procDir = getCurrentProcessDir();
  logTestInfo("start - copy the process directory");
  copyMinimumAppFiles(procDir, tmpDir, "bin");
  logTestInfo("finish - copy the process directory");
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
  do_register_cleanup(function APOW_cleanup() {
    logTestInfo("start - unregistering directory provider");
    ds.unregisterProvider(dirProvider);
    logTestInfo("finish - unregistering directory provider");
  });
  logTestInfo("finish - setup new process directory");
}

let gWindowsBinDir = null;





function adjustGeneralPaths() {
  let dirProvider = {
    getFile: function DP_getFile(prop, persistent) {
      persistent.value = true;
      if (prop == XRE_EXECUTABLE_FILE)
        return do_get_file(getApplyDirPath() + "test" + APP_BIN_SUFFIX, true);
      if (prop == XRE_UPDATE_ROOT_DIR)
        return do_get_file(getApplyDirPath(), true);
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
  ds.registerProvider(dirProvider);
  do_register_cleanup(function() {
    
    end_test();
    ds.unregisterProvider(dirProvider);
    let testBin = do_get_file(getApplyDirPath() + "test" + APP_BIN_SUFFIX, true);
    
    if (testBin.exists()) {
      try {
        testBin.remove(false);
      }
      catch (e) {
        dump("Unable to remove file\n" +
             "path: " + testBin.path + "\n" +
             "Exception: " + e + "\n");
      }
    }
    let testDir = do_get_file(getApplyDirPath(), true).parent;
    
    
    if (testDir.exists()) {
      try {
        removeDirRecursive(testDir);
      }
      catch (e) {
        dump("Unable to remove directory\n" +
             "path: " + testDir.path + "\n" +
             "Exception: " + e + "\n");
      }
    }
  });
}





function getAppDir() {
  let dir = getCurrentProcessDir();
  if (shouldAdjustPathsOnMac()) {
    
    dir = dir.parent;
    dir.append(BUNDLE_NAME);
    dir.append("Contents");
    dir.append("MacOS");
  } else if (IS_WIN && gWindowsBinDir) {
    dir = gWindowsBinDir.clone();
  }
  return dir;
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
    do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
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




let gUpdateStagedObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "update-staged") {
      Services.obs.removeObserver(gUpdateStagedObserver, "update-staged");
      checkUpdateApplied();
    }
  },
  QueryInterface: XPCOMUtils.generateQI([AUS_Ci.nsIObserver])
};


let gShouldResetEnv = undefined;
let gAddedEnvXRENoWindowsCrashDialog = false;
let gEnvXPCOMDebugBreak;
let gEnvXPCOMMemLeakLog;
let gEnvDyldLibraryPath;
let gEnvLdLibraryPath;
let gEnvUpdateRootOverride = null;
let gEnvAppDirOverride = null;
let gEnvSKipUpdateDirHashing = false;





function setEnvironment() {
  
  if (gShouldResetEnv !== undefined)
    return;

  gShouldResetEnv = true;

  let env = AUS_Cc["@mozilla.org/process/environment;1"].
            getService(AUS_Ci.nsIEnvironment);
  if (IS_WIN && !env.exists("XRE_NO_WINDOWS_CRASH_DIALOG")) {
    gAddedEnvXRENoWindowsCrashDialog = true;
    logTestInfo("setting the XRE_NO_WINDOWS_CRASH_DIALOG environment " +
                "variable to 1... previously it didn't exist");
    env.set("XRE_NO_WINDOWS_CRASH_DIALOG", "1");
  }

  if (IS_UNIX) {
    let appGreDir = Services.dirsvc.get("GreD", AUS_Ci.nsIFile);
    let envGreDir = AUS_Cc["@mozilla.org/file/local;1"].
                    createInstance(AUS_Ci.nsILocalFile);
    let shouldSetEnv = true;
    if (IS_MACOSX) {
      if (env.exists("DYLD_LIBRARY_PATH")) {
        gEnvDyldLibraryPath = env.get("DYLD_LIBRARY_PATH");
        envGreDir.initWithPath(gEnvDyldLibraryPath);
        if (envGreDir.path == appGreDir.path) {
          gEnvDyldLibraryPath = null;
          shouldSetEnv = false;
        }
      }

      if (shouldSetEnv) {
        logTestInfo("setting DYLD_LIBRARY_PATH environment variable value to " +
                    appGreDir.path);
        env.set("DYLD_LIBRARY_PATH", appGreDir.path);
      }
    }
    else {
      if (env.exists("LD_LIBRARY_PATH")) {
        gEnvLdLibraryPath = env.get("LD_LIBRARY_PATH");
        envGreDir.initWithPath(gEnvLdLibraryPath);
        if (envGreDir.path == appGreDir.path) {
          gEnvLdLibraryPath = null;
          shouldSetEnv = false;
        }
      }

      if (shouldSetEnv) {
        logTestInfo("setting LD_LIBRARY_PATH environment variable value to " +
                    appGreDir.path);
        env.set("LD_LIBRARY_PATH", appGreDir.path);
      }
    }
  }

  if (env.exists("XPCOM_MEM_LEAK_LOG")) {
    gEnvXPCOMMemLeakLog = env.get("XPCOM_MEM_LEAK_LOG");
    logTestInfo("removing the XPCOM_MEM_LEAK_LOG environment variable... " +
                "previous value " + gEnvXPCOMMemLeakLog);
    env.set("XPCOM_MEM_LEAK_LOG", "");
  }

  if (env.exists("XPCOM_DEBUG_BREAK")) {
    gEnvXPCOMDebugBreak = env.get("XPCOM_DEBUG_BREAK");
    logTestInfo("setting the XPCOM_DEBUG_BREAK environment variable to " +
                "warn... previous value " + gEnvXPCOMDebugBreak);
  }
  else {
    logTestInfo("setting the XPCOM_DEBUG_BREAK environment variable to " +
                "warn... previously it didn't exist");
  }

  env.set("XPCOM_DEBUG_BREAK", "warn");

  if (IS_WIN && gEnvSKipUpdateDirHashing) {
    env.set("MOZ_UPDATE_NO_HASH_DIR", "1");
  }

  if (gEnvUpdateRootOverride) {
    logTestInfo("setting the MOZ_UPDATE_ROOT_OVERRIDE environment variable to " +
                gEnvUpdateRootOverride + "\n");
    env.set("MOZ_UPDATE_ROOT_OVERRIDE", gEnvUpdateRootOverride);
  }

  if (gEnvAppDirOverride) {
    logTestInfo("setting the MOZ_UPDATE_APPDIR_OVERRIDE environment variable to " +
                gEnvAppDirOverride + "\n");
    env.set("MOZ_UPDATE_APPDIR_OVERRIDE", gEnvAppDirOverride);
  }

  if (gBackgroundUpdate) {
    logTestInfo("setting the MOZ_UPDATE_BACKGROUND environment variable to 1\n");
    env.set("MOZ_UPDATE_BACKGROUND", "1");
  }

  logTestInfo("setting MOZ_NO_SERVICE_FALLBACK environment variable to 1");
  env.set("MOZ_NO_SERVICE_FALLBACK", "1");
}





function resetEnvironment() {
  
  if (gShouldResetEnv !== true)
    return;

  gShouldResetEnv = false;

  let env = AUS_Cc["@mozilla.org/process/environment;1"].
            getService(AUS_Ci.nsIEnvironment);

  if (gEnvXPCOMMemLeakLog) {
    logTestInfo("setting the XPCOM_MEM_LEAK_LOG environment variable back to " +
                gEnvXPCOMMemLeakLog);
    env.set("XPCOM_MEM_LEAK_LOG", gEnvXPCOMMemLeakLog);
  }

  if (gEnvXPCOMDebugBreak) {
    logTestInfo("setting the XPCOM_DEBUG_BREAK environment variable back to " +
                gEnvXPCOMDebugBreak);
    env.set("XPCOM_DEBUG_BREAK", gEnvXPCOMDebugBreak);
  }
  else {
    logTestInfo("clearing the XPCOM_DEBUG_BREAK environment variable");
    env.set("XPCOM_DEBUG_BREAK", "");
  }

  if (IS_UNIX) {
    if (IS_MACOSX) {
      if (gEnvDyldLibraryPath) {
        logTestInfo("setting DYLD_LIBRARY_PATH environment variable value " +
                    "back to " + gEnvDyldLibraryPath);
        env.set("DYLD_LIBRARY_PATH", gEnvDyldLibraryPath);
      }
      else {
        logTestInfo("removing DYLD_LIBRARY_PATH environment variable");
        env.set("DYLD_LIBRARY_PATH", "");
      }
    }
    else {
      if (gEnvLdLibraryPath) {
        logTestInfo("setting LD_LIBRARY_PATH environment variable value back " +
                    "to " + gEnvLdLibraryPath);
        env.set("LD_LIBRARY_PATH", gEnvLdLibraryPath);
      }
      else {
        logTestInfo("removing LD_LIBRARY_PATH environment variable");
        env.set("LD_LIBRARY_PATH", "");
      }
    }
  }

  if (IS_WIN && gAddedEnvXRENoWindowsCrashDialog) {
    logTestInfo("removing the XRE_NO_WINDOWS_CRASH_DIALOG environment " +
                "variable");
    env.set("XRE_NO_WINDOWS_CRASH_DIALOG", "");
  }

  if (gEnvUpdateRootOverride) {
    logTestInfo("removing the MOZ_UPDATE_ROOT_OVERRIDE environment variable\n");
    env.set("MOZ_UPDATE_ROOT_OVERRIDE", "");
    gEnvUpdateRootOverride = null;
  }

  if (gEnvAppDirOverride) {
    logTestInfo("removing the MOZ_UPDATE_APPDIR_OVERRIDE environment variable\n");
    env.set("MOZ_UPDATE_APPDIR_OVERRIDE", "");
    gEnvAppDirOverride = null;
  }

  if (gBackgroundUpdate) {
    logTestInfo("removing the MOZ_UPDATE_BACKGROUND environment variable\n");
    env.set("MOZ_UPDATE_BACKGROUND", "");
  }

  logTestInfo("removing MOZ_NO_SERVICE_FALLBACK environment variable");
  env.set("MOZ_NO_SERVICE_FALLBACK", "");
}
