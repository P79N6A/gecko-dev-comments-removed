





function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setupUpdaterTest(FILE_COMPLETE_MAR);

  
  let callbackApp = getApplyDirFile(DIR_RESOURCES + gCallbackBinFile);
  callbackApp.permissions = PERMS_DIRECTORY;
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
  setAppBundleModTime();
  runUpdateUsingService(STATE_PENDING_SVC, STATE_SUCCEEDED);
}

function checkUpdateFinished() {
  setupHelperFinish();
}

function checkUpdate() {
  checkAppBundleModTime();
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_COMPLETE_SUCCESS);
  standardInit();
  checkCallbackServiceLog();
}
