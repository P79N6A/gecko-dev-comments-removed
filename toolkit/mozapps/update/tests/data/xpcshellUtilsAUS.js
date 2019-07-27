



'use strict';

const { classes: Cc, interfaces: Ci, manager: Cm, results: Cr,
        utils: Cu } = Components;

load("../data/xpcshellConstantsPP.js");

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/ctypes.jsm", this);

const DIR_MACOS = IS_MACOSX ? "Contents/MacOS/" : "";
const DIR_RESOURCES = IS_MACOSX ? "Contents/Resources/" : "";
const TEST_FILE_SUFFIX =  IS_MACOSX ? "_mac" : "";
const FILE_COMPLETE_MAR = "complete" + TEST_FILE_SUFFIX + ".mar";
const FILE_PARTIAL_MAR = "partial" + TEST_FILE_SUFFIX + ".mar";
const LOG_COMPLETE_SUCCESS = "complete_log_success" + TEST_FILE_SUFFIX;
const LOG_PARTIAL_SUCCESS  = "partial_log_success" + TEST_FILE_SUFFIX;
const LOG_PARTIAL_FAILURE  = "partial_log_failure" + TEST_FILE_SUFFIX;
const FILE_COMPLETE_PRECOMPLETE = "complete_precomplete" + TEST_FILE_SUFFIX;
const FILE_PARTIAL_PRECOMPLETE = "partial_precomplete" + TEST_FILE_SUFFIX;
const FILE_COMPLETE_REMOVEDFILES = "complete_removed-files" + TEST_FILE_SUFFIX;
const FILE_PARTIAL_REMOVEDFILES = "partial_removed-files" + TEST_FILE_SUFFIX;

const USE_EXECV = IS_UNIX && !IS_MACOSX;

const URL_HOST = "http://localhost";

const FILE_APP_BIN = MOZ_APP_NAME + APP_BIN_SUFFIX;
const FILE_COMPLETE_EXE = "complete.exe";
const FILE_HELPER_BIN = "TestAUSHelper" + BIN_SUFFIX;
const FILE_MAINTENANCE_SERVICE_BIN = "maintenanceservice.exe";
const FILE_MAINTENANCE_SERVICE_INSTALLER_BIN = "maintenanceservice_installer.exe";
const FILE_OLD_VERSION_MAR = "old_version.mar";
const FILE_PARTIAL_EXE = "partial.exe";
const FILE_UPDATER_BIN = "updater" + BIN_SUFFIX;
const FILE_WRONG_CHANNEL_MAR = "wrong_product_channel.mar";

const LOG_SWITCH_SUCCESS = "rename_file: proceeding to rename the directory\n" +
                           "rename_file: proceeding to rename the directory\n" +
                           "Now, remove the tmpDir\n" +
                           "succeeded\n" +
                           "calling QuitProgressUI";

const ERR_RENAME_FILE = "rename_file: failed to rename file";
const ERR_UNABLE_OPEN_DEST = "unable to open destination file";
const ERR_BACKUP_DISCARD = "backup_discard: unable to remove";

const LOG_SVC_SUCCESSFUL_LAUNCH = "Process was started... waiting on result.";




const MAC_MAX_TIME_DIFFERENCE = 60000;


const TEST_HELPER_TIMEOUT = 100;


const TEST_CHECK_TIMEOUT = 100;


const MAX_TIMEOUT_RUNS = 2000;


const HELPER_SLEEP_TIMEOUT = 180;



const APP_TIMER_TIMEOUT = 120000;

const PIPE_TO_NULL = IS_WIN ? ">nul" : "> /dev/null 2>&1";

const LOG_FUNCTION = do_print;


var gURLData = URL_HOST + "/";

var gTestID;

var gTestserver;

var gRegisteredServiceCleanup;

var gCheckFunc;
var gResponseBody;
var gResponseStatusCode = 200;
var gRequestURL;
var gUpdateCount;
var gUpdates;
var gStatusCode;
var gStatusText;
var gStatusResult;

var gProcess;
var gAppTimer;
var gHandle;

var gGREDirOrig;
var gGREBinDirOrig;
var gAppDirOrig;

var gServiceLaunchedCallbackLog = null;
var gServiceLaunchedCallbackArgs = null;



var gCallbackBinFile = "callback_app" + BIN_SUFFIX;
var gCallbackArgs = ["./", "callback.log", "Test Arg 2", "Test Arg 3"];
var gPostUpdateBinFile = "postup_app" + BIN_SUFFIX;
var gStageUpdate = false;
var gSwitchApp = false;
var gDisableReplaceFallback = false;
var gUseTestAppDir = true;

var gTimeoutRuns = 0;


var gShouldResetEnv = undefined;
var gAddedEnvXRENoWindowsCrashDialog = false;
var gEnvXPCOMDebugBreak;
var gEnvXPCOMMemLeakLog;
var gEnvDyldLibraryPath;
var gEnvLdLibraryPath;




var DEBUG_AUS_TEST = false;




var DEBUG_TEST_LOG = false;

var gDeleteLogFile = true;
var gRealDump;
var gTestLogText = "";
var gPassed;

const DATA_URI_SPEC = Services.io.newFileURI(do_get_file("../data", false)).spec;
Services.scriptloader.loadSubScript(DATA_URI_SPEC + "shared.js", this);

var gTestFiles = [];
var gTestDirs = [];


var gTestFilesCommon = [
{
  description      : "Should never change",
  fileName         : FILE_UPDATE_SETTINGS_INI,
  relPathDir       : DIR_RESOURCES,
  originalContents : UPDATE_SETTINGS_CONTENTS,
  compareContents  : UPDATE_SETTINGS_CONTENTS,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o767,
  comparePerms     : 0o767
}, {
  description      : "Should never change",
  fileName         : "channel-prefs.js",
  relPathDir       : DIR_RESOURCES + "defaults/pref/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o767,
  comparePerms     : 0o767
}];



var gTestFilesCompleteSuccess = [
{
  description      : "Added by update.manifest (add)",
  fileName         : "precomplete",
  relPathDir       : DIR_RESOURCES,
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_PARTIAL_PRECOMPLETE,
  compareFile      : FILE_COMPLETE_PRECOMPLETE,
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginstext0",
  relPathDir       : DIR_RESOURCES + "searchplugins/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o775,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginspng1.png",
  relPathDir       : DIR_RESOURCES + "searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginspng0.png",
  relPathDir       : DIR_RESOURCES + "searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "partial.png",
  compareFile      : "complete.png",
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "removed-files",
  relPathDir       : DIR_RESOURCES,
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_PARTIAL_REMOVEDFILES,
  compareFile      : FILE_COMPLETE_REMOVEDFILES,
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions1text0",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions1/",
  originalContents : null,
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions1png1.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "partial.png",
  compareFile      : "complete.png",
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions1png0.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions0text0",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions0/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions0png1.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions0png0.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "exe0.exe",
  relPathDir       : DIR_MACOS,
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_HELPER_BIN,
  compareFile      : FILE_COMPLETE_EXE,
  originalPerms    : 0o777,
  comparePerms     : 0o755
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "10text0",
  relPathDir       : DIR_RESOURCES + "1/10/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o767,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "0exe0.exe",
  relPathDir       : DIR_RESOURCES + "0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_HELPER_BIN,
  compareFile      : FILE_COMPLETE_EXE,
  originalPerms    : 0o777,
  comparePerms     : 0o755
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text1",
  relPathDir       : DIR_RESOURCES + "0/00/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o677,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text0",
  relPathDir       : DIR_RESOURCES + "0/00/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o775,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00png0.png",
  relPathDir       : DIR_RESOURCES + "0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : 0o776,
  comparePerms     : 0o644
}, {
  description      : "Removed by precomplete (remove)",
  fileName         : "20text0",
  relPathDir       : DIR_RESOURCES + "2/20/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Removed by precomplete (remove)",
  fileName         : "20png0.png",
  relPathDir       : DIR_RESOURCES + "2/20/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}];


gTestFilesCompleteSuccess = gTestFilesCompleteSuccess.concat(gTestFilesCommon);



var gTestFilesPartialSuccess = [
{
  description      : "Added by update.manifest (add)",
  fileName         : "precomplete",
  relPathDir       : DIR_RESOURCES,
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_COMPLETE_PRECOMPLETE,
  compareFile      : FILE_PARTIAL_PRECOMPLETE,
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginstext0",
  relPathDir       : DIR_RESOURCES + "searchplugins/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o775,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the file exists " +
                     "(patch-if)",
  fileName         : "searchpluginspng1.png",
  relPathDir       : DIR_RESOURCES + "searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Patched by update.manifest if the file exists " +
                     "(patch-if)",
  fileName         : "searchpluginspng0.png",
  relPathDir       : DIR_RESOURCES + "searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions1text0",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions1/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions1png1.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions1png0.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions0text0",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions0/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions0png1.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions0png0.png",
  relPathDir       : DIR_RESOURCES + "distribution/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "exe0.exe",
  relPathDir       : DIR_MACOS,
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_COMPLETE_EXE,
  compareFile      : FILE_PARTIAL_EXE,
  originalPerms    : 0o755,
  comparePerms     : 0o755
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "0exe0.exe",
  relPathDir       : DIR_RESOURCES + "0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_COMPLETE_EXE,
  compareFile      : FILE_PARTIAL_EXE,
  originalPerms    : 0o755,
  comparePerms     : 0o755
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text0",
  relPathDir       : DIR_RESOURCES + "0/00/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "00png0.png",
  relPathDir       : DIR_RESOURCES + "0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20text0",
  relPathDir       : DIR_RESOURCES + "2/20/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20png0.png",
  relPathDir       : DIR_RESOURCES + "2/20/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "partial.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text2",
  relPathDir       : DIR_RESOURCES + "0/00/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "10text0",
  relPathDir       : DIR_RESOURCES + "1/10/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "00text1",
  relPathDir       : DIR_RESOURCES + "0/00/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}];


gTestFilesPartialSuccess = gTestFilesPartialSuccess.concat(gTestFilesCommon);

var gTestDirsCommon = [
{
  relPathDir   : DIR_RESOURCES + "3/",
  dirRemoved   : false,
  files        : ["3text0", "3text1"],
  filesRemoved : true
}, {
  relPathDir   : DIR_RESOURCES + "4/",
  dirRemoved   : true,
  files        : ["4text0", "4text1"],
  filesRemoved : true
}, {
  relPathDir   : DIR_RESOURCES + "5/",
  dirRemoved   : true,
  files        : ["5test.exe", "5text0", "5text1"],
  filesRemoved : true
}, {
  relPathDir   : DIR_RESOURCES + "6/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "7/",
  dirRemoved   : true,
  files        : ["7text0", "7text1"],
  subDirs      : ["70/", "71/"],
  subDirFiles  : ["7xtest.exe", "7xtext0", "7xtext1"]
}, {
  relPathDir   : DIR_RESOURCES + "8/",
  dirRemoved   : false
}, {
  relPathDir   : DIR_RESOURCES + "8/80/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "8/81/",
  dirRemoved   : false,
  files        : ["81text0", "81text1"]
}, {
  relPathDir   : DIR_RESOURCES + "8/82/",
  dirRemoved   : false,
  subDirs      : ["820/", "821/"]
}, {
  relPathDir   : DIR_RESOURCES + "8/83/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "8/84/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "8/85/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "8/86/",
  dirRemoved   : true,
  files        : ["86text0", "86text1"]
}, {
  relPathDir   : DIR_RESOURCES + "8/87/",
  dirRemoved   : true,
  subDirs      : ["870/", "871/"],
  subDirFiles  : ["87xtext0", "87xtext1"]
}, {
  relPathDir   : DIR_RESOURCES + "8/88/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "8/89/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "9/90/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "9/91/",
  dirRemoved   : false,
  files        : ["91text0", "91text1"]
}, {
  relPathDir   : DIR_RESOURCES + "9/92/",
  dirRemoved   : false,
  subDirs      : ["920/", "921/"]
}, {
  relPathDir   : DIR_RESOURCES + "9/93/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "9/94/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "9/95/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "9/96/",
  dirRemoved   : true,
  files        : ["96text0", "96text1"]
}, {
  relPathDir   : DIR_RESOURCES + "9/97/",
  dirRemoved   : true,
  subDirs      : ["970/", "971/"],
  subDirFiles  : ["97xtext0", "97xtext1"]
}, {
  relPathDir   : DIR_RESOURCES + "9/98/",
  dirRemoved   : true
}, {
  relPathDir   : DIR_RESOURCES + "9/99/",
  dirRemoved   : true
}];



var gTestDirsCompleteSuccess = [
{
  description  : "Removed by precomplete (rmdir)",
  relPathDir   : DIR_RESOURCES + "2/20/",
  dirRemoved   : true
}, {
  description  : "Removed by precomplete (rmdir)",
  relPathDir   : DIR_RESOURCES + "2/",
  dirRemoved   : true
}];


gTestDirsCompleteSuccess = gTestDirsCommon.concat(gTestDirsCompleteSuccess);



var gTestDirsPartialSuccess = [
{
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : DIR_RESOURCES + "1/10/",
  dirRemoved   : true
}, {
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : DIR_RESOURCES + "1/",
  dirRemoved   : true
}];


gTestDirsPartialSuccess = gTestDirsCommon.concat(gTestDirsPartialSuccess);



if (MOZ_APP_NAME == "xulrunner") {
  try {
    gDefaultPrefBranch.getCharPref(PREF_APP_UPDATE_CHANNEL);
  } catch (e) {
    setUpdateChannel("test_channel");
  }
}




function setupTestCommon() {
  debugDump("start - general test setup");

  do_test_pending();

  if (gTestID) {
    do_throw("setupTestCommon should only be called once!");
  }

  let caller = Components.stack.caller;
  gTestID = caller.filename.toString().split("/").pop().split(".")[0];

  if (DEBUG_TEST_LOG) {
    let logFile = do_get_file(gTestID + ".log", true);
    if (logFile.exists()) {
      gPassed = false;
      logTestInfo("start - dumping previous test run log");
      logTestInfo("\n" + readFile(logFile) + "\n");
      logTestInfo("finish - dumping previous test run log");
      if (gDeleteLogFile) {
        logFile.remove(false);
      }
      do_throw("The parallel run of this test failed. Failing non-parallel " +
               "test so the log from the parallel run can be displayed in " +
               "non-parallel log.")
    } else {
      gRealDump = dump;
      dump = dumpOverride;
    }
  }

  
  Services.prefs.setBoolPref(PREF_APP_UPDATE_SILENT, true);

  gGREDirOrig = getGREDir();
  gGREBinDirOrig = getGREBinDir();
  gAppDirOrig = getAppBaseDir();

  let applyDir = getApplyDirFile(null, true).parent;

  
  
  
  if (applyDir.exists()) {
    debugDump("attempting to remove directory. Path: " + applyDir.path);
    try {
      removeDirRecursive(applyDir);
    } catch (e) {
      logTestInfo("non-fatal error removing directory. Path: " +
                  applyDir.path + ", Exception: " + e);
    }
  }

  
  
  adjustGeneralPaths();

  
  
  
  
  if (IS_WIN || IS_MACOSX) {
    let updatesDir = getMockUpdRootD();
    if (updatesDir.exists()) {
      debugDump("attempting to remove directory. Path: " + updatesDir.path);
      try {
        removeDirRecursive(updatesDir);
      } catch (e) {
        logTestInfo("non-fatal error removing directory. Path: " +
                    updatesDir.path + ", Exception: " + e);
      }
    }
  }

  debugDump("finish - general test setup");
}





function cleanupTestCommon() {
  debugDump("start - general test cleanup");

  
  
  reloadUpdateManagerData();

  if (gChannel) {
    gPrefRoot.removeObserver(PREF_APP_UPDATE_CHANNEL, observer);
  }

  
  
  
  
  
  gAUS.observe(null, "xpcom-shutdown", "");

  gTestserver = null;

  if (IS_UNIX) {
    
    getLaunchScript();
  }

  if (IS_WIN && MOZ_APP_BASENAME) {
    let appDir = getApplyDirFile(null, true);
    let vendor = MOZ_APP_VENDOR ? MOZ_APP_VENDOR : "Mozilla";
    const REG_PATH = "SOFTWARE\\" + vendor + "\\" + MOZ_APP_BASENAME +
                     "\\TaskBarIDs";
    let key = Cc["@mozilla.org/windows-registry-key;1"].
              createInstance(Ci.nsIWindowsRegKey);
    try {
      key.open(Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE, REG_PATH,
               Ci.nsIWindowsRegKey.ACCESS_ALL);
      if (key.hasValue(appDir.path)) {
        key.removeValue(appDir.path);
      }
    } catch (e) {
    }
    try {
      key.open(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER, REG_PATH,
               Ci.nsIWindowsRegKey.ACCESS_ALL);
      if (key.hasValue(appDir.path)) {
        key.removeValue(appDir.path);
      }
    } catch (e) {
    }
  }

  
  
  if (IS_WIN || IS_MACOSX) {
    let updatesDir = getMockUpdRootD();
    
    
    if (updatesDir.exists()) {
      debugDump("attempting to remove directory. Path: " + updatesDir.path);
      try {
        removeDirRecursive(updatesDir);
      } catch (e) {
        logTestInfo("non-fatal error removing directory. Path: " +
                    updatesDir.path + ", Exception: " + e);
      }
      if (IS_MACOSX) {
        let updatesRootDir = gUpdatesRootDir.clone();
        while (updatesRootDir.path != updatesDir.path) {
          if (updatesDir.exists()) {
            debugDump("attempting to remove directory. Path: " +
                      updatesDir.path);
            try {
              
              
              
              
              updatesDir.remove(false);
            } catch (e) {
              logTestInfo("non-fatal error removing directory. Path: " +
                          updatesDir.path + ", Exception: " + e);
            }
          }
          updatesDir = updatesDir.parent;
        }
      }
    }
  }

  let applyDir = getApplyDirFile(null, true).parent;

  
  
  if (applyDir.exists()) {
    debugDump("attempting to remove directory. Path: " + applyDir.path);
    try {
      removeDirRecursive(applyDir);
    } catch (e) {
      logTestInfo("non-fatal error removing directory. Path: " +
                  applyDir.path + ", Exception: " + e);
    }
  }

  resetEnvironment();

  debugDump("finish - general test cleanup");

  if (gRealDump) {
    dump = gRealDump;
    gRealDump = null;
  }

  if (DEBUG_TEST_LOG && !gPassed) {
    let fos = Cc["@mozilla.org/network/file-output-stream;1"].
              createInstance(Ci.nsIFileOutputStream);
    let logFile = do_get_file(gTestID + ".log", true);
    if (!logFile.exists()) {
      logFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
    }
    fos.init(logFile, MODE_WRONLY | MODE_CREATE | MODE_APPEND, PERMS_FILE, 0);
    fos.write(gTestLogText, gTestLogText.length);
    fos.close();
  }

  if (DEBUG_TEST_LOG) {
    gTestLogText = null;
  } else {
    let logFile = do_get_file(gTestID + ".log", true);
    if (logFile.exists()) {
      logFile.remove(false);
    }
  }
}






function dumpOverride(aText) {
  gTestLogText += aText;
  gRealDump(aText);
}






function doTestFinish() {
  if (gPassed === undefined) {
    gPassed = true;
  }
  do_test_finished();
}




function setDefaultPrefs() {
  Services.prefs.setBoolPref(PREF_APP_UPDATE_ENABLED, true);
  Services.prefs.setBoolPref(PREF_APP_UPDATE_METRO_ENABLED, true);
  
  
  Services.prefs.setBoolPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI, false);
  if (DEBUG_AUS_TEST) {
    
    Services.prefs.setBoolPref(PREF_APP_UPDATE_LOG, true);
  }
}





function setTestFilesAndDirsForFailure() {
  gTestFiles.forEach(function STFADFF_Files(aTestFile) {
    aTestFile.compareContents = aTestFile.originalContents;
    aTestFile.compareFile = aTestFile.originalFile;
    aTestFile.comparePerms = aTestFile.originalPerms;
  });

  gTestDirs.forEach(function STFADFF_Dirs(aTestDir) {
    aTestDir.dirRemoved = false;
    if (aTestDir.filesRemoved) {
      aTestDir.filesRemoved = false;
    }
  });
}





function preventDistributionFiles() {
  gTestFiles = gTestFiles.filter(function(aTestFile) {
    return aTestFile.relPathDir.indexOf("distribution/") == -1;
  });

  gTestDirs = gTestDirs.filter(function(aTestDir) {
    return aTestDir.relPathDir.indexOf("distribution/") == -1;
  });
}






function standardInit() {
  createAppInfo("xpcshell@tests.mozilla.org", APP_INFO_NAME, "1.0", "2.0");
  setDefaultPrefs();
  
  initUpdateServiceStub();
}









function pathHandler(aMetadata, aResponse) {
  aResponse.setHeader("Content-Type", "text/xml", false);
  aResponse.setStatusLine(aMetadata.httpVersion, gResponseStatusCode, "OK");
  aResponse.bodyOutputStream.write(gResponseBody, gResponseBody.length);
}









function getAppVersion() {
  
  let iniFile = gGREDirOrig.clone();
  iniFile.append(FILE_APPLICATION_INI);
  if (!iniFile.exists()) {
    iniFile = gGREBinDirOrig.clone();
    iniFile.append(FILE_APPLICATION_INI);
    if (!iniFile.exists()) {
      do_throw("Unable to find application.ini!");
    }
  }
  let iniParser = Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
                  getService(Ci.nsIINIParserFactory).
                  createINIParser(iniFile);
  return iniParser.getString("App", "Version");
}













function getApplyDirPath() {
  return gTestID + "/dir.app/";
}






















function getApplyDirFile(aRelPath, aAllowNonexistent) {
  let relpath = getApplyDirPath() + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, aAllowNonexistent);
}






















function getStageDirFile(aRelPath, aAllowNonexistent) {
  if (IS_MACOSX) {
    let file = getMockUpdRootD();
    file.append(DIR_UPDATES);
    file.append(DIR_PATCH);
    file.append(DIR_UPDATED);
    if (aRelPath) {
      let pathParts = aRelPath.split("/");
      for (let i = 0; i < pathParts.length; i++) {
        if (pathParts[i]) {
          file.append(pathParts[i]);
        }
      }
    }
    if (!aAllowNonexistent && !file.exists()) {
      do_throw(file.path + " does not exist");
    }
    return file;
  }

  let relpath = getApplyDirPath() + DIR_UPDATED + "/" + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, aAllowNonexistent);
}








function getTestDirPath() {
  return "../data/";
}












function getTestDirFile(aRelPath) {
  let relpath = getTestDirPath() + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, false);
}

function getSpecialFolderDir(aCSIDL) {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  let lib = ctypes.open("shell32");
  let SHGetSpecialFolderPath = lib.declare("SHGetSpecialFolderPathW",
                                           ctypes.winapi_abi,
                                           ctypes.bool, 
                                           ctypes.int32_t, 
                                           ctypes.char16_t.ptr, 
                                           ctypes.int32_t, 
                                           ctypes.bool );

  let aryPath = ctypes.char16_t.array()(260);
  let rv = SHGetSpecialFolderPath(0, aryPath, aCSIDL, false);
  lib.close();

  let path = aryPath.readString(); 
  if (!path) {
    return null;
  }
  debugDump("SHGetSpecialFolderPath returned path: " + path);
  let dir = Cc["@mozilla.org/file/local;1"].
            createInstance(Ci.nsILocalFile);
  dir.initWithPath(path);
  return dir;
}

XPCOMUtils.defineLazyGetter(this, "gInstallDirPathHash",
                            function test_gInstallDirPathHash() {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  
  if (!MOZ_APP_BASENAME) {
    return null;
  }

  let vendor = MOZ_APP_VENDOR ? MOZ_APP_VENDOR : "Mozilla";
  let appDir = getApplyDirFile(null, true);

  const REG_PATH = "SOFTWARE\\" + vendor + "\\" + MOZ_APP_BASENAME +
                   "\\TaskBarIDs";
  let regKey = Cc["@mozilla.org/windows-registry-key;1"].
               createInstance(Ci.nsIWindowsRegKey);
  try {
    regKey.open(Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE, REG_PATH,
                Ci.nsIWindowsRegKey.ACCESS_ALL);
    regKey.writeStringValue(appDir.path, gTestID);
    return gTestID;
  } catch (e) {
  }

  try {
    regKey.create(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER, REG_PATH,
                  Ci.nsIWindowsRegKey.ACCESS_ALL);
    regKey.writeStringValue(appDir.path, gTestID);
    return gTestID;
  } catch (e) {
    logTestInfo("failed to create registry key. Registry Path: " + REG_PATH +
                ", Key Name: " + appDir.path + ", Key Value: " + gTestID +
                ", Exception " + e);
  }
  return null;
});

XPCOMUtils.defineLazyGetter(this, "gLocalAppDataDir",
                            function test_gLocalAppDataDir() {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  const CSIDL_LOCAL_APPDATA = 0x1c;
  return getSpecialFolderDir(CSIDL_LOCAL_APPDATA);
});

XPCOMUtils.defineLazyGetter(this, "gProgFilesDir",
                            function test_gProgFilesDir() {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  const CSIDL_PROGRAM_FILES = 0x26;
  return getSpecialFolderDir(CSIDL_PROGRAM_FILES);
});







function getMockUpdRootD() {
  if (IS_WIN) {
    return getMockUpdRootDWin();
  }

  if (IS_MACOSX) {
    return getMockUpdRootDMac();
  }

  return getApplyDirFile(DIR_MACOS, true);
}







function getMockUpdRootDWin() {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  let localAppDataDir = gLocalAppDataDir.clone();
  let progFilesDir = gProgFilesDir.clone();
  let appDir = Services.dirsvc.get(XRE_EXECUTABLE_FILE, Ci.nsIFile).parent;

  let appDirPath = appDir.path;
  let relPathUpdates = "";
  if (gInstallDirPathHash && (MOZ_APP_VENDOR || MOZ_APP_BASENAME)) {
    relPathUpdates += (MOZ_APP_VENDOR ? MOZ_APP_VENDOR : MOZ_APP_BASENAME) +
                      "\\" + DIR_UPDATES + "\\" + gInstallDirPathHash;
  }

  if (!relPathUpdates && progFilesDir) {
    if (appDirPath.length > progFilesDir.path.length) {
      if (appDirPath.substr(0, progFilesDir.path.length) == progFilesDir.path) {
        if (MOZ_APP_VENDOR && MOZ_APP_BASENAME) {
          relPathUpdates += MOZ_APP_VENDOR + "\\" + MOZ_APP_BASENAME;
        } else {
          relPathUpdates += MOZ_APP_BASENAME;
        }
        relPathUpdates += appDirPath.substr(progFilesDir.path.length);
      }
    }
  }

  if (!relPathUpdates) {
    if (MOZ_APP_VENDOR && MOZ_APP_BASENAME) {
      relPathUpdates += MOZ_APP_VENDOR + "\\" + MOZ_APP_BASENAME;
    } else {
      relPathUpdates += MOZ_APP_BASENAME;
    }
    relPathUpdates += "\\" + MOZ_APP_NAME;
  }

  let updatesDir = Cc["@mozilla.org/file/local;1"].
                   createInstance(Ci.nsILocalFile);
  updatesDir.initWithPath(localAppDataDir.path + "\\" + relPathUpdates);
  debugDump("returning UpdRootD Path: " + updatesDir.path);
  return updatesDir;
}

XPCOMUtils.defineLazyGetter(this, "gUpdatesRootDir",
                            function test_gUpdatesRootDir() {
  if (!IS_MACOSX) {
    do_throw("Mac OS X only function called by a different platform!");
  }

  let dir = Services.dirsvc.get("ULibDir", Ci.nsILocalFile);
  dir.append("Caches");
  if (MOZ_APP_VENDOR || MOZ_APP_BASENAME) {
    dir.append(MOZ_APP_VENDOR ? MOZ_APP_VENDOR : MOZ_APP_BASENAME);
  } else {
    dir.append("Mozilla");
  }
  dir.append(DIR_UPDATES);
  return dir;
});







function getMockUpdRootDMac() {
  if (!IS_MACOSX) {
    do_throw("Mac OS X only function called by a different platform!");
  }

  let appDir = Services.dirsvc.get(XRE_EXECUTABLE_FILE, Ci.nsIFile).
               parent.parent.parent;
  let appDirPath = appDir.path;
  appDirPath = appDirPath.substr(0, appDirPath.length - 4);

  let pathUpdates = gUpdatesRootDir.path + appDirPath;
  let updatesDir = Cc["@mozilla.org/file/local;1"].
                   createInstance(Ci.nsILocalFile);
  updatesDir.initWithPath(pathUpdates);
  debugDump("returning UpdRootD Path: " + updatesDir.path);
  return updatesDir;
}

const kLockFileName = "updated.update_in_progress.lock";






function lockDirectory(aDir) {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  let file = aDir.clone();
  file.append(kLockFileName);
  file.create(file.NORMAL_FILE_TYPE, 0o444);
  file.QueryInterface(Ci.nsILocalFileWin);
  file.fileAttributesWin |= file.WFA_READONLY;
  file.fileAttributesWin &= ~file.WFA_READWRITE;
  debugDump("testing the successful creation of the lock file");
  do_check_true(file.exists());
  do_check_false(file.isWritable());
}






function unlockDirectory(aDir) {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }
  let file = aDir.clone();
  file.append(kLockFileName);
  file.QueryInterface(Ci.nsILocalFileWin);
  file.fileAttributesWin |= file.WFA_READWRITE;
  file.fileAttributesWin &= ~file.WFA_READONLY;
  debugDump("removing and testing the successful removal of the lock file");
  file.remove(false);
  do_check_false(file.exists());
}















function runUpdate(aExpectedExitValue, aExpectedStatus, aCallback) {
  
  let binDir = gGREBinDirOrig.clone();
  let updater = binDir.clone();
  updater.append("updater.app");
  if (!updater.exists()) {
    updater = binDir.clone();
    updater.append(FILE_UPDATER_BIN);
    if (!updater.exists()) {
      do_throw("Unable to find updater binary!");
    }
  }

  let updatesDir = getUpdatesPatchDir();
  updater.copyToFollowingLinks(updatesDir, updater.leafName);
  let updateBin = updatesDir.clone();
  updateBin.append(updater.leafName);
  if (updateBin.leafName == "updater.app") {
    updateBin.append("Contents");
    updateBin.append("MacOS");
    updateBin.append("updater");
    if (!updateBin.exists()) {
      do_throw("Unable to find the updater executable!");
    }
  }

  let applyToDir = getApplyDirFile(null, true);
  let applyToDirPath = applyToDir.path;

  let stageDir = getStageDirFile(null, true);
  let stageDirPath = stageDir.path;

  if (IS_WIN) {
    
    applyToDirPath = applyToDirPath.replace(/\//g, "\\");
    stageDirPath = stageDirPath.replace(/\//g, "\\");
  }

  let callbackApp = getApplyDirFile(DIR_RESOURCES + gCallbackBinFile);
  callbackApp.permissions = PERMS_DIRECTORY;

  let args = [updatesDir.path, applyToDirPath];
  if (gStageUpdate) {
    args[2] = stageDirPath;
    args[3] = -1;
  } else {
    if (gSwitchApp) {
      args[2] = stageDirPath;
      args[3] = "0/replace";
    } else {
      args[2] = applyToDirPath;
      args[3] = "0";
    }
    args = args.concat([callbackApp.parent.path, callbackApp.path]);
    args = args.concat(gCallbackArgs);
  }
  debugDump("running the updater: " + updateBin.path + " " + args.join(" "));

  let env = Cc["@mozilla.org/process/environment;1"].
            getService(Ci.nsIEnvironment);
  if (gDisableReplaceFallback) {
    env.set("MOZ_NO_REPLACE_FALLBACK", "1");
  }

  let process = Cc["@mozilla.org/process/util;1"].
                createInstance(Ci.nsIProcess);
  process.init(updateBin);
  process.run(true, args, args.length);

  if (gDisableReplaceFallback) {
    env.set("MOZ_NO_REPLACE_FALLBACK", "");
  }

  let status = readStatusFile();
  if (process.exitValue != aExpectedExitValue || status != aExpectedStatus) {
    if (process.exitValue != aExpectedExitValue) {
      logTestInfo("updater exited with unexpected value! Got: " +
                  process.exitValue + ", Expected: " +  aExpectedExitValue);
    }
    if (status != aExpectedStatus) {
      logTestInfo("update status is not the expected status! Got: " + status +
                  ", Expected: " +  aExpectedStatus);
    }
    let updateLog = getUpdatesPatchDir();
    updateLog.append(FILE_UPDATE_LOG);
    
    let contents = readFileBytes(updateLog).replace(/\r\n/g, "\n");
    let aryLogContents = contents.split("\n");
    logTestInfo("contents of " + updateLog.path + ":");
    aryLogContents.forEach(function RU_LC_FE(aLine) {
      logTestInfo(aLine);
    });
  }
  debugDump("testing updater binary process exitValue against expected " +
            "exit value");
  do_check_eq(process.exitValue, aExpectedExitValue);
  debugDump("testing update status against expected status");
  do_check_eq(status, aExpectedStatus);

  if (aCallback !== null) {
    if (typeof(aCallback) == typeof(Function)) {
      aCallback();
    } else {
      checkUpdateApplied();
    }
  }
}




function stageUpdate() {
  debugDump("start - attempting to stage update");
  Services.obs.addObserver(gUpdateStagedObserver, "update-staged", false);

  setEnvironment();
  
  Cc["@mozilla.org/updates/update-processor;1"].
    createInstance(Ci.nsIUpdateProcessor).
    processUpdate(gUpdateManager.activeUpdate);
  resetEnvironment();

  debugDump("finish - attempting to stage update");
}









function shouldRunServiceTest(aFirstTest) {
  let binDir = getGREBinDir();
  let updaterBin = binDir.clone();
  updaterBin.append(FILE_UPDATER_BIN);
  if (!updaterBin.exists()) {
    do_throw("Unable to find updater binary!");
  }

  let updaterBinPath = updaterBin.path;
  if (/ /.test(updaterBinPath)) {
    updaterBinPath = '"' + updaterBinPath + '"';
  }

  const REG_PATH = "SOFTWARE\\Mozilla\\MaintenanceService\\" +
                   "3932ecacee736d366d6436db0f55bce4";

  let key = Cc["@mozilla.org/windows-registry-key;1"].
            createInstance(Ci.nsIWindowsRegKey);
  try {
    key.open(Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE, REG_PATH,
             Ci.nsIWindowsRegKey.ACCESS_READ | key.WOW64_64);
  } catch (e) {
    
    
    if (IS_AUTHENTICODE_CHECK_ENABLED && isBinarySigned(updaterBinPath)) {
      do_throw("binary is signed but the test registry key does not exists!");
    }

    logTestInfo("this test can only run on the buildbot build system at this " +
                "time.");
    return false;
  }

  
  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let args = ["wait-for-service-stop", "MozillaMaintenance", "10"];
  let process = Cc["@mozilla.org/process/util;1"].
                createInstance(Ci.nsIProcess);
  process.init(helperBin);
  debugDump("checking if the service exists on this machine.");
  process.run(true, args, args.length);
  if (process.exitValue == 0xEE) {
    do_throw("test registry key exists but this test can only run on systems " +
             "with the maintenance service installed.");
  } else {
    debugDump("service exists, return value: " + process.exitValue);
  }

  
  
  
  if (aFirstTest && process.exitValue != 0) {
    do_throw("First test, check for service stopped state returned error " +
             process.exitValue);
  }

  if (IS_AUTHENTICODE_CHECK_ENABLED && !isBinarySigned(updaterBinPath)) {
    logTestInfo("test registry key exists but this test can only run on " +
                "builds with signed binaries when " +
                "DISABLE_UPDATER_AUTHENTICODE_CHECK is not defined");
    do_throw("this test can only run on builds with signed binaries.");
  }

  
  
  
  return attemptServiceInstall();
}







function isBinarySigned(aBinPath) {
  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let args = ["check-signature", aBinPath];
  let process = Cc["@mozilla.org/process/util;1"].
                createInstance(Ci.nsIProcess);
  process.init(helperBin);
  process.run(true, args, args.length);
  if (process.exitValue != 0) {
    logTestInfo("binary is not signed. " + FILE_HELPER_BIN + " returned " +
                process.exitValue + " for file " + aBinPath);
    return false;
  }
  return true;
}








function setupAppFilesAsync() {
  gTimeoutRuns++;
  try {
    setupAppFiles();
  } catch (e) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while trying to setup application " +
               "files. Exception: " + e);
    }
    do_timeout(TEST_CHECK_TIMEOUT, setupAppFilesAsync);
    return;
  }

  setupAppFilesFinished();
}






function setupAppFiles() {
  debugDump("start - copying or creating symlinks to application files " +
            "for the test");

  let destDir = getApplyDirFile(null, true);
  if (!destDir.exists()) {
    try {
      destDir.create(Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
    } catch (e) {
      logTestInfo("unable to create directory, Path: " + destDir.path +
                  ", Exception: " + e);
      do_throw(e);
    }
  }

  
  
  let appFiles = [ { relPath  : FILE_APP_BIN,
                     inGreDir : false },
                   { relPath  : FILE_UPDATER_BIN,
                     inGreDir : false },
                   { relPath  : FILE_APPLICATION_INI,
                     inGreDir : true },
                   { relPath  : "dependentlibs.list",
                     inGreDir : true } ];

  
  if (IS_UNIX && !IS_MACOSX) {
    appFiles.push( { relPath  : "icons/updater.png",
                     inGreDir : true } );
  }

  
  
  let deplibsFile = gGREDirOrig.clone();
  deplibsFile.append("dependentlibs.list");
  let istream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  istream.init(deplibsFile, 0x01, 0o444, 0);
  istream.QueryInterface(Ci.nsILineInputStream);

  let hasMore;
  let line = {};
  do {
    hasMore = istream.readLine(line);
    appFiles.push( { relPath  : line.value,
                     inGreDir : false } );
  } while(hasMore);

  istream.close();

  appFiles.forEach(function CMAF_FLN_FE(aAppFile) {
    copyFileToTestAppDir(aAppFile.relPath, aAppFile.inGreDir);
  });

  debugDump("finish - copying or creating symlinks to application files " +
            "for the test");
}















function copyFileToTestAppDir(aFileRelPath, aInGreDir) {
  
  
  let srcFile = aInGreDir ? gGREDirOrig.clone() : gGREBinDirOrig.clone();
  let destFile = aInGreDir ? getGREDir() : getGREBinDir();
  let fileRelPath = aFileRelPath;
  let pathParts = fileRelPath.split("/");
  for (let i = 0; i < pathParts.length; i++) {
    if (pathParts[i]) {
      srcFile.append(pathParts[i]);
      destFile.append(pathParts[i]);
    }
  }

  if (IS_MACOSX && !srcFile.exists()) {
    debugDump("unable to copy file since it doesn't exist! Checking if " +
              fileRelPath + ".app exists. Path: " + srcFile.path);
    
    
    srcFile = aInGreDir ? gGREDirOrig.clone() : gGREBinDirOrig.clone();
    destFile = aInGreDir ? getGREDir() : getGREBinDir();
    for (let i = 0; i < pathParts.length; i++) {
      if (pathParts[i]) {
        srcFile.append(pathParts[i] + (pathParts.length - 1 == i ? ".app" : ""));
        destFile.append(pathParts[i] + (pathParts.length - 1 == i ? ".app" : ""));
      }
    }
    fileRelPath = fileRelPath + ".app";
  }

  if (!srcFile.exists()) {
    do_throw("Unable to copy file since it doesn't exist! Path: " +
             srcFile.path);
  }

  
  
  let shouldSymlink = (pathParts[pathParts.length - 1] == "XUL" ||
                       fileRelPath.substr(fileRelPath.length - 3) == ".so" ||
                       fileRelPath.substr(fileRelPath.length - 6) == ".dylib");
  if (!shouldSymlink) {
    if (!destFile.exists()) {
      try {
        srcFile.copyToFollowingLinks(destFile.parent, destFile.leafName);
      } catch (e) {
        
        if (destFile.exists()) {
          try {
            destFile.remove(true);
          } catch (e) {
            logTestInfo("unable to remove file that failed to copy! Path: " +
                        destFile.path);
          }
        }
        do_throw("Unable to copy file! Path: " + srcFile.path +
                 ", Exception: " + e);
      }
    }
  } else {
    try {
      if (destFile.exists()) {
        destFile.remove(false);
      }
      let ln = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      ln.initWithPath("/bin/ln");
      let process = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
      process.init(ln);
      let args = ["-s", srcFile.path, destFile.path];
      process.run(true, args, args.length);
      debugDump("verifying symlink. Path: " + destFile.path);
      do_check_true(destFile.isSymlink());
    } catch (e) {
      do_throw("Unable to create symlink for file! Path: " + srcFile.path +
               ", Exception: " + e);
    }
  }
}







function attemptServiceInstall() {
  const CSIDL_PROGRAM_FILES = 0x26;
  const CSIDL_PROGRAM_FILESX86 = 0x2A;
  
  let maintSvcDir = getSpecialFolderDir(CSIDL_PROGRAM_FILESX86);
  if (maintSvcDir) {
    maintSvcDir.append("Mozilla Maintenance Service");
    debugDump("using CSIDL_PROGRAM_FILESX86 - maintenance service install " +
              "directory path: " + maintSvcDir.path);
  }
  if (!maintSvcDir || !maintSvcDir.exists()) {
    maintSvcDir = getSpecialFolderDir(CSIDL_PROGRAM_FILES);
    if (maintSvcDir) {
      maintSvcDir.append("Mozilla Maintenance Service");
      debugDump("using CSIDL_PROGRAM_FILES - maintenance service install " +
                "directory path: " + maintSvcDir.path);
    }
  }
  if (!maintSvcDir || !maintSvcDir.exists()) {
    do_throw("maintenance service install directory doesn't exist!");
  }
  let oldMaintSvcBin = maintSvcDir.clone();
  oldMaintSvcBin.append(FILE_MAINTENANCE_SERVICE_BIN);
  if (!oldMaintSvcBin.exists()) {
    do_throw("maintenance service install directory binary doesn't exist! " +
             "Path: " + oldMaintSvcBin.path);
  }
  let buildMaintSvcBin = getGREBinDir();
  buildMaintSvcBin.append(FILE_MAINTENANCE_SERVICE_BIN);
  if (readFileBytes(oldMaintSvcBin) == readFileBytes(buildMaintSvcBin)) {
    debugDump("installed maintenance service binary is the same as the " +
              "build's maintenance service binary");
    return true;
  }
  let backupMaintSvcBin = maintSvcDir.clone();
  backupMaintSvcBin.append(FILE_MAINTENANCE_SERVICE_BIN + ".backup");
  try {
    if (backupMaintSvcBin.exists()) {
      backupMaintSvcBin.remove(false);
    }
    oldMaintSvcBin.moveTo(maintSvcDir, FILE_MAINTENANCE_SERVICE_BIN + ".backup");
    buildMaintSvcBin.copyTo(maintSvcDir, FILE_MAINTENANCE_SERVICE_BIN);
    backupMaintSvcBin.remove(false);
  } catch (e) {
    
    if (backupMaintSvcBin.exists()) {
      oldMaintSvcBin = maintSvcDir.clone();
      oldMaintSvcBin.append(FILE_MAINTENANCE_SERVICE_BIN);
      if (!oldMaintSvcBin.exists()) {
        backupMaintSvcBin.moveTo(maintSvcDir, FILE_MAINTENANCE_SERVICE_BIN);
      }
    }
    logTestInfo("unable to copy new maintenance service into the " +
                "maintenance service directory: " + maintSvcDir.path + ", " +
                "Exception: " + e);
    do_throw("The account running the tests on the build systems should have " +
             "write access to the maintenance service directory!");
  }

  return true;
}












function runUpdateUsingService(aInitialStatus, aExpectedStatus, aCheckSvcLog) {
  
  function checkServiceLogs(aOriginalContents) {
    let contents = readServiceLogFile();
    debugDump("the contents of maintenanceservice.log:\n" + contents + "\n");
    do_check_neq(contents, aOriginalContents);
    do_check_neq(contents.indexOf(LOG_SVC_SUCCESSFUL_LAUNCH), -1);
  }
  function readServiceLogFile() {
    let file = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties).
               get("CmAppData", Ci.nsIFile);
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
    debugDump("waiting for service to stop if necessary...");
    
    
    let helperBin = getTestDirFile(FILE_HELPER_BIN);
    let helperBinArgs = ["wait-for-service-stop",
                         "MozillaMaintenance",
                         "120"];
    let helperBinProcess = Cc["@mozilla.org/process/util;1"].
                           createInstance(Ci.nsIProcess);
    helperBinProcess.init(helperBin);
    debugDump("stopping service...");
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
      debugDump("service stopped");
    }
    waitServiceApps();
  }
  function waitForApplicationStop(aApplication) {
    debugDump("waiting for " + aApplication + " to stop if necessary..");
    
    
    let helperBin = getTestDirFile(FILE_HELPER_BIN);
    let helperBinArgs = ["wait-for-application-exit",
                         aApplication,
                         "120"];
    let helperBinProcess = Cc["@mozilla.org/process/util;1"].
                           createInstance(Ci.nsIProcess);
    helperBinProcess.init(helperBin);
    helperBinProcess.run(true, helperBinArgs, helperBinArgs.length);
    if (helperBinProcess.exitValue != 0) {
      do_throw(aApplication + " did not stop, last state: " +
               helperBinProcess.exitValue + ". Forcing test failure.");
    }
  }

  
  waitForServiceStop(true);

  
  if (gRegisteredServiceCleanup === undefined) {
    gRegisteredServiceCleanup = true;

    do_register_cleanup(function RUUS_cleanup() {
      resetEnvironment();

      
      try {
        getAppArgsLogPath();
      } catch (e) {
        logTestInfo("unable to remove file during cleanup. Exception: " + e);
      }
    });
  }

  let svcOriginalLog;
  
  if (aCheckSvcLog === undefined || aCheckSvcLog) {
    svcOriginalLog = readServiceLogFile();
  }

  let appArgsLogPath = getAppArgsLogPath();
  gServiceLaunchedCallbackLog = appArgsLogPath.replace(/^"|"$/g, "");

  let updatesDir = getUpdatesPatchDir();
  writeStatusFile(aInitialStatus);

  
  do_check_eq(readStatusState(), aInitialStatus);

  writeVersionFile(DEFAULT_UPDATE_VERSION);

  gServiceLaunchedCallbackArgs = [
    "-no-remote",
    "-process-updates",
    "-dump-args",
    appArgsLogPath
  ];

  if (gSwitchApp) {
    
    gShouldResetEnv = undefined;
  }

  setEnvironment();

  
  
  
  copyFileToTestAppDir(FILE_UPDATER_BIN, false);

  
  
  
  
  copyFileToTestAppDir(FILE_MAINTENANCE_SERVICE_BIN, false);
  copyFileToTestAppDir(FILE_MAINTENANCE_SERVICE_INSTALLER_BIN, false);

  let launchBin = getLaunchBin();
  let args = getProcessArgs(["-dump-args", appArgsLogPath]);

  let process = Cc["@mozilla.org/process/util;1"].
                createInstance(Ci.nsIProcess);
  process.init(launchBin);
  debugDump("launching " + launchBin.path + " " + args.join(" "));
  
  
  
  
  
  process.run(true, args, args.length);

  resetEnvironment();

  function timerCallback(aTimer) {
    
    let status = readStatusFile();
    
    
    
    if (status == STATE_APPLYING ||
        status == STATE_PENDING_SVC) {
      debugDump("still waiting to see the " + aExpectedStatus +
                " status, got " + status + " for now...");
      return;
    }

    
    waitForServiceStop(false);

    aTimer.cancel();
    aTimer = null;

    if (status != aExpectedStatus) {
      logTestInfo("update status is not the expected status! Got: " + status +
                  ", Expected: " +  aExpectedStatus);
      logTestInfo("update.status contents: " + readStatusFile());
      let updateLog = getUpdatesPatchDir();
      updateLog.append(FILE_UPDATE_LOG);
      
      let contents = readFileBytes(updateLog).replace(/\r\n/g, "\n");
      let aryLogContents = contents.split("\n");
      logTestInfo("contents of " + updateLog.path + ":");
      aryLogContents.forEach(function RUUS_TC_LC_FE(aLine) {
        logTestInfo(aLine);
      });
    }
    debugDump("testing update status against expected status");
    do_check_eq(status, aExpectedStatus);

    if (aCheckSvcLog) {
      checkServiceLogs(svcOriginalLog);
    }

    checkUpdateFinished();
  }

  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(timerCallback, 1000, timer.TYPE_REPEATING_SLACK);
}










function getLaunchBin() {
  let launchBin;
  if (IS_WIN) {
    launchBin = Services.dirsvc.get("WinD", Ci.nsIFile);
    launchBin.append("System32");
    launchBin.append("cmd.exe");
  } else {
    launchBin = Cc["@mozilla.org/file/local;1"].
                createInstance(Ci.nsILocalFile);
    launchBin.initWithPath("/bin/sh");
  }

  if (!launchBin.exists()) {
    do_throw(launchBin.path + " must exist to run this test!");
  }

  return launchBin;
}





function waitForHelperSleep() {
  gTimeoutRuns++;
  
  
  let output = getApplyDirFile(DIR_RESOURCES + "output", true);
  if (readFile(output) != "sleeping\n") {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the helper to " +
               "finish its operation. Path: " + output.path);
    }
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
    return;
  }
  try {
    output.remove(false);
  }
  catch (e) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the helper " +
               "message file to no longer be in use. Path: " + output.path);
    }
    debugDump("failed to remove file. Path: " + output.path);
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
    return;
  }
  doUpdate();
}






function waitForHelperFinished() {
  
  
  let output = getApplyDirFile(DIR_RESOURCES + "output", true);
  if (readFile(output) != "finished\n") {
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperFinished);
    return;
  }
  
  
  waitForHelperFinishFileUnlock();
}





function waitForHelperFinishFileUnlock() {
  try {
    let output = getApplyDirFile(DIR_RESOURCES + "output", true);
    if (output.exists()) {
      output.remove(false);
    }
    let input = getApplyDirFile(DIR_RESOURCES + "input", true);
    if (input.exists()) {
      input.remove(false);
    }
  } catch (e) {
    
    
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperFinishFileUnlock);
    return;
  }
  checkUpdate();
}




function setupHelperFinish() {
  let input = getApplyDirFile(DIR_RESOURCES + "input", true);
  writeFile(input, "finish\n");
  waitForHelperFinished();
}








function setupUpdaterTest(aMarFile) {
  let updatesPatchDir = getUpdatesPatchDir();
  if (!updatesPatchDir.exists()) {
    updatesPatchDir.create(Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
  }
  
  let mar = getTestDirFile(aMarFile);
  mar.copyToFollowingLinks(updatesPatchDir, FILE_UPDATE_ARCHIVE);

  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let afterApplyBinDir = getApplyDirFile(DIR_RESOURCES, true);
  helperBin.copyToFollowingLinks(afterApplyBinDir, gCallbackBinFile);
  helperBin.copyToFollowingLinks(afterApplyBinDir, gPostUpdateBinFile);

  let applyToDir = getApplyDirFile(null, true);
  gTestFiles.forEach(function SUT_TF_FE(aTestFile) {
    if (aTestFile.originalFile || aTestFile.originalContents) {
      let testDir = getApplyDirFile(aTestFile.relPathDir, true);
      if (!testDir.exists()) {
        testDir.create(Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
      }

      let testFile;
      if (aTestFile.originalFile) {
        testFile = getTestDirFile(aTestFile.originalFile);
        testFile.copyToFollowingLinks(testDir, aTestFile.fileName);
        testFile = getApplyDirFile(aTestFile.relPathDir + aTestFile.fileName);
      } else {
        testFile = getApplyDirFile(aTestFile.relPathDir + aTestFile.fileName,
                                   true);
        writeFile(testFile, aTestFile.originalContents);
      }

      
      
      if (!IS_WIN && aTestFile.originalPerms) {
        testFile.permissions = aTestFile.originalPerms;
        
        
        if (!aTestFile.comparePerms) {
          aTestFile.comparePerms = testFile.permissions;
        }
      }
    }
  });

  
  
  gTestDirs.forEach(function SUT_TD_FE(aTestDir) {
    let testDir = getApplyDirFile(aTestDir.relPathDir, true);
    if (!testDir.exists()) {
      testDir.create(Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
    }

    if (aTestDir.files) {
      aTestDir.files.forEach(function SUT_TD_F_FE(aTestFile) {
        let testFile = getApplyDirFile(aTestDir.relPathDir + aTestFile, true);
        if (!testFile.exists()) {
          testFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
        }
      });
    }

    if (aTestDir.subDirs) {
      aTestDir.subDirs.forEach(function SUT_TD_SD_FE(aSubDir) {
        let testSubDir = getApplyDirFile(aTestDir.relPathDir + aSubDir, true);
        if (!testSubDir.exists()) {
          testSubDir.create(Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
        }

        if (aTestDir.subDirFiles) {
          aTestDir.subDirFiles.forEach(function SUT_TD_SDF_FE(aTestFile) {
            let testFile = getApplyDirFile(aTestDir.relPathDir + aSubDir + aTestFile, true);
            if (!testFile.exists()) {
              testFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
            }
          });
        }
      });
    }
  });
}





function createUpdateSettingsINI() {
  let updateSettingsIni = getApplyDirFile(null, true);
  if (IS_MACOSX) {
    updateSettingsIni.append("Contents");
    updateSettingsIni.append("Resources");
  }
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  writeFile(updateSettingsIni, UPDATE_SETTINGS_CONTENTS);
}










function createUpdaterINI(aIsExeAsync) {
  let exeArg = "ExeArg=post-update-async\n";
  let exeAsync = "";
  if (aIsExeAsync !== undefined) {
    if (aIsExeAsync) {
      exeAsync = "ExeAsync=true\n";
    } else {
      exeArg = "ExeArg=post-update-sync\n";
      exeAsync = "ExeAsync=false\n";
    }
  }

  let updaterIniContents = "[Strings]\n" +
                           "Title=Update Test\n" +
                           "Info=Running update test " + gTestID + "\n\n" +
                           "[PostUpdateMac]\n" +
                           "ExeRelPath=" + DIR_RESOURCES + gPostUpdateBinFile + "\n" +
                           exeArg +
                           exeAsync +
                           "\n" +
                           "[PostUpdateWin]\n" +
                           "ExeRelPath=" + gPostUpdateBinFile + "\n" +
                           exeArg +
                           exeAsync;
  let updaterIni = getApplyDirFile(DIR_RESOURCES + FILE_UPDATER_INI, true);
  writeFile(updaterIni, updaterIniContents);
}











function checkUpdateLogContents(aCompareLogFile, aExcludeDistributionDir) {
  if (IS_UNIX && !IS_MACOSX) {
    
    return;
  }
  let updateLog = getUpdatesPatchDir();
  updateLog.append(FILE_UPDATE_LOG);
  let updateLogContents = readFileBytes(updateLog);

  
  
  if (gTestFiles.length > 1 &&
      gTestFiles[gTestFiles.length - 1].fileName == "channel-prefs.js" &&
      !gTestFiles[gTestFiles.length - 1].originalContents) {
    updateLogContents = updateLogContents.replace(/.*defaults\/.*/g, "");
  }
  if (gTestFiles.length > 2 &&
      gTestFiles[gTestFiles.length - 2].fileName == FILE_UPDATE_SETTINGS_INI &&
      !gTestFiles[gTestFiles.length - 2].originalContents) {
    updateLogContents = updateLogContents.replace(/.*update-settings.ini.*/g, "");
  }
  if (gStageUpdate) {
    
    updateLogContents = updateLogContents.replace(/Performing a staged update/, "");
  } else if (gSwitchApp) {
    
    updateLogContents = updateLogContents.replace(/Performing a staged update/, "");
    updateLogContents = updateLogContents.replace(/Performing a replace request/, "");
  }
  
  updateLogContents = updateLogContents.replace(/PATCH DIRECTORY.*/g, "");
  updateLogContents = updateLogContents.replace(/INSTALLATION DIRECTORY.*/g, "");
  updateLogContents = updateLogContents.replace(/WORKING DIRECTORY.*/g, "");
  
  updateLogContents = updateLogContents.replace(/NS_main: callback app file .*/g, "");
  if (IS_MACOSX) {
    
    updateLogContents = updateLogContents.replace(/Moving old [^\n]*\nrename_file: .*/g, "");
    updateLogContents = updateLogContents.replace(/New distribution directory .*/g, "");
  }
  if (gSwitchApp) {
    
    updateLogContents = updateLogContents.replace(/^Begin moving.*$/mg, "");
  }
  updateLogContents = updateLogContents.replace(/\r/g, "");
  
  updateLogContents = updateLogContents.replace(/, err:.*\n/g, "\n");
  
  updateLogContents = updateLogContents.replace(/non-fatal error /g, "");
  
  
  updateLogContents = updateLogContents.replace(/.*7\/7text.*\n/g, "");
  
  updateLogContents = updateLogContents.replace(/\n+/g, "\n");
  
  updateLogContents = updateLogContents.replace(/^\n|\n$/g, "");
  
  
  updateLogContents = updateLogContents.replace(/^calling QuitProgressUI\n[^\n]*\nUPDATE TYPE/g, "UPDATE TYPE");

  let compareLogContents = "";
  if (aCompareLogFile) {
    compareLogContents = readFileBytes(getTestDirFile(aCompareLogFile));
  }
  if (gSwitchApp) {
    compareLogContents += LOG_SWITCH_SUCCESS;
  }
  
  
  if (gTestFiles.length > 1 &&
      gTestFiles[gTestFiles.length - 1].fileName == "channel-prefs.js" &&
      !gTestFiles[gTestFiles.length - 1].originalContents) {
    compareLogContents = compareLogContents.replace(/.*defaults\/.*/g, "");
  }
  if (gTestFiles.length > 2 &&
      gTestFiles[gTestFiles.length - 2].fileName == FILE_UPDATE_SETTINGS_INI &&
      !gTestFiles[gTestFiles.length - 2].originalContents) {
    compareLogContents = compareLogContents.replace(/.*update-settings.ini.*/g, "");
  }
  if (aExcludeDistributionDir) {
    compareLogContents = compareLogContents.replace(/.*distribution\/.*/g, "");
  }
  
  compareLogContents = compareLogContents.replace(/\n+/g, "\n");
  
  compareLogContents = compareLogContents.replace(/^\n|\n$/g, "");

  
  
  if (compareLogContents == updateLogContents) {
    debugDump("log contents are correct");
    do_check_true(true);
  } else {
    logTestInfo("log contents are not correct");
    let aryLog = updateLogContents.split("\n");
    let aryCompare = compareLogContents.split("\n");
    
    
    aryLog.push("");
    aryCompare.push("");
    
    
    for (let i = 0; i < aryLog.length; ++i) {
      if (aryCompare[i] != aryLog[i]) {
        logTestInfo("the first incorrect line in the log is: " + aryLog[i]);
        do_check_eq(aryCompare[i], aryLog[i]);
      }
    }
    
    do_throw("Unable to find incorrect log contents!");
  }
}







function checkUpdateLogContains(aCheckString) {
  let updateLog = getUpdatesPatchDir();
  updateLog.append(FILE_UPDATE_LOG);
  let updateLogContents = readFileBytes(updateLog);
  if (updateLogContents.indexOf(aCheckString) != -1) {
    debugDump("log file does contain: " + aCheckString);
    do_check_true(true);
  } else {
    logTestInfo("log file does not contain: " + aCheckString);
    do_check_true(false);
  }
}
















function checkFilesAfterUpdateSuccess(aGetFileFunc, aStageDirExists,
                                      aToBeDeletedDirExists) {
  debugDump("testing contents of files after a successful update");
  gTestFiles.forEach(function CFAUS_TF_FE(aTestFile) {
    let testFile = aGetFileFunc(aTestFile.relPathDir + aTestFile.fileName, true);
    debugDump("testing file: " + testFile.path);
    if (aTestFile.compareFile || aTestFile.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && aTestFile.comparePerms) {
        
        let logPerms = "testing file permissions - ";
        if (aTestFile.originalPerms) {
          logPerms += "original permissions: " +
                      aTestFile.originalPerms.toString(8) + ", ";
        }
        debugDump("compare permissions : " +
                  aTestFile.comparePerms.toString(8) + ", " +
                  "updated permissions : " + testFile.permissions.toString(8));
        do_check_eq(testFile.permissions & 0xfff, aTestFile.comparePerms & 0xfff);
      }

      let fileContents1 = readFileBytes(testFile);
      let fileContents2 = aTestFile.compareFile ?
                          readFileBytes(getTestDirFile(aTestFile.compareFile)) :
                          aTestFile.compareContents;
      
      
      if (fileContents1 == fileContents2) {
        debugDump("file contents are correct");
        do_check_true(true);
      } else {
        logTestInfo("file contents are not correct");
        do_check_eq(fileContents1, fileContents2);
      }
    } else {
      do_check_false(testFile.exists());
    }
  });

  debugDump("testing operations specified in removed-files were performed " +
            "after a successful update");
  gTestDirs.forEach(function CFAUS_TD_FE(aTestDir) {
    let testDir = aGetFileFunc(aTestDir.relPathDir, true);
    debugDump("testing directory: " + testDir.path);
    if (aTestDir.dirRemoved) {
      do_check_false(testDir.exists());
    } else {
      do_check_true(testDir.exists());

      if (aTestDir.files) {
        aTestDir.files.forEach(function CFAUS_TD_F_FE(aTestFile) {
          let testFile = aGetFileFunc(aTestDir.relPathDir + aTestFile, true);
          debugDump("testing directory file: " + testFile.path);
          if (aTestDir.filesRemoved) {
            do_check_false(testFile.exists());
          } else {
            do_check_true(testFile.exists());
          }
        });
      }

      if (aTestDir.subDirs) {
        aTestDir.subDirs.forEach(function CFAUS_TD_SD_FE(aSubDir) {
          let testSubDir = aGetFileFunc(aTestDir.relPathDir + aSubDir, true);
          debugDump("testing sub-directory: " + testSubDir.path);
          do_check_true(testSubDir.exists());
          if (aTestDir.subDirFiles) {
            aTestDir.subDirFiles.forEach(function CFAUS_TD_SDF_FE(aTestFile) {
              let testFile = aGetFileFunc(aTestDir.relPathDir +
                                          aSubDir + aTestFile, true);
              debugDump("testing sub-directory file: " + testFile.path);
              do_check_true(testFile.exists());
            });
          }
        });
      }
    }
  });

  checkFilesAfterUpdateCommon(aGetFileFunc, aStageDirExists,
                              aToBeDeletedDirExists);
}
















function checkFilesAfterUpdateFailure(aGetFileFunc, aStageDirExists,
                                      aToBeDeletedDirExists) {
  debugDump("testing contents of files after a failed update");
  gTestFiles.forEach(function CFAUF_TF_FE(aTestFile) {
    let testFile = aGetFileFunc(aTestFile.relPathDir + aTestFile.fileName, true);
    debugDump("testing file: " + testFile.path);
    if (aTestFile.compareFile || aTestFile.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && aTestFile.comparePerms) {
        
        let logPerms = "testing file permissions - ";
        if (aTestFile.originalPerms) {
          logPerms += "original permissions: " +
                      aTestFile.originalPerms.toString(8) + ", ";
        }
        debugDump("compare permissions : " +
                  aTestFile.comparePerms.toString(8) + ", " +
                  "updated permissions : " + testFile.permissions.toString(8));
        do_check_eq(testFile.permissions & 0xfff, aTestFile.comparePerms & 0xfff);
      }

      let fileContents1 = readFileBytes(testFile);
      let fileContents2 = aTestFile.compareFile ?
                          readFileBytes(getTestDirFile(aTestFile.compareFile)) :
                          aTestFile.compareContents;
      
      
      if (fileContents1 == fileContents2) {
        debugDump("file contents are correct");
        do_check_true(true);
      } else {
        logTestInfo("file contents are not correct");
        do_check_eq(fileContents1, fileContents2);
      }
    } else {
      do_check_false(testFile.exists());
    }
  });

  debugDump("testing operations specified in removed-files were not " +
            "performed after a failed update");
  gTestDirs.forEach(function CFAUF_TD_FE(aTestDir) {
    let testDir = aGetFileFunc(aTestDir.relPathDir, true);
    debugDump("testing directory: " + testDir.path);
    do_check_true(testDir.exists());

    if (aTestDir.files) {
      aTestDir.files.forEach(function CFAUS_TD_F_FE(aTestFile) {
        let testFile = aGetFileFunc(aTestDir.relPathDir + aTestFile, true);
        debugDump("testing directory file: " + testFile.path);
        do_check_true(testFile.exists());
      });
    }

    if (aTestDir.subDirs) {
      aTestDir.subDirs.forEach(function CFAUS_TD_SD_FE(aSubDir) {
        let testSubDir = aGetFileFunc(aTestDir.relPathDir + aSubDir, true);
        debugDump("testing sub-directory: " + testSubDir.path);
        do_check_true(testSubDir.exists());
        if (aTestDir.subDirFiles) {
          aTestDir.subDirFiles.forEach(function CFAUS_TD_SDF_FE(aTestFile) {
            let testFile = aGetFileFunc(aTestDir.relPathDir +
                                        aSubDir + aTestFile, true);
            debugDump("testing sub-directory file: " + testFile.path);
            do_check_true(testFile.exists());
          });
        }
      });
    }
  });

  checkFilesAfterUpdateCommon(aGetFileFunc, aStageDirExists,
                              aToBeDeletedDirExists);
}
















function checkFilesAfterUpdateCommon(aGetFileFunc, aStageDirExists,
                                     aToBeDeletedDirExists) {
  debugDump("testing extra directories");

  let stageDir = getStageDirFile(null, true);
  debugDump("testing directory should " +
            (aStageDirExists ? "" : "not ") +
            "exist: " + stageDir.path);
  do_check_eq(stageDir.exists(), aStageDirExists);

  let toBeDeletedDirExists = IS_WIN ? aToBeDeletedDirExists : false;
  let toBeDeletedDir = getApplyDirFile(DIR_TOBEDELETED, true);
  debugDump("testing directory should " +
            (toBeDeletedDirExists ? "" : "not ") +
            "exist: " + toBeDeletedDir.path);
  do_check_eq(toBeDeletedDir.exists(), toBeDeletedDirExists);

  debugDump("testing updating directory doesn't exist in the application " +
            "directory");
  let updatingDir = getApplyDirFile("updating", true);
  do_check_false(updatingDir.exists());

  if (stageDir.exists()) {
    debugDump("testing updating directory doesn't exist in the staging " +
              "directory");
    updatingDir = stageDir.clone();
    updatingDir.append("updating");
    do_check_false(updatingDir.exists());
  }

  debugDump("testing backup files should not be left behind in the " +
            "application directory");
  let applyToDir = getApplyDirFile(null, true);
  checkFilesInDirRecursive(applyToDir, checkForBackupFiles);

  if (stageDir.exists()) {
    debugDump("testing backup files should not be left behind in the " +
              "staging directory");
    let applyToDir = getApplyDirFile(null, true);
    checkFilesInDirRecursive(stageDir, checkForBackupFiles);
  }

  debugDump("testing patch files should not be left behind");
  let updatesDir = getUpdatesPatchDir();
  let entries = updatesDir.QueryInterface(Ci.nsIFile).directoryEntries;
  while (entries.hasMoreElements()) {
    let entry = entries.getNext().QueryInterface(Ci.nsIFile);
    do_check_neq(getFileExtension(entry), "patch");
  }
}






function checkCallbackAppLog() {
  let appLaunchLog = getApplyDirFile(DIR_RESOURCES + gCallbackArgs[1], true);
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
    debugDump("callback log file contents are correct");
    do_check_true(true);
  } else {
    debugDump("callback log file contents are not correct");
    do_check_eq(logContents, expectedLogContents);
  }

  waitForFilesInUse();
}









function getPostUpdateFile(aSuffix) {
  return getApplyDirFile(DIR_RESOURCES + gPostUpdateBinFile + aSuffix, true);
}





function checkPostUpdateAppLog() {
  gTimeoutRuns++;
  let postUpdateLog = getPostUpdateFile(".log");
  if (!postUpdateLog.exists()) {
    debugDump("postUpdateLog does not exist. Path: " + postUpdateLog.path);
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the post update " +
               "process to create the post update log. Path: " +
               postUpdateLog.path);
    }
    do_timeout(TEST_HELPER_TIMEOUT, checkPostUpdateAppLog);
    return;
  }

  let logContents = readFile(postUpdateLog);
  
  
  
  
  if (logContents != "post-update\n") {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the post update " +
               "process to create the expected contents in the post update log. Path: " +
               postUpdateLog.path);
    }
    do_timeout(TEST_HELPER_TIMEOUT, checkPostUpdateAppLog);
    return;
  }

  debugDump("post update app log file contents are correct");
  do_check_true(true);

  gCheckFunc();
}






function checkCallbackServiceLog() {
  do_check_neq(gServiceLaunchedCallbackLog, null);

  let expectedLogContents = gServiceLaunchedCallbackArgs.join("\n") + "\n";
  let logFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  logFile.initWithPath(gServiceLaunchedCallbackLog);
  let logContents = readFile(logFile);
  
  
  
  
  if (logContents != expectedLogContents) {
    debugDump("callback service log not expected value, waiting longer");
    do_timeout(TEST_HELPER_TIMEOUT, checkCallbackServiceLog);
    return;
  }

  debugDump("testing that the callback application successfully launched " +
            "and the expected command line arguments were passed to it");
  do_check_eq(logContents, expectedLogContents);

  waitForFilesInUse();
}



function waitForFilesInUse() {
  if (IS_WIN) {
    let appBin = getApplyDirFile(FILE_APP_BIN, true);
    let maintSvcInstaller = getApplyDirFile(FILE_MAINTENANCE_SERVICE_INSTALLER_BIN, true);
    let helper = getApplyDirFile("uninstall/helper.exe", true);
    let updater = getUpdatesPatchDir();
    updater.append(FILE_UPDATER_BIN);

    let files = [appBin, updater, maintSvcInstaller, helper];

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
          debugDump("file is not in use. Path: " + file.path);
        } catch (e) {
          debugDump("file in use, will try again after " + TEST_CHECK_TIMEOUT +
                    " ms, Path: " + file.path + ", Exception: " + e);
          try {
            if (fileBak.exists()) {
              fileBak.remove(false);
            }
          } catch (e) {
            logTestInfo("unable to remove file, this should never happen! " +
                        "Path: " + fileBak.path + ", Exception: " + e);
          }
          do_timeout(TEST_CHECK_TIMEOUT, waitForFilesInUse);
          return;
        }
      }
    }
  }

  debugDump("calling doTestFinish");
  doTestFinish();
}








function checkForBackupFiles(aFile) {
  do_check_neq(getFileExtension(aFile), "moz-backup");
}












function checkFilesInDirRecursive(aDir, aCallback) {
  if (!aDir.exists()) {
    do_throw("Directory must exist!");
  }

  let dirEntries = aDir.directoryEntries;
  while (dirEntries.hasMoreElements()) {
    let entry = dirEntries.getNext().QueryInterface(Ci.nsIFile);

    if (entry.isDirectory()) {
      checkFilesInDirRecursive(entry, aCallback);
    } else {
      aCallback(entry);
    }
  }
}
















function overrideXHR(aCallback) {
  Cu.import("resource://testing-common/MockRegistrar.jsm");
  MockRegistrar.register("@mozilla.org/xmlextras/xmlhttprequest;1", xhr, [aCallback]);
}






function makeHandler(aVal) {
  if (typeof aVal == "function") {
    return { handleEvent: aVal };
  }
  return aVal;
}
function xhr(aCallback) {
  this._callback = aCallback;
}
xhr.prototype = {
  overrideMimeType: function(aMimetype) { },
  setRequestHeader: function(aHeader, aValue) { },
  status: null,
  channel: {
    set notificationCallbacks(aVal) { },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIChannel])
  },
  _url: null,
  _method: null,
  open: function(aMethod, aUrl) {
    this.channel.originalURI = Services.io.newURI(aUrl, null, null);
    this._method = aMethod; this._url = aUrl;
  },
  responseXML: null,
  responseText: null,
  send: function(aBody) {
    do_execute_soon(function() {
      this._callback(this);
    }.bind(this)); 
  },
  _onprogress: null,
  set onprogress(aValue) { this._onprogress = makeHandler(aValue); },
  get onprogress() { return this._onprogress; },
  _onerror: null,
  set onerror(aValue) { this._onerror = makeHandler(aValue); },
  get onerror() { return this._onerror; },
  _onload: null,
  set onload(aValue) { this._onload = makeHandler(aValue); },
  get onload() { return this._onload; },
  addEventListener: function(aEvent, aValue, aCapturing) {
    eval("this._on" + aEvent + " = aValue");
  },
  flags: Ci.nsIClassInfo.SINGLETON,
  getScriptableHelper: function() null,
  getInterfaces: function(aCount) {
    let interfaces = [Ci.nsISupports];
    aCount.value = interfaces.length;
    return interfaces;
  },
  get wrappedJSObject() { return this; },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIClassInfo])
};








function overrideUpdatePrompt(aCallback) {
  Cu.import("resource://testing-common/MockRegistrar.jsm");
  MockRegistrar.register("@mozilla.org/updates/update-prompt;1", UpdatePrompt, [aCallback]);
}

function UpdatePrompt(aCallback) {
  this._callback = aCallback;

  let fns = ["checkForUpdates", "showUpdateAvailable", "showUpdateDownloaded",
             "showUpdateError", "showUpdateHistory", "showUpdateInstalled"];

  fns.forEach(function(aPromptFn) {
    UpdatePrompt.prototype[aPromptFn] = function() {
      if (!this._callback) {
        return;
      }

      let callback = this._callback[aPromptFn];
      if (!callback) {
        return;
      }

      callback.apply(this._callback,
                     Array.prototype.slice.call(arguments));
    }
  });
}

UpdatePrompt.prototype = {
  flags: Ci.nsIClassInfo.SINGLETON,
  getScriptableHelper: function() null,
  getInterfaces: function(aCount) {
    let interfaces = [Ci.nsISupports, Ci.nsIUpdatePrompt];
    aCount.value = interfaces.length;
    return interfaces;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIClassInfo, Ci.nsIUpdatePrompt])
};


const updateCheckListener = {
  onProgress: function UCL_onProgress(aRequest, aPosition, aTotalSize) {
  },

  onCheckComplete: function UCL_onCheckComplete(aRequest, aUpdates, aUpdateCount) {
    gRequestURL = aRequest.channel.originalURI.spec;
    gUpdateCount = aUpdateCount;
    gUpdates = aUpdates;
    debugDump("url = " + gRequestURL + ", " +
              "request.status = " + aRequest.status + ", " +
              "updateCount = " + aUpdateCount);
    
    do_execute_soon(gCheckFunc);
  },

  onError: function UCL_onError(aRequest, aUpdate) {
    gRequestURL = aRequest.channel.originalURI.spec;
    gStatusCode = aRequest.status;
    gStatusText = aUpdate.statusText ? aUpdate.statusText : null;
    debugDump("url = " + gRequestURL + ", " +
              "request.status = " + gStatusCode + ", " +
              "update.statusText = " + gStatusText);
    
    do_execute_soon(gCheckFunc.bind(null, aRequest, aUpdate));
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdateCheckListener])
};


const downloadListener = {
  onStartRequest: function DL_onStartRequest(aRequest, aContext) {
  },

  onProgress: function DL_onProgress(aRequest, aContext, aProgress, aMaxProgress) {
  },

  onStatus: function DL_onStatus(aRequest, aContext, aStatus, aStatusText) {
  },

  onStopRequest: function DL_onStopRequest(aRequest, aContext, aStatus) {
    gStatusResult = aStatus;
    
    do_execute_soon(gCheckFunc);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRequestObserver,
                                         Ci.nsIProgressEventSink])
};




function start_httpserver() {
  let dir = getTestDirFile();
  debugDump("http server directory path: " + dir.path);

  if (!dir.isDirectory()) {
    do_throw("A file instead of a directory was specified for HttpServer " +
             "registerDirectory! Path: " + dir.path);
  }

  let { HttpServer } = Cu.import("resource://testing-common/httpd.js", {});
  gTestserver = new HttpServer();
  gTestserver.registerDirectory("/", dir);
  gTestserver.start(-1);
  let testserverPort = gTestserver.identity.primaryPort;
  gURLData = URL_HOST + ":" + testserverPort + "/";
  debugDump("http server port = " + testserverPort);
}







function stop_httpserver(aCallback) {
  do_check_true(!!aCallback);
  gTestserver.stop(aCallback);
}













function createAppInfo(aID, aName, aVersion, aPlatformVersion) {
  const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
  const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");
  let ifaces = [Ci.nsIXULAppInfo, Ci.nsIXULRuntime];
  if (IS_WIN) {
    ifaces.push(Ci.nsIWinAppHelper);
  }
  const XULAppInfo = {
    vendor: APP_INFO_VENDOR,
    name: aName,
    ID: aID,
    version: aVersion,
    appBuildID: "2007010101",
    platformVersion: aPlatformVersion,
    platformBuildID: "2007010101",
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",

    QueryInterface: XPCOMUtils.generateQI(ifaces)
  };

  const XULAppInfoFactory = {
    createInstance: function(aOuter, aIID) {
      if (aOuter == null) {
        return XULAppInfo.QueryInterface(aIID);
      }
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
  };

  let registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}





















function getProcessArgs(aExtraArgs) {
  if (!aExtraArgs) {
    aExtraArgs = [];
  }

  let appBinPath = getApplyDirFile(DIR_MACOS + FILE_APP_BIN, false).path;
  if (/ /.test(appBinPath)) {
    appBinPath = '"' + appBinPath + '"';
  }

  let args;
  if (IS_UNIX) {
    let launchScript = getLaunchScript();
    
    launchScript.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_DIRECTORY);

    let scriptContents = "#! /bin/sh\n";
    scriptContents += appBinPath + " -no-remote -process-updates " +
                      aExtraArgs.join(" ") + " " + PIPE_TO_NULL;
    writeFile(launchScript, scriptContents);
    debugDump("created " + launchScript.path + " containing:\n" +
              scriptContents);
    args = [launchScript.path];
  } else {
    args = ["/D", "/Q", "/C", appBinPath, "-no-remote", "-process-updates"].
           concat(aExtraArgs).concat([PIPE_TO_NULL]);
  }
  return args;
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





function adjustGeneralPaths() {
  let dirProvider = {
    getFile: function AGP_DP_getFile(aProp, aPersistent) {
      aPersistent.value = true;
      switch (aProp) {
        case NS_GRE_DIR:
          if (gUseTestAppDir) {
            return getApplyDirFile(DIR_RESOURCES, true);
          }
          break;
        case NS_GRE_BIN_DIR:
          if (gUseTestAppDir) {
            return getApplyDirFile(DIR_MACOS, true);
          }
          break;
        case XRE_EXECUTABLE_FILE:
          if (gUseTestAppDir) {
            return getApplyDirFile(DIR_MACOS + FILE_APP_BIN, true);
          }
          break;
        case XRE_UPDATE_ROOT_DIR:
          return getMockUpdRootD();
      }
      return null;
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider])
  };
  let ds = Services.dirsvc.QueryInterface(Ci.nsIDirectoryService);
  ds.QueryInterface(Ci.nsIProperties).undefine(NS_GRE_DIR);
  ds.QueryInterface(Ci.nsIProperties).undefine(NS_GRE_BIN_DIR);
  ds.QueryInterface(Ci.nsIProperties).undefine(XRE_EXECUTABLE_FILE);
  ds.registerProvider(dirProvider);
  do_register_cleanup(function AGP_cleanup() {
    debugDump("start - unregistering directory provider");

    if (gAppTimer) {
      debugDump("start - cancel app timer");
      gAppTimer.cancel();
      gAppTimer = null;
      debugDump("finish - cancel app timer");
    }

    if (gProcess && gProcess.isRunning) {
      debugDump("start - kill process");
      try {
        gProcess.kill();
      } catch (e) {
        debugDump("kill process failed. Exception: " + e);
      }
      gProcess = null;
      debugDump("finish - kill process");
    }

    if (gHandle) {
      try {
        debugDump("start - closing handle");
        let kernel32 = ctypes.open("kernel32");
        let CloseHandle = kernel32.declare("CloseHandle", ctypes.default_abi,
                                           ctypes.bool, 
                                           ctypes.voidptr_t );
        if (!CloseHandle(gHandle)) {
          debugDump("call to CloseHandle failed");
        }
        kernel32.close();
        gHandle = null;
        debugDump("finish - closing handle");
      } catch (e) {
        debugDump("call to CloseHandle failed. Exception: " + e);
      }
    }

    
    if (typeof(end_test) == typeof(Function)) {
      debugDump("calling end_test");
      end_test();
    }

    ds.unregisterProvider(dirProvider);
    cleanupTestCommon();

    debugDump("finish - unregistering directory provider");
  });
}





function launchAppToApplyUpdate() {
  debugDump("start - launching application to apply update");

  let appBin = getApplyDirFile(DIR_MACOS + FILE_APP_BIN, false);

  if (typeof(customLaunchAppToApplyUpdate) == typeof(Function)) {
    customLaunchAppToApplyUpdate();
  }

  let launchBin = getLaunchBin();
  let args = getProcessArgs();
  debugDump("launching " + launchBin.path + " " + args.join(" "));

  gProcess = Cc["@mozilla.org/process/util;1"].
             createInstance(Ci.nsIProcess);
  gProcess.init(launchBin);

  gAppTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  gAppTimer.initWithCallback(gTimerCallback, APP_TIMER_TIMEOUT,
                             Ci.nsITimer.TYPE_ONE_SHOT);

  setEnvironment();
  debugDump("launching application");
  gProcess.runAsync(args, args.length, gProcessObserver);
  resetEnvironment();

  debugDump("finish - launching application to apply update");
}




const gProcessObserver = {
  observe: function PO_observe(aSubject, aTopic, aData) {
    debugDump("topic: " + aTopic + ", process exitValue: " +
              gProcess.exitValue);
    if (gAppTimer) {
      gAppTimer.cancel();
      gAppTimer = null;
    }
    if (aTopic != "process-finished" || gProcess.exitValue != 0) {
      do_throw("Failed to launch application");
    }
    do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
};




const gTimerCallback = {
  notify: function TC_notify(aTimer) {
    gAppTimer = null;
    if (gProcess.isRunning) {
      logTestInfo("attempt to kill process");
      gProcess.kill();
    }
    do_throw("launch application timer expired");
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback])
};




const gUpdateStagedObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "update-staged") {
      Services.obs.removeObserver(gUpdateStagedObserver, "update-staged");
      checkUpdateApplied();
    }
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
};





function setEnvironment() {
  
  if (gShouldResetEnv !== undefined) {
    return;
  }

  gShouldResetEnv = true;

  let env = Cc["@mozilla.org/process/environment;1"].
            getService(Ci.nsIEnvironment);
  if (IS_WIN && !env.exists("XRE_NO_WINDOWS_CRASH_DIALOG")) {
    gAddedEnvXRENoWindowsCrashDialog = true;
    debugDump("setting the XRE_NO_WINDOWS_CRASH_DIALOG environment " +
              "variable to 1... previously it didn't exist");
    env.set("XRE_NO_WINDOWS_CRASH_DIALOG", "1");
  }

  if (IS_UNIX) {
    let appGreBinDir = gGREBinDirOrig.clone();
    let envGreBinDir = Cc["@mozilla.org/file/local;1"].
                       createInstance(Ci.nsILocalFile);
    let shouldSetEnv = true;
    if (IS_MACOSX) {
      if (env.exists("DYLD_LIBRARY_PATH")) {
        gEnvDyldLibraryPath = env.get("DYLD_LIBRARY_PATH");
        envGreBinDir.initWithPath(gEnvDyldLibraryPath);
        if (envGreBinDir.path == appGreBinDir.path) {
          gEnvDyldLibraryPath = null;
          shouldSetEnv = false;
        }
      }

      if (shouldSetEnv) {
        debugDump("setting DYLD_LIBRARY_PATH environment variable value to " +
                  appGreBinDir.path);
        env.set("DYLD_LIBRARY_PATH", appGreBinDir.path);
      }
    } else {
      if (env.exists("LD_LIBRARY_PATH")) {
        gEnvLdLibraryPath = env.get("LD_LIBRARY_PATH");
        envGreBinDir.initWithPath(gEnvLdLibraryPath);
        if (envGreBinDir.path == appGreBinDir.path) {
          gEnvLdLibraryPath = null;
          shouldSetEnv = false;
        }
      }

      if (shouldSetEnv) {
        debugDump("setting LD_LIBRARY_PATH environment variable value to " +
                  appGreBinDir.path);
        env.set("LD_LIBRARY_PATH", appGreBinDir.path);
      }
    }
  }

  if (env.exists("XPCOM_MEM_LEAK_LOG")) {
    gEnvXPCOMMemLeakLog = env.get("XPCOM_MEM_LEAK_LOG");
    debugDump("removing the XPCOM_MEM_LEAK_LOG environment variable... " +
              "previous value " + gEnvXPCOMMemLeakLog);
    env.set("XPCOM_MEM_LEAK_LOG", "");
  }

  if (env.exists("XPCOM_DEBUG_BREAK")) {
    gEnvXPCOMDebugBreak = env.get("XPCOM_DEBUG_BREAK");
    debugDump("setting the XPCOM_DEBUG_BREAK environment variable to " +
              "warn... previous value " + gEnvXPCOMDebugBreak);
  } else {
    debugDump("setting the XPCOM_DEBUG_BREAK environment variable to " +
              "warn... previously it didn't exist");
  }

  env.set("XPCOM_DEBUG_BREAK", "warn");

  if (gStageUpdate) {
    debugDump("setting the MOZ_UPDATE_STAGING environment variable to 1");
    env.set("MOZ_UPDATE_STAGING", "1");
  }

  debugDump("setting MOZ_NO_SERVICE_FALLBACK environment variable to 1");
  env.set("MOZ_NO_SERVICE_FALLBACK", "1");
}





function resetEnvironment() {
  
  if (gShouldResetEnv !== true) {
    return;
  }

  gShouldResetEnv = false;

  let env = Cc["@mozilla.org/process/environment;1"].
            getService(Ci.nsIEnvironment);

  if (gEnvXPCOMMemLeakLog) {
    debugDump("setting the XPCOM_MEM_LEAK_LOG environment variable back to " +
              gEnvXPCOMMemLeakLog);
    env.set("XPCOM_MEM_LEAK_LOG", gEnvXPCOMMemLeakLog);
  }

  if (gEnvXPCOMDebugBreak) {
    debugDump("setting the XPCOM_DEBUG_BREAK environment variable back to " +
              gEnvXPCOMDebugBreak);
    env.set("XPCOM_DEBUG_BREAK", gEnvXPCOMDebugBreak);
  } else {
    debugDump("clearing the XPCOM_DEBUG_BREAK environment variable");
    env.set("XPCOM_DEBUG_BREAK", "");
  }

  if (IS_UNIX) {
    if (IS_MACOSX) {
      if (gEnvDyldLibraryPath) {
        debugDump("setting DYLD_LIBRARY_PATH environment variable value " +
                  "back to " + gEnvDyldLibraryPath);
        env.set("DYLD_LIBRARY_PATH", gEnvDyldLibraryPath);
      } else {
        debugDump("removing DYLD_LIBRARY_PATH environment variable");
        env.set("DYLD_LIBRARY_PATH", "");
      }
    } else {
      if (gEnvLdLibraryPath) {
        debugDump("setting LD_LIBRARY_PATH environment variable value back " +
                  "to " + gEnvLdLibraryPath);
        env.set("LD_LIBRARY_PATH", gEnvLdLibraryPath);
      } else {
        debugDump("removing LD_LIBRARY_PATH environment variable");
        env.set("LD_LIBRARY_PATH", "");
      }
    }
  }

  if (IS_WIN && gAddedEnvXRENoWindowsCrashDialog) {
    debugDump("removing the XRE_NO_WINDOWS_CRASH_DIALOG environment " +
              "variable");
    env.set("XRE_NO_WINDOWS_CRASH_DIALOG", "");
  }

  if (gStageUpdate) {
    debugDump("removing the MOZ_UPDATE_STAGING environment variable");
    env.set("MOZ_UPDATE_STAGING", "");
  }

  debugDump("removing MOZ_NO_SERVICE_FALLBACK environment variable");
  env.set("MOZ_NO_SERVICE_FALLBACK", "");
}
