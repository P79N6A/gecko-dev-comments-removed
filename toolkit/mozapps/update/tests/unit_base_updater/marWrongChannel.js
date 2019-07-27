






function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  
  
  gTestFiles = gTestFilesCommon;
  gTestDirs = [];
  setupUpdaterTest(FILE_WRONG_CHANNEL_MAR);

  createUpdaterINI();

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_CHANNEL_MISMATCH_ERROR,
            checkUpdateApplied);
}





function checkUpdateApplied() {
  checkPostUpdateRunningFile(false);
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  doTestFinish();
}
