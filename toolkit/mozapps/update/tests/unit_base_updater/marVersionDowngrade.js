








const TEST_FILES = [];

const VERSION_DOWNGRADE_ERROR = "23";

function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon(true);

  
  setupUpdaterTest(FILE_OLD_VERSION_MAR);

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for failure when " +
              "applying a version downgrade MAR");
  
  
  
  
  do_check_eq(exitValue, USE_EXECV ? 0 : 1);
  let updatesDir = do_get_file(gTestID + UPDATES_DIR_SUFFIX);

  
  let updateStatus = readStatusFile(updatesDir);
  do_check_eq(updateStatus.split(": ")[1], VERSION_DOWNGRADE_ERROR);
  do_test_finished();
}

function end_test() {
  cleanupUpdaterTest();
}
