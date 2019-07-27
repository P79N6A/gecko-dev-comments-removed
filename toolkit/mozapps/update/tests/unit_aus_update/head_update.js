



const INSTALL_LOCALE = "@AB_CD@";
const MOZ_APP_NAME = "@MOZ_APP_NAME@";
const BIN_SUFFIX = "@BIN_SUFFIX@";

#ifdef XP_WIN



#ifdef MOZ_APP_VENDOR
const MOZ_APP_VENDOR = "@MOZ_APP_VENDOR@";
#else
const MOZ_APP_VENDOR = "";
#endif


const MOZ_APP_BASENAME = "@MOZ_APP_BASENAME@";
#endif 

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

const FILE_APP_BIN = MOZ_APP_NAME + APP_BIN_SUFFIX;
const FILE_COMPLETE_EXE = "complete.exe";
const FILE_COMPLETE_MAR = "complete.mar";
const FILE_HELPER_BIN = "TestAUSHelper" + BIN_SUFFIX;
const FILE_MAINTENANCE_SERVICE_BIN = "maintenanceservice.exe";
const FILE_MAINTENANCE_SERVICE_INSTALLER_BIN = "maintenanceservice_installer.exe";
const FILE_OLD_VERSION_MAR = "old_version.mar";
const FILE_PARTIAL_EXE = "partial.exe";
const FILE_PARTIAL_MAR = "partial.mar";
const FILE_UPDATER_BIN = "updater" + BIN_SUFFIX;
const FILE_WRONG_CHANNEL_MAR = "wrong_product_channel.mar";

const LOG_COMPLETE_SUCCESS = "complete_log_success";
const LOG_PARTIAL_SUCCESS  = "partial_log_success";
const LOG_PARTIAL_FAILURE  = "partial_log_failure";

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

#ifdef XP_WIN
const PIPE_TO_NULL = ">nul";
#else
const PIPE_TO_NULL = "> /dev/null 2>&1";
#endif


var gURLData = URL_HOST + "/";

var gTestID;

var gTestserver;

var gRegisteredServiceCleanup;

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

var gProcess;
var gAppTimer;
var gHandle;

var gGREDirOrig;
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




var DEBUG_AUS_TEST = true;




var DEBUG_TEST_LOG = false;

var gDeleteLogFile = true;
var gRealDump;
var gTestLogText = "";
var gPassed;

#include ../shared.js

var gTestFiles = [];
var gTestDirs = [];


var gTestFilesCommon = [
{
  description      : "Should never change",
  fileName         : FILE_UPDATE_SETTINGS_INI,
  relPathDir       : "a/b/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o767,
  comparePerms     : 0o767
}, {
  description      : "Should never change",
  fileName         : "channel-prefs.js",
  relPathDir       : "a/b/defaults/pref/",
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
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "partial_precomplete",
  compareFile      : "complete_precomplete",
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginstext0",
  relPathDir       : "a/b/searchplugins/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o775,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginspng1.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginspng0.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "partial.png",
  compareFile      : "complete.png",
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "removed-files",
  relPathDir       : "a/b/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "partial_removed-files",
  compareFile      : "complete_removed-files",
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions1text0",
  relPathDir       : "a/b/distribution/extensions/extensions1/",
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
  relPathDir       : "a/b/distribution/extensions/extensions1/",
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
  relPathDir       : "a/b/distribution/extensions/extensions1/",
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
  relPathDir       : "a/b/distribution/extensions/extensions0/",
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
  relPathDir       : "a/b/distribution/extensions/extensions0/",
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
  relPathDir       : "a/b/distribution/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "exe0.exe",
  relPathDir       : "a/b/",
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_HELPER_BIN,
  compareFile      : FILE_COMPLETE_EXE,
  originalPerms    : 0o777,
  comparePerms     : 0o755
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "10text0",
  relPathDir       : "a/b/1/10/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o767,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "0exe0.exe",
  relPathDir       : "a/b/0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_HELPER_BIN,
  compareFile      : FILE_COMPLETE_EXE,
  originalPerms    : 0o777,
  comparePerms     : 0o755
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text1",
  relPathDir       : "a/b/0/00/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o677,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text0",
  relPathDir       : "a/b/0/00/",
  originalContents : "ToBeReplacedWithFromComplete\n",
  compareContents  : "FromComplete\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o775,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00png0.png",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "complete.png",
  originalPerms    : 0o776,
  comparePerms     : 0o644
}, {
  description      : "Removed by precomplete (remove)",
  fileName         : "20text0",
  relPathDir       : "a/b/2/20/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Removed by precomplete (remove)",
  fileName         : "20png0.png",
  relPathDir       : "a/b/2/20/",
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
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete_precomplete",
  compareFile      : "partial_precomplete",
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginstext0",
  relPathDir       : "a/b/searchplugins/",
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
  relPathDir       : "a/b/searchplugins/",
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
  relPathDir       : "a/b/searchplugins/",
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
  relPathDir       : "a/b/distribution/extensions/extensions1/",
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
  relPathDir       : "a/b/distribution/extensions/extensions1/",
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
  relPathDir       : "a/b/distribution/extensions/extensions1/",
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
  relPathDir       : "a/b/distribution/extensions/extensions0/",
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
  relPathDir       : "a/b/distribution/extensions/extensions0/",
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
  relPathDir       : "a/b/distribution/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "exe0.exe",
  relPathDir       : "a/b/",
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_COMPLETE_EXE,
  compareFile      : FILE_PARTIAL_EXE,
  originalPerms    : 0o755,
  comparePerms     : 0o755
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "0exe0.exe",
  relPathDir       : "a/b/0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : FILE_COMPLETE_EXE,
  compareFile      : FILE_PARTIAL_EXE,
  originalPerms    : 0o755,
  comparePerms     : 0o755
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text0",
  relPathDir       : "a/b/0/00/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "00png0.png",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20text0",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20png0.png",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "partial.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text2",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "10text0",
  relPathDir       : "a/b/1/10/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "00text1",
  relPathDir       : "a/b/0/00/",
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



var gTestDirsCompleteSuccess = [
{
  description  : "Removed by precomplete (rmdir)",
  relPathDir   : "a/b/2/20/",
  dirRemoved   : true
}, {
  description  : "Removed by precomplete (rmdir)",
  relPathDir   : "a/b/2/",
  dirRemoved   : true
}];


gTestDirsCompleteSuccess = gTestDirsCommon.concat(gTestDirsCompleteSuccess);



var gTestDirsPartialSuccess = [
{
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : "a/b/1/10/",
  dirRemoved   : true
}, {
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : "a/b/1/",
  dirRemoved   : true
}];


gTestDirsPartialSuccess = gTestDirsCommon.concat(gTestDirsPartialSuccess);



var gTestExtraDirs = [
{
  relPathDir   : DIR_UPDATED,
  dirExists    : false
}, {
  relPathDir   : DIR_TOBEDELETED,
  dirExists    : false
}];



if (MOZ_APP_NAME == "xulrunner") {
  try {
    gDefaultPrefBranch.getCharPref(PREF_APP_UPDATE_CHANNEL);
  } catch (e) {
    setUpdateChannel("test_channel");
  }
}




function setupTestCommon() {
  logTestInfo("start - general test setup");

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
  gAppDirOrig = getAppBaseDir();

  let applyDir = getApplyDirFile(null, true).parent;

  
  
  
  if (applyDir.exists()) {
    logTestInfo("attempting to remove directory. Path: " + applyDir.path);
    try {
      removeDirRecursive(applyDir);
    } catch (e) {
      logTestInfo("non-fatal error removing directory. Path: " +
                  applyDir.path + ", Exception: " + e);
    }
  }

  
  
  adjustGeneralPaths();

  
  
  
  
  if (IS_WIN) {
    let updatesDir = getMockUpdRootD();
    if (updatesDir.exists())  {
      logTestInfo("attempting to remove directory. Path: " + updatesDir.path);
      try {
        removeDirRecursive(updatesDir);
      } catch (e) {
        logTestInfo("non-fatal error removing directory. Path: " +
                    updatesDir.path + ", Exception: " + e);
      }
    }
  }

  logTestInfo("finish - general test setup");
}





function cleanupTestCommon() {
  logTestInfo("start - general test cleanup");

  
  
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

  if (IS_UNIX) {
    
    getLaunchScript();
  }

  if (IS_WIN && MOZ_APP_BASENAME) {
    let appDir = getApplyDirFile(null, true);
    let vendor = MOZ_APP_VENDOR ? MOZ_APP_VENDOR : "Mozilla";
    const REG_PATH = "SOFTWARE\\" + vendor + "\\" + MOZ_APP_BASENAME +
                     "\\TaskBarIDs";
    let key = AUS_Cc["@mozilla.org/windows-registry-key;1"].
              createInstance(AUS_Ci.nsIWindowsRegKey);
    try {
      key.open(AUS_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE, REG_PATH,
               AUS_Ci.nsIWindowsRegKey.ACCESS_ALL);
      if (key.hasValue(appDir.path)) {
        key.removeValue(appDir.path);
      }
    } catch (e) {
    }
    try {
      key.open(AUS_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER, REG_PATH,
               AUS_Ci.nsIWindowsRegKey.ACCESS_ALL);
      if (key.hasValue(appDir.path)) {
        key.removeValue(appDir.path);
      }
    } catch (e) {
    }
  }

  
  
  if (IS_WIN) {
    let updatesDir = getMockUpdRootD();
    
    
    if (updatesDir.exists()) {
      logTestInfo("attempting to remove directory. Path: " + updatesDir.path);
      try {
        removeDirRecursive(updatesDir);
      } catch (e) {
        logTestInfo("non-fatal error removing directory. Path: " +
                    updatesDir.path + ", Exception: " + e);
      }
    }
  }

  let applyDir = getApplyDirFile(null, true).parent;

  
  
  if (applyDir.exists()) {
    logTestInfo("attempting to remove directory. Path: " + applyDir.path);
    try {
      removeDirRecursive(applyDir);
    } catch (e) {
      logTestInfo("non-fatal error removing directory. Path: " +
                  applyDir.path + ", Exception: " + e);
    }
  }

  resetEnvironment();

  logTestInfo("finish - general test cleanup");

  if (gRealDump) {
    dump = gRealDump;
    gRealDump = null;
  }

  if (DEBUG_TEST_LOG && !gPassed) {
    let fos = AUS_Cc["@mozilla.org/network/file-output-stream;1"].
              createInstance(AUS_Ci.nsIFileOutputStream);
    let logFile = do_get_file(gTestID + ".log", true);
    if (!logFile.exists()) {
      logFile.create(AUS_Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
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
  
  Services.prefs.setBoolPref(PREF_APP_UPDATE_LOG, true);
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
  iniFile.append("application.ini");
  if (!iniFile.exists()) {
    iniFile = gAppDirOrig.clone();
    iniFile.append("application.ini");
  }
  if (!iniFile.exists()) {
    do_throw("Unable to find application.ini!");
  }
  let iniParser = AUS_Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
                  getService(AUS_Ci.nsIINIParserFactory).
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








function getTestDirPath() {
  return "../data/";
}












function getTestDirFile(aRelPath) {
  let relpath = getTestDirPath() + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, false);
}






function getUpdatedDirPath() {
  return getApplyDirPath() + (gStageUpdate ? DIR_UPDATED +  "/" : "");
}

#ifdef XP_WIN
XPCOMUtils.defineLazyGetter(this, "gInstallDirPathHash",
                            function test_gInstallDirPathHash() {
  
  if (!MOZ_APP_BASENAME)
    return null;

  let vendor = MOZ_APP_VENDOR ? MOZ_APP_VENDOR : "Mozilla";
  let appDir = getApplyDirFile(null, true);

  const REG_PATH = "SOFTWARE\\" + vendor + "\\" + MOZ_APP_BASENAME +
                   "\\TaskBarIDs";
  let regKey = AUS_Cc["@mozilla.org/windows-registry-key;1"].
               createInstance(AUS_Ci.nsIWindowsRegKey);
  try {
    regKey.open(AUS_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE, REG_PATH,
                AUS_Ci.nsIWindowsRegKey.ACCESS_ALL);
    regKey.writeStringValue(appDir.path, gTestID);
    return gTestID;
  } catch (e) {
  }

  try {
    regKey.create(AUS_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER, REG_PATH,
                  AUS_Ci.nsIWindowsRegKey.ACCESS_ALL);
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
  const CSIDL_LOCAL_APPDATA = 0x1c;

  AUS_Cu.import("resource://gre/modules/ctypes.jsm");
  let lib = ctypes.open("shell32");
  let SHGetSpecialFolderPath = lib.declare("SHGetSpecialFolderPathW",
                                           ctypes.winapi_abi,
                                           ctypes.bool, 
                                           ctypes.int32_t, 
                                           ctypes.jschar.ptr, 
                                           ctypes.int32_t, 
                                           ctypes.bool );

  let aryPathLocalAppData = ctypes.jschar.array()(260);
  let rv = SHGetSpecialFolderPath(0, aryPathLocalAppData, CSIDL_LOCAL_APPDATA, false);
  lib.close();

  let pathLocalAppData = aryPathLocalAppData.readString(); 
  let updatesDir = AUS_Cc["@mozilla.org/file/local;1"].
                   createInstance(AUS_Ci.nsILocalFile);
  updatesDir.initWithPath(pathLocalAppData);
  return updatesDir;
});

XPCOMUtils.defineLazyGetter(this, "gProgFilesDir",
                            function test_gProgFilesDir() {
  const CSIDL_PROGRAM_FILES = 0x26;

  AUS_Cu.import("resource://gre/modules/ctypes.jsm");
  let lib = ctypes.open("shell32");
  let SHGetSpecialFolderPath = lib.declare("SHGetSpecialFolderPathW",
                                           ctypes.winapi_abi,
                                           ctypes.bool, 
                                           ctypes.int32_t, 
                                           ctypes.jschar.ptr, 
                                           ctypes.int32_t, 
                                           ctypes.bool );

  let aryPathProgFiles = ctypes.jschar.array()(260);
  let rv = SHGetSpecialFolderPath(0, aryPathProgFiles, CSIDL_PROGRAM_FILES, false);
  lib.close();

  let pathProgFiles = aryPathProgFiles.readString(); 
  let progFilesDir = AUS_Cc["@mozilla.org/file/local;1"].
                     createInstance(AUS_Ci.nsILocalFile);
  progFilesDir.initWithPath(pathProgFiles);
  return progFilesDir;
});







function getMockUpdRootD() {
  let localAppDataDir = gLocalAppDataDir.clone();
  let progFilesDir = gProgFilesDir.clone();
  let appDir = Services.dirsvc.get(XRE_EXECUTABLE_FILE, AUS_Ci.nsIFile).parent;

  let appDirPath = appDir.path;
  var relPathUpdates = "";
  if (gInstallDirPathHash && (MOZ_APP_VENDOR || MOZ_APP_BASENAME)) {
    relPathUpdates += (MOZ_APP_VENDOR ? MOZ_APP_VENDOR : MOZ_APP_BASENAME) +
                      "\\" + DIR_UPDATES + "\\" + gInstallDirPathHash;
  }

  if (!relPathUpdates) {
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

  var updatesDir = AUS_Cc["@mozilla.org/file/local;1"].
                   createInstance(AUS_Ci.nsILocalFile);
  updatesDir.initWithPath(localAppDataDir.path + "\\" + relPathUpdates);
  logTestInfo("returning UpdRootD Path: " + updatesDir.path);
  return updatesDir;
}
#else






function getMockUpdRootD() {
  return getApplyDirFile(DIR_BIN_REL_PATH, true);
}
#endif




















function getTargetDirFile(aRelPath, aAllowNonexistent) {
  let relpath = getUpdatedDirPath() + (aRelPath ? aRelPath : "");
  return do_get_file(relpath, aAllowNonexistent);
}

if (IS_WIN) {
  const kLockFileName = "updated.update_in_progress.lock";
  





  function lockDirectory(aDir) {
    var file = aDir.clone();
    file.append(kLockFileName);
    file.create(file.NORMAL_FILE_TYPE, 0o444);
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















function runUpdate(aExpectedExitValue, aExpectedStatus, aCallback) {
  
  let binDir = gGREDirOrig.clone();
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
  if (gStageUpdate || gSwitchApp) {
    applyToDirPath += "/" + DIR_UPDATED + "/";
  }

  if (IS_WIN) {
    
    applyToDirPath = applyToDirPath.replace(/\//g, "\\");
  }

  let callbackApp = getApplyDirFile("a/b/" + gCallbackBinFile);
  callbackApp.permissions = PERMS_DIRECTORY;

  let args = [updatesDir.path, applyToDirPath, 0];
  if (gStageUpdate) {
    args[2] = -1;
  } else {
    if (gSwitchApp) {
      args[2] = "0/replace";
    }
    args = args.concat([callbackApp.parent.path, callbackApp.path]);
    args = args.concat(gCallbackArgs);
  }
  logTestInfo("running the updater: " + updateBin.path + " " + args.join(" "));

  let env = AUS_Cc["@mozilla.org/process/environment;1"].
            getService(AUS_Ci.nsIEnvironment);
  if (gDisableReplaceFallback) {
    env.set("MOZ_NO_REPLACE_FALLBACK", "1");
  }
  env.set("MOZ_EMULATE_ELEVATION_PATH", "1");

  let process = AUS_Cc["@mozilla.org/process/util;1"].
                createInstance(AUS_Ci.nsIProcess);
  process.init(updateBin);

  process.run(true, args, args.length);

  if (gDisableReplaceFallback) {
    env.set("MOZ_NO_REPLACE_FALLBACK", "");
  }
  env.set("MOZ_EMULATE_ELEVATION_PATH", "");

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
    logTestInfo("contents of " + updateLog.path + ":\n" +
                readFileBytes(updateLog).replace(/\r\n/g, "\n"));
  }
  logTestInfo("testing updater binary process exitValue against expected " +
              "exit value");
  do_check_eq(process.exitValue, aExpectedExitValue);
  logTestInfo("testing update status against expected status");
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
  logTestInfo("start - staging update");
  Services.obs.addObserver(gUpdateStagedObserver, "update-staged", false);

  setEnvironment();
  
  AUS_Cc["@mozilla.org/updates/update-processor;1"].
    createInstance(AUS_Ci.nsIUpdateProcessor).
    processUpdate(gUpdateManager.activeUpdate);
  resetEnvironment();

  logTestInfo("finish - staging update");
}









function shouldRunServiceTest(aFirstTest) {
  
  
  
  attemptServiceInstall();

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

  const REG_PATH = "SOFTWARE\\Mozilla\\MaintenanceService\\" +
                   "3932ecacee736d366d6436db0f55bce4";

  let key = AUS_Cc["@mozilla.org/windows-registry-key;1"].
            createInstance(AUS_Ci.nsIWindowsRegKey);
  try {
    key.open(AUS_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE, REG_PATH,
             AUS_Ci.nsIWindowsRegKey.ACCESS_READ | key.WOW64_64);
  } catch (e) {
#ifndef DISABLE_UPDATER_AUTHENTICODE_CHECK
    
    
    if (isBinarySigned(updaterBinPath)) {
      do_throw("binary is signed but the test registry key does not exists!");
    }
#endif

    logTestInfo("this test can only run on the buildbot build system at this " +
                "time.");
    return false;
  }

  
  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let args = ["wait-for-service-stop", "MozillaMaintenance", "10"];
  let process = AUS_Cc["@mozilla.org/process/util;1"].
                createInstance(AUS_Ci.nsIProcess);
  process.init(helperBin);
  logTestInfo("checking if the service exists on this machine.");
  process.run(true, args, args.length);
  if (process.exitValue == 0xEE) {
    do_throw("test registry key exists but this test can only run on systems " +
             "with the maintenance service installed.");
  } else {
    logTestInfo("service exists, return value: " + process.exitValue);
  }

  
  
  
  if (aFirstTest && process.exitValue != 0) {
    do_throw("First test, check for service stopped state returned error " +
             process.exitValue);
  }

#ifndef DISABLE_UPDATER_AUTHENTICODE_CHECK
  if (!isBinarySigned(updaterBinPath)) {
    logTestInfo("test registry key exists but this test can only run on " +
                "builds with signed binaries when " +
                "DISABLE_UPDATER_AUTHENTICODE_CHECK is not defined");
    do_throw("this test can only run on builds with signed binaries.");
  }
#endif
  return true;
}







function isBinarySigned(aBinPath) {
  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let args = ["check-signature", aBinPath];
  let process = AUS_Cc["@mozilla.org/process/util;1"].
                createInstance(AUS_Ci.nsIProcess);
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
  logTestInfo("start - copying or creating symlinks for application files " +
              "for the test");

  let srcDir = getCurrentProcessDir();
  let destDir = getApplyDirFile(null, true);
  if (!destDir.exists()) {
    try {
      destDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
    } catch (e) {
      logTestInfo("unable to create directory, Path: " + destDir.path +
                  ", Exception: " + e);
      do_throw(e);
    }
  }

  
  
  let fileRelPaths = [FILE_APP_BIN, FILE_UPDATER_BIN,
                      "application.ini", "dependentlibs.list"];

  
  if (IS_UNIX && !IS_MACOSX) {
    fileRelPaths.push("icons/updater.png");
  }

  
  
  let deplibsFile = srcDir.clone();
  deplibsFile.append("dependentlibs.list");
  let istream = AUS_Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(AUS_Ci.nsIFileInputStream);
  istream.init(deplibsFile, 0x01, 0o444, 0);
  istream.QueryInterface(AUS_Ci.nsILineInputStream);

  let hasMore;
  let line = {};
  do {
    hasMore = istream.readLine(line);
    fileRelPaths.push(line.value);
  } while(hasMore);

  istream.close();

  fileRelPaths.forEach(function CMAF_FLN_FE(aFileRelPath) {
    copyFileToTestAppDir(aFileRelPath);
  });

  logTestInfo("finish - copying or creating symlinks for application files " +
              "for the test");
}








function copyFileToTestAppDir(aFileRelPath) {
  let fileRelPath = aFileRelPath;
  let srcFile = gGREDirOrig.clone();
  let pathParts = fileRelPath.split("/");
  for (let i = 0; i < pathParts.length; i++) {
    if (pathParts[i]) {
      srcFile.append(pathParts[i]);
    }
  }

  if (IS_MACOSX && !srcFile.exists()) {
    logTestInfo("unable to copy file since it doesn't exist! Checking if " +
                 fileRelPath + ".app exists. Path: " +
                 srcFile.path);
    srcFile = gGREDirOrig.clone();
    for (let i = 0; i < pathParts.length; i++) {
      if (pathParts[i]) {
        srcFile.append(pathParts[i] + (pathParts.length - 1 == i ? ".app" : ""));
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
  let destFile = getApplyDirFile(DIR_BIN_REL_PATH + fileRelPath, true);
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
      let ln = AUS_Cc["@mozilla.org/file/local;1"].createInstance(AUS_Ci.nsILocalFile);
      ln.initWithPath("/bin/ln");
      let process = AUS_Cc["@mozilla.org/process/util;1"].createInstance(AUS_Ci.nsIProcess);
      process.init(ln);
      let args = ["-s", srcFile.path, destFile.path];
      process.run(true, args, args.length);
      logTestInfo("verifying symlink. Path: " + destFile.path);
      do_check_true(destFile.isSymlink());
    } catch (e) {
      do_throw("Unable to create symlink for file! Path: " + srcFile.path +
               ", Exception: " + e);
    }
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
  logTestInfo("starting installer process...");
  installerProcess.run(true, [], 0);
}












function runUpdateUsingService(aInitialStatus, aExpectedStatus, aCheckSvcLog) {
  
  function checkServiceLogs(aOriginalContents) {
    let contents = readServiceLogFile();
    logTestInfo("the contents of maintenanceservice.log:\n" + contents + "\n");
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
    logTestInfo("waiting for service to stop if necessary...");
    
    
    let helperBin = getTestDirFile(FILE_HELPER_BIN);
    let helperBinArgs = ["wait-for-service-stop",
                         "MozillaMaintenance",
                         "120"];
    let helperBinProcess = AUS_Cc["@mozilla.org/process/util;1"].
                           createInstance(AUS_Ci.nsIProcess);
    helperBinProcess.init(helperBin);
    logTestInfo("stopping service...");
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
      logTestInfo("service stopped.");
    }
    waitServiceApps();
  }
  function waitForApplicationStop(aApplication) {
    logTestInfo("waiting for " + aApplication + " to stop if " +
                "necessary...");
    
    
    let helperBin = getTestDirFile(FILE_HELPER_BIN);
    let helperBinArgs = ["wait-for-application-exit",
                         aApplication,
                         "120"];
    let helperBinProcess = AUS_Cc["@mozilla.org/process/util;1"].
                           createInstance(AUS_Ci.nsIProcess);
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
  let file = updatesDir.clone();
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

  
  
  
  copyFileToTestAppDir(FILE_UPDATER_BIN);

  
  
  
  
  copyFileToTestAppDir(FILE_MAINTENANCE_SERVICE_BIN);
  copyFileToTestAppDir(FILE_MAINTENANCE_SERVICE_INSTALLER_BIN);

  let launchBin = getLaunchBin();
  let args = getProcessArgs(["-dump-args", appArgsLogPath]);

  let process = AUS_Cc["@mozilla.org/process/util;1"].
                   createInstance(AUS_Ci.nsIProcess);
  process.init(launchBin);
  logTestInfo("launching " + launchBin.path + " " + args.join(" "));
  
  
  
  
  
  process.run(true, args, args.length);

  resetEnvironment();

  function timerCallback(aTimer) {
    
    let status = readStatusState();
    
    
    
    if (status == STATE_APPLYING ||
        status == STATE_PENDING_SVC) {
      logTestInfo("still waiting to see the " + aExpectedStatus +
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
      logTestInfo("contents of " + updateLog.path + ":\n" +
                  readFileBytes(updateLog).replace(/\r\n/g, "\n"));
    }
    logTestInfo("testing update status against expected status");
    do_check_eq(status, aExpectedStatus);

    if (aCheckSvcLog) {
      checkServiceLogs(svcOriginalLog);
    }

    checkUpdateFinished();
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
  } else {
    launchBin = AUS_Cc["@mozilla.org/file/local;1"].
                createInstance(AUS_Ci.nsILocalFile);
    launchBin.initWithPath("/bin/sh");
  }

  if (!launchBin.exists())
    do_throw(launchBin.path + " must exist to run this test!");

  return launchBin;
}





function waitForHelperSleep() {
  gTimeoutRuns++;
  
  
  let output = getApplyDirFile("a/b/output", true);
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
    logTestInfo("failed to remove file. Path: " + output.path);
    do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
    return;
  }
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
  } catch (e) {
    
    
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








function setupUpdaterTest(aMarFile, aUpdatedDirExists, aToBeDeletedDirExists) {
  let updatesPatchDir = getUpdatesPatchDir();
  if (!updatesPatchDir.exists()) {
    updatesPatchDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
  }
  
  let mar = getTestDirFile(aMarFile);
  mar.copyToFollowingLinks(updatesPatchDir, FILE_UPDATE_ARCHIVE);

  createUpdateSettingsINI();

  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let afterApplyBinDir = getApplyDirFile("a/b/", true);
  helperBin.copyToFollowingLinks(afterApplyBinDir, gCallbackBinFile);
  helperBin.copyToFollowingLinks(afterApplyBinDir, gPostUpdateBinFile);

  let applyToDir = getApplyDirFile(null, true);
  gTestFiles.forEach(function SUT_TF_FE(aTestFile) {
    if (aTestFile.originalFile || aTestFile.originalContents) {
      let testDir = getApplyDirFile(aTestFile.relPathDir, true);
      if (!testDir.exists())
        testDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);

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

  gTestExtraDirs[0].dirExists = aUpdatedDirExists;
  gTestExtraDirs[1].dirExists = IS_WIN ? aToBeDeletedDirExists : false;
}





function createUpdateSettingsINI() {
  let updateSettingsIni = getApplyDirFile(null, true);
  if (IS_MACOSX) {
    updateSettingsIni.append("Contents");
    updateSettingsIni.append("MacOS");
  }
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  writeFile(updateSettingsIni, UPDATE_SETTINGS_CONTENTS);
}










function createUpdaterINI(aIsExeAsync) {
  let exeArg = "ExeArg=post-update-async\n";
  let exeAsync = "";
  if (aIsExeAsync !== undefined)
  {
    if (aIsExeAsync) {
      exeAsync = "ExeAsync=true\n";
    } else {
      exeArg = "ExeArg=post-update-sync\n";
      exeAsync = "ExeAsync=false\n";
    }
  }

  let updaterIniContents = "[PostUpdateMac]\n" +
                           "ExeRelPath=a/b/" + gPostUpdateBinFile + "\n" +
                           exeArg +
                           exeAsync +
                           "\n" +
                           "[PostUpdateWin]\n" +
                           "ExeRelPath=a/b/" + gPostUpdateBinFile + "\n" +
                           exeArg +
                           exeAsync;
  let updaterIni = getApplyDirFile((IS_MACOSX ? "Contents/MacOS/" : "") +
                                    FILE_UPDATER_INI, true);
  writeFile(updaterIni, updaterIniContents);
}








function checkUpdateLogContents(aCompareLogFile) {
  let updateLog = getUpdatesPatchDir();
  updateLog.append(FILE_UPDATE_LOG);
  let updateLogContents = readFileBytes(updateLog);

  
  
  if (gTestFiles.length > 1 &&
	  gTestFiles[gTestFiles.length - 1].fileName == "channel-prefs.js" &&
	  !gTestFiles[gTestFiles.length - 1].originalContents) {
    updateLogContents = updateLogContents.replace(/.* a\/b\/defaults\/.*/g, "");
  }
  if (gTestFiles.length > 2 &&
	  gTestFiles[gTestFiles.length - 2].fileName == FILE_UPDATE_SETTINGS_INI &&
	  !gTestFiles[gTestFiles.length - 2].originalContents) {
    updateLogContents = updateLogContents.replace(/.* a\/b\/update-settings.ini.*/g, "");
  }
  if (gStageUpdate) {
    
    updateLogContents = updateLogContents.replace(/Performing a staged update/, "");
  } else if (gSwitchApp) {
    
    updateLogContents = updateLogContents.replace(/Performing a staged update/, "");
    updateLogContents = updateLogContents.replace(/Performing a replace request/, "");
  }
  
  updateLogContents = updateLogContents.replace(/SOURCE DIRECTORY.*/g, "");
  updateLogContents = updateLogContents.replace(/DESTINATION DIRECTORY.*/g, "");
  
  updateLogContents = updateLogContents.replace(/NS_main: callback app file .*/g, "");
  if (gSwitchApp) {
    
    updateLogContents = updateLogContents.replace(/^Begin moving.*$/mg, "");
    if (IS_MACOSX) {
      
      
      updateLogContents = updateLogContents.replace(/\n/g, "%%%EOL%%%");
      updateLogContents = updateLogContents.replace(/Moving the precomplete file.*Finished moving the precomplete file/, "");
      updateLogContents = updateLogContents.replace(/%%%EOL%%%/g, "\n");
    }
  }
  updateLogContents = updateLogContents.replace(/\r/g, "");
  
  updateLogContents = updateLogContents.replace(/, err:.*\n/g, "\n");
  
  updateLogContents = updateLogContents.replace(/non-fatal error /g, "");
  
  
  updateLogContents = updateLogContents.replace(/.* a\/b\/7\/7text.*\n/g, "");
  
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
    compareLogContents = compareLogContents.replace(/.* a\/b\/defaults\/.*/g, "");
  }
  if (gTestFiles.length > 2 &&
	  gTestFiles[gTestFiles.length - 2].fileName == FILE_UPDATE_SETTINGS_INI &&
	  !gTestFiles[gTestFiles.length - 2].originalContents) {
    compareLogContents = compareLogContents.replace(/.* a\/b\/update-settings.ini.*/g, "");
  }
  
  compareLogContents = compareLogContents.replace(/\n+/g, "\n");
  
  compareLogContents = compareLogContents.replace(/^\n|\n$/g, "");

  
  
  if (compareLogContents == updateLogContents) {
    logTestInfo("log contents are correct");
    do_check_true(true);
  } else {
    logTestInfo("log contents are not correct");
    do_check_eq(compareLogContents, updateLogContents);
  }
}







function checkUpdateLogContains(aCheckString) {
  let updateLog = getUpdatesPatchDir();
  updateLog.append(FILE_UPDATE_LOG);
  let updateLogContents = readFileBytes(updateLog);
  if (updateLogContents.indexOf(aCheckString) != -1) {
    logTestInfo("log file does contain: " + aCheckString);
    do_check_true(true);
  } else {
    logTestInfo("log file does not contain: " + aCheckString);
    logTestInfo("log file contents:\n" + updateLogContents);
    do_check_true(false);
  }
}





function checkFilesAfterUpdateSuccess() {
  logTestInfo("testing contents of files after a successful update");
  gTestFiles.forEach(function CFAUS_TF_FE(aTestFile) {
    let testFile = getTargetDirFile(aTestFile.relPathDir + aTestFile.fileName,
                                    true);
    logTestInfo("testing file: " + testFile.path);
    if (aTestFile.compareFile || aTestFile.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && aTestFile.comparePerms) {
        
        let logPerms = "testing file permissions - ";
        if (aTestFile.originalPerms) {
          logPerms += "original permissions: " +
                      aTestFile.originalPerms.toString(8) + ", ";
        }
        logPerms += "compare permissions : " +
                    aTestFile.comparePerms.toString(8) + ", ";
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
      } else {
        logTestInfo("file contents are not correct");
        do_check_eq(fileContents1, fileContents2);
      }
    } else {
      do_check_false(testFile.exists());
    }
  });

  logTestInfo("testing operations specified in removed-files were performed " +
              "after a successful update");
  gTestDirs.forEach(function CFAUS_TD_FE(aTestDir) {
    let testDir = getTargetDirFile(aTestDir.relPathDir, true);
    logTestInfo("testing directory: " + testDir.path);
    if (aTestDir.dirRemoved) {
      do_check_false(testDir.exists());
    } else {
      do_check_true(testDir.exists());

      if (aTestDir.files) {
        aTestDir.files.forEach(function CFAUS_TD_F_FE(aTestFile) {
          let testFile = getTargetDirFile(aTestDir.relPathDir + aTestFile, true);
          logTestInfo("testing directory file: " + testFile.path);
          if (aTestDir.filesRemoved) {
            do_check_false(testFile.exists());
          } else {
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
  gTestFiles.forEach(function CFAUF_TF_FE(aTestFile) {
    let testFile = getdir(aTestFile.relPathDir + aTestFile.fileName, true);
    logTestInfo("testing file: " + testFile.path);
    if (aTestFile.compareFile || aTestFile.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && aTestFile.comparePerms) {
        
        let logPerms = "testing file permissions - ";
        if (aTestFile.originalPerms) {
          logPerms += "original permissions: " +
                      aTestFile.originalPerms.toString(8) + ", ";
        }
        logPerms += "compare permissions : " +
                    aTestFile.comparePerms.toString(8) + ", ";
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
      } else {
        logTestInfo("file contents are not correct");
        do_check_eq(fileContents1, fileContents2);
      }
    } else {
      do_check_false(testFile.exists());
    }
  });

  logTestInfo("testing operations specified in removed-files were not " +
              "performed after a failed update");
  gTestDirs.forEach(function CFAUF_TD_FE(aTestDir) {
    let testDir = getdir(aTestDir.relPathDir, true);
    logTestInfo("testing directory: " + testDir.path);
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
            let testFile = getdir(aTestDir.relPathDir + aSubDir + aTestFile,
                                  true);
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
  logTestInfo("testing extra directories");
  gTestExtraDirs.forEach(function CFAUC_TED_FE(aTestExtraDir) {
    let testDir = getTargetDirFile(aTestExtraDir.relPathDir, true);
    logTestInfo("testing directory: " + testDir.path);
    if (aTestExtraDir.dirExists) {
      do_check_true(testDir.exists());
    } else {
      do_check_false(testDir.exists());
    }
  });

  logTestInfo("testing updating directory doesn't exist in the application " +
	          "directory");
  let updatingDir = getTargetDirFile("updating", true);
  do_check_false(updatingDir.exists());

  if (gStageUpdate) {
    logTestInfo("testing updating directory doesn't exist in the updated " +
		        "directory");
    updatingDir = getApplyDirFile("updating", true);
    do_check_false(updatingDir.exists());

    
	
    logTestInfo("testing tobedeleted directory doesn't exist in the updated " +
                "directory");
    let toBeDeletedDir = getApplyDirFile(DIR_TOBEDELETED, true);
    do_check_false(toBeDeletedDir.exists());
  }

  logTestInfo("testing patch files should not be left behind");
  let updatesDir = getUpdatesPatchDir();
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
  } else {
    logTestInfo("callback log file contents are not correct");
    do_check_eq(logContents, expectedLogContents);
  }

  waitForFilesInUse();
}









function getPostUpdateFile(aSuffix) {
  return getApplyDirFile("a/b/" + gPostUpdateBinFile + aSuffix, true);
}





function checkPostUpdateAppLog() {
  gTimeoutRuns++;
  let postUpdateLog = getPostUpdateFile(".log");
  if (!postUpdateLog.exists()) {
    logTestInfo("postUpdateLog does not exist");
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

  logTestInfo("post update app log file contents are correct");
  do_check_true(true);

  gCheckFunc();
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

    for (var i = 0; i < files.length; ++i) {
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
          logTestInfo("file is not in use. Path: " + file.path);
        } catch (e) {
          logTestInfo("file in use, will try again after " + TEST_CHECK_TIMEOUT +
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

  logTestInfo("calling doTestFinish");
  doTestFinish();
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
    } else {
      aCallback(entry);
    }
  }
}
















function overrideXHR(aCallback) {
  gXHRCallback = aCallback;
  gXHR = new xhr();
  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(gXHR.classID, gXHR.classDescription,
                            gXHR.contractID, gXHR);
}






function makeHandler(aVal) {
  if (typeof aVal == "function")
    return { handleEvent: aVal };
  return aVal;
}
function xhr() {
}
xhr.prototype = {
  overrideMimeType: function(aMimetype) { },
  setRequestHeader: function(aHeader, aValue) { },
  status: null,
  channel: { set notificationCallbacks(aVal) { } },
  _url: null,
  _method: null,
  open: function(aMethod, aUrl) {
    gXHR.channel.originalURI = Services.io.newURI(aUrl, null, null);
    gXHR._method = aMethod; gXHR._url = aUrl;
  },
  responseXML: null,
  responseText: null,
  send: function(aBody) {
    do_execute_soon(gXHRCallback); 
  },
  _onprogress: null,
  set onprogress(aValue) { gXHR._onprogress = makeHandler(aValue); },
  get onprogress() { return gXHR._onprogress; },
  _onerror: null,
  set onerror(aValue) { gXHR._onerror = makeHandler(aValue); },
  get onerror() { return gXHR._onerror; },
  _onload: null,
  set onload(aValue) { gXHR._onload = makeHandler(aValue); },
  get onload() { return gXHR._onload; },
  addEventListener: function(aEvent, aValue, aCapturing) {
    eval("gXHR._on" + aEvent + " = aValue");
  },
  flags: AUS_Ci.nsIClassInfo.SINGLETON,
  implementationLanguage: AUS_Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage: function(aLanguage) null,
  getInterfaces: function(aCount) {
    var interfaces = [AUS_Ci.nsISupports];
    aCount.value = interfaces.length;
    return interfaces;
  },
  classDescription: "XMLHttpRequest",
  contractID: "@mozilla.org/xmlextras/xmlhttprequest;1",
  classID: Components.ID("{c9b37f43-4278-4304-a5e0-600991ab08cb}"),
  createInstance: function(aOuter, aIID) {
    if (aOuter == null)
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








function overrideUpdatePrompt(aCallback) {
  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  gUpdatePrompt = new UpdatePrompt();
  gUpdatePromptCallback = aCallback;
  registrar.registerFactory(gUpdatePrompt.classID, gUpdatePrompt.classDescription,
                            gUpdatePrompt.contractID, gUpdatePrompt);
}

function UpdatePrompt() {
  var fns = ["checkForUpdates", "showUpdateAvailable", "showUpdateDownloaded",
             "showUpdateError", "showUpdateHistory", "showUpdateInstalled"];

  fns.forEach(function(aPromptFn) {
    UpdatePrompt.prototype[aPromptFn] = function() {
      if (!gUpdatePromptCallback) {
        return;
      }

      var callback = gUpdatePromptCallback[aPromptFn];
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
  getHelperForLanguage: function(aLanguage) null,
  getInterfaces: function(aCount) {
    var interfaces = [AUS_Ci.nsISupports, AUS_Ci.nsIUpdatePrompt];
    aCount.value = interfaces.length;
    return interfaces;
  },
  classDescription: "UpdatePrompt",
  contractID: "@mozilla.org/updates/update-prompt;1",
  classID: Components.ID("{8c350a15-9b90-4622-93a1-4d320308664b}"),
  createInstance: function(aOuter, aIID) {
    if (aOuter == null)
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
  onProgress: function UCL_onProgress(aRequest, aPosition, aTotalSize) {
  },

  onCheckComplete: function UCL_onCheckComplete(aRequest, aUpdates, aUpdateCount) {
    gRequestURL = aRequest.channel.originalURI.spec;
    gUpdateCount = aUpdateCount;
    gUpdates = aUpdates;
    logTestInfo("url = " + gRequestURL + ", " +
                "request.status = " + aRequest.status + ", " +
                "update.statusText = " + aRequest.statusText + ", " +
                "updateCount = " + aUpdateCount);
    
    do_execute_soon(gCheckFunc);
  },

  onError: function UCL_onError(aRequest, aUpdate) {
    gRequestURL = aRequest.channel.originalURI.spec;
    gStatusCode = aRequest.status;

    gStatusText = aUpdate.statusText;
    logTestInfo("url = " + gRequestURL + ", " +
                "request.status = " + gStatusCode + ", " +
                "update.statusText = " + gStatusText);
    
    do_execute_soon(gCheckFunc.bind(null, aRequest, aUpdate));
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
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
  logTestInfo("http server directory path: " + dir.path);

  if (!dir.isDirectory()) {
    do_throw("A file instead of a directory was specified for HttpServer " +
             "registerDirectory! Path: " + dir.path + "\n");
  }

  AUS_Cu.import("resource://testing-common/httpd.js");
  gTestserver = new HttpServer();
  gTestserver.registerDirectory("/", dir);
  gTestserver.start(-1);
  let testserverPort = gTestserver.identity.primaryPort;
  gURLData = URL_HOST + ":" + testserverPort + "/";
  logTestInfo("http server port = " + testserverPort);
}







function stop_httpserver(aCallback) {
  do_check_true(!!aCallback);
  gTestserver.stop(aCallback);
}













function createAppInfo(aID, aName, aVersion, aPlatformVersion) {
  const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
  const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");
  var XULAppInfo = {
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

    QueryInterface: function QueryInterface(aIID) {
      if (aIID.equals(AUS_Ci.nsIXULAppInfo) ||
          aIID.equals(AUS_Ci.nsIXULRuntime) ||
#ifdef XP_WIN
          aIID.equals(AUS_Ci.nsIWinAppHelper) ||
#endif
          aIID.equals(AUS_Ci.nsISupports))
        return this;
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    }
  };

  var XULAppInfoFactory = {
    createInstance: function (aOuter, aIID) {
      if (aOuter == null)
        return XULAppInfo.QueryInterface(aIID);
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

  let appBinPath = getApplyDirFile(DIR_BIN_REL_PATH + FILE_APP_BIN, false).path;
  if (/ /.test(appBinPath)) {
    appBinPath = '"' + appBinPath + '"';
  }

  let args;
  if (IS_UNIX) {
    let launchScript = getLaunchScript();
    
    launchScript.create(AUS_Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_DIRECTORY);

    let scriptContents = "#! /bin/sh\n";
    scriptContents += appBinPath + " -no-remote -process-updates " +
                      aExtraArgs.join(" ") + " " + PIPE_TO_NULL;
    writeFile(launchScript, scriptContents);
    logTestInfo("created " + launchScript.path + " containing:\n" +
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
            return getApplyDirFile(DIR_BIN_REL_PATH, true);
          }
          break;
        case XRE_EXECUTABLE_FILE:
          if (gUseTestAppDir) {
            return getApplyDirFile(DIR_BIN_REL_PATH + FILE_APP_BIN, true);
          }
          break;
        case XRE_UPDATE_ROOT_DIR:
          return getMockUpdRootD();
      }
      return null;
    },
    QueryInterface: function(aIID) {
      if (aIID.equals(AUS_Ci.nsIDirectoryServiceProvider) ||
          aIID.equals(AUS_Ci.nsISupports))
        return this;
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  let ds = Services.dirsvc.QueryInterface(AUS_Ci.nsIDirectoryService);
  ds.QueryInterface(AUS_Ci.nsIProperties).undefine(NS_GRE_DIR);
  ds.QueryInterface(AUS_Ci.nsIProperties).undefine(XRE_EXECUTABLE_FILE);
  ds.registerProvider(dirProvider);
  do_register_cleanup(function AGP_cleanup() {
    logTestInfo("start - unregistering directory provider");

    if (gAppTimer) {
      logTestInfo("start - cancel app timer");
      gAppTimer.cancel();
      gAppTimer = null;
      logTestInfo("finish - cancel app timer");
    }

    if (gProcess && gProcess.isRunning) {
      logTestInfo("start - kill process");
      try {
        gProcess.kill();
      } catch (e) {
        logTestInfo("kill process failed. Exception: " + e);
      }
      gProcess = null;
      logTestInfo("finish - kill process");
    }

    if (gHandle) {
      try {
        logTestInfo("start - closing handle");
        let kernel32 = ctypes.open("kernel32");
        let CloseHandle = kernel32.declare("CloseHandle", ctypes.default_abi,
                                           ctypes.bool, 
                                           ctypes.voidptr_t );
        if (!CloseHandle(gHandle)) {
          logTestInfo("call to CloseHandle failed");
        }
        kernel32.close();
        gHandle = null;
        logTestInfo("finish - closing handle");
      } catch (e) {
        logTestInfo("call to CloseHandle failed. Exception: " + e);
      }
    }

    
    if (typeof(end_test) == typeof(Function)) {
      logTestInfo("calling end_test");
      end_test();
    }

    ds.unregisterProvider(dirProvider);
    cleanupTestCommon();

    logTestInfo("finish - unregistering directory provider");
  });
}





function launchAppToApplyUpdate() {
  logTestInfo("start - launching application to apply update");

  let appBin = getApplyDirFile(DIR_BIN_REL_PATH + FILE_APP_BIN, false);

  if (typeof(customLaunchAppToApplyUpdate) == typeof(Function)) {
    customLaunchAppToApplyUpdate();
  }

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
  logTestInfo("launching application");
  gProcess.runAsync(args, args.length, gProcessObserver);
  resetEnvironment();

  logTestInfo("finish - launching application to apply update");
}




let gProcessObserver = {
  observe: function PO_observe(aSubject, aTopic, aData) {
    logTestInfo("topic: " + aTopic + ", process exitValue: " +
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
  QueryInterface: XPCOMUtils.generateQI([AUS_Ci.nsIObserver])
};




let gTimerCallback = {
  notify: function TC_notify(aTimer) {
    gAppTimer = null;
    if (gProcess.isRunning) {
      logTestInfo("attempt to kill process");
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
    let appGreDir = gGREDirOrig.clone();
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
    } else {
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
  } else {
    logTestInfo("setting the XPCOM_DEBUG_BREAK environment variable to " +
                "warn... previously it didn't exist");
  }

  env.set("XPCOM_DEBUG_BREAK", "warn");

  if (gStageUpdate) {
    logTestInfo("setting the MOZ_UPDATE_STAGING environment variable to 1\n");
    env.set("MOZ_UPDATE_STAGING", "1");
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
  } else {
    logTestInfo("clearing the XPCOM_DEBUG_BREAK environment variable");
    env.set("XPCOM_DEBUG_BREAK", "");
  }

  if (IS_UNIX) {
    if (IS_MACOSX) {
      if (gEnvDyldLibraryPath) {
        logTestInfo("setting DYLD_LIBRARY_PATH environment variable value " +
                    "back to " + gEnvDyldLibraryPath);
        env.set("DYLD_LIBRARY_PATH", gEnvDyldLibraryPath);
      } else {
        logTestInfo("removing DYLD_LIBRARY_PATH environment variable");
        env.set("DYLD_LIBRARY_PATH", "");
      }
    } else {
      if (gEnvLdLibraryPath) {
        logTestInfo("setting LD_LIBRARY_PATH environment variable value back " +
                    "to " + gEnvLdLibraryPath);
        env.set("LD_LIBRARY_PATH", gEnvLdLibraryPath);
      } else {
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

  if (gStageUpdate) {
    logTestInfo("removing the MOZ_UPDATE_STAGING environment variable\n");
    env.set("MOZ_UPDATE_STAGING", "");
  }

  logTestInfo("removing MOZ_NO_SERVICE_FALLBACK environment variable");
  env.set("MOZ_NO_SERVICE_FALLBACK", "");
}
