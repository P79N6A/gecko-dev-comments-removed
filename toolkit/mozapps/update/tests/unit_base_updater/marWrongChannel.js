








const TEST_FILES = [];
const MAR_CHANNEL_MISMATCH_ERROR = "22";

function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  setupUpdaterTest(FILE_WRONG_CHANNEL_MAR);

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1));
}

function checkUpdateApplied() {
  
  do_check_eq(readStatusFailedCode(), MAR_CHANNEL_MISMATCH_ERROR);
  do_test_finished();
}
