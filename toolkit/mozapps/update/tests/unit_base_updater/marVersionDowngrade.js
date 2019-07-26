








const TEST_FILES = [];
const VERSION_DOWNGRADE_ERROR = "23";

function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  setupUpdaterTest(FILE_OLD_VERSION_MAR);

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1));
}

function checkUpdateApplied() {
  do_check_eq(readStatusFailedCode(), VERSION_DOWNGRADE_ERROR);
  do_test_finished();
}
