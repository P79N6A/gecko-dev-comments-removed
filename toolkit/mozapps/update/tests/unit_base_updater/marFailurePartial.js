






function run_test() {
  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestFiles[11].originalFile = "partial.png";
  gTestDirs = gTestDirsPartialSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_PARTIAL_MAR);

  createUpdaterINI();
  setAppBundleModTime();

  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_LOADSOURCE_ERROR_WRONG_SIZE,
            checkUpdateFinished);
}





function checkUpdateFinished() {
  checkPostUpdateRunningFile(false);
  checkAppBundleModTime();
  checkFilesAfterUpdateFailure(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  standardInit();
  checkCallbackAppLog();
}
