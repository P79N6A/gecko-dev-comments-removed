






function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  
  
  gTestFiles = gTestFilesCommon;
  gTestDirs = [];
  setupUpdaterTest(FILE_WRONG_CHANNEL_MAR, false, false);

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_CHANNEL_MISMATCH_ERROR);
}

function checkUpdateApplied() {
  checkFilesAfterUpdateSuccess();
  doTestFinish();
}
