








const TEST_FILES = [];

function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  setupUpdaterTest(FILE_WRONG_CHANNEL_MAR);

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_CHANNEL_MISMATCH_ERROR);
}

function checkUpdateApplied() {
  doTestFinish();
}
