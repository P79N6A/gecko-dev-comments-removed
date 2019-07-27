






function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  
  
  gTestFiles = gTestFilesCommon;
  gTestDirs = [];
  setupUpdaterTest(FILE_OLD_VERSION_MAR);

  createUpdaterINI(true);

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_VERSION_DOWNGRADE_ERROR,
            checkUpdateApplied);
}





function checkUpdateApplied() {
  checkPostUpdateRunningFile(false);
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  doTestFinish();
}
