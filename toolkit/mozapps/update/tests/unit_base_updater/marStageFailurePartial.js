






function run_test() {
  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestFiles[11].originalFile = "partial.png";
  gTestDirs = gTestDirsPartialSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_PARTIAL_MAR);

  createUpdaterINI(true);

  runUpdate(1, STATE_FAILED_LOADSOURCE_ERROR_WRONG_SIZE, checkUpdateFinished);
}





function checkUpdateFinished() {
  checkPostUpdateRunningFile(false);
  checkFilesAfterUpdateFailure(getApplyDirFile, true, false);
  checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  standardInit();
  waitForFilesInUse();
}
