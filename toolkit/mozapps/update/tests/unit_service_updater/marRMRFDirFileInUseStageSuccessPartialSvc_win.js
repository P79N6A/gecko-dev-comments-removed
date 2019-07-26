








const TEST_FILES = [
{
  description      : "Should never change",
  fileName         : "channel-prefs.js",
  relPathDir       : "a/b/defaults/pref/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "precomplete",
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete_precomplete",
  compareFile      : "complete_precomplete"
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginstext0",
  relPathDir       : "a/b/searchplugins/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "ToBeReplacedWithFromPartial\n",
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Patched by update.manifest if the file exists " +
                     "(patch-if)",
  fileName         : "searchpluginspng1.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Patched by update.manifest if the file exists " +
                     "(patch-if)",
  fileName         : "searchpluginspng0.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions1text0",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions1png1.png",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions1png0.png",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions0text0",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "ToBeReplacedWithFromPartial\n",
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions0png1.png",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions0png0.png",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "exe0.exe",
  relPathDir       : "a/b/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "0exe0.exe",
  relPathDir       : "a/b/0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text0",
  relPathDir       : "a/b/0/00/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "ToBeReplacedWithFromPartial\n",
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "00png0.png",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "complete.png"
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20text0",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20png0.png",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text2",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "10text0",
  relPathDir       : "a/b/1/10/",
  originalContents : "ToBeDeleted\n",
  compareContents  : "ToBeDeleted\n",
  originalFile     : null,
  compareFile      : null
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "00text1",
  relPathDir       : "a/b/0/00/",
  originalContents : "ToBeDeleted\n",
  compareContents  : "ToBeDeleted\n",
  originalFile     : null,
  compareFile      : null
}];

ADDITIONAL_TEST_DIRS = [
{
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : "a/b/1/10/",
  dirRemoved   : true
}, {
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : "a/b/1/",
  dirRemoved   : true
}];

function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  gStageUpdate = true;
  setupTestCommon();
  setupUpdaterTest(FILE_PARTIAL_MAR);

  let fileInUseBin = getApplyDirFile(TEST_DIRS[2].relPathDir +
                                     TEST_DIRS[2].files[0]);
  
  
  fileInUseBin.remove(false);

  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let fileInUseDir = getApplyDirFile(TEST_DIRS[2].relPathDir);
  helperBin.copyTo(fileInUseDir, TEST_DIRS[2].files[0]);

  
  let args = [getApplyDirPath() + "a/b/", "input", "output", "-s", "40"];
  let fileInUseProcess = AUS_Cc["@mozilla.org/process/util;1"].
                         createInstance(AUS_Ci.nsIProcess);
  fileInUseProcess.init(fileInUseBin);
  fileInUseProcess.run(false, args, args.length);

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
}

function doUpdate() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_APPLIED);
}

function checkUpdateFinished() {
  logTestInfo("testing update.status should be " + STATE_APPLIED);
  do_check_eq(readStatusState(), STATE_APPLIED);

  
  gStageUpdate = false;
  gSwitchApp = true;
  gDisableReplaceFallback = true;
  runUpdate(1);
}

function checkUpdateApplied() {
  setupHelperFinish();
}

function checkUpdate() {
  logTestInfo("testing update.status should be " + STATE_FAILED);
  do_check_eq(readStatusState(), STATE_FAILED);

  checkFilesAfterUpdateFailure(getApplyDirFile);
  checkUpdateLogContains(ERR_RENAME_FILE);

  logTestInfo("testing tobedeleted directory does not exist");
  let toBeDeletedDir = getApplyDirFile("tobedeleted", true);
  do_check_false(toBeDeletedDir.exists());

  checkCallbackAppLog();
}
