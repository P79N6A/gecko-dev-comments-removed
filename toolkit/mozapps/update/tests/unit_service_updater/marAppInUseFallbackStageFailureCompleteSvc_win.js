





function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_COMPLETE_MAR);

  
  let callbackApp = getApplyDirFile(DIR_RESOURCES + gCallbackBinFile);
  let args = [getApplyDirPath() + DIR_RESOURCES, "input", "output", "-s",
              HELPER_SLEEP_TIMEOUT];
  let callbackAppProcess = Cc["@mozilla.org/process/util;1"].
                           createInstance(Ci.nsIProcess);
  callbackAppProcess.init(callbackApp);
  callbackAppProcess.run(false, args, args.length);

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
}

function doUpdate() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_APPLIED);
}

function checkUpdateFinished() {
  
  gStageUpdate = false;
  gSwitchApp = true;
  runUpdate(1, STATE_PENDING);
}

function checkUpdateApplied() {
  setupHelperFinish();
}

function checkUpdate() {
  checkFilesAfterUpdateFailure(getApplyDirFile, false, false);
  checkUpdateLogContains(ERR_RENAME_FILE);
  checkCallbackAppLog();
}
