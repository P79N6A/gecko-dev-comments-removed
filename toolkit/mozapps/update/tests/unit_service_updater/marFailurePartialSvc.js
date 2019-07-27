






function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestFiles[11].originalFile = "partial.png";
  gTestDirs = gTestDirsPartialSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_PARTIAL_MAR);

  createUpdaterINI();
  setAppBundleModTime();

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  runUpdateUsingService(STATE_PENDING_SVC,
                        STATE_FAILED_LOADSOURCE_ERROR_WRONG_SIZE);
}





function checkUpdateFinished() {
  checkPostUpdateRunningFile(false);
  checkAppBundleModTime();
  checkFilesAfterUpdateFailure(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  standardInit();
  checkCallbackServiceLog();
}
