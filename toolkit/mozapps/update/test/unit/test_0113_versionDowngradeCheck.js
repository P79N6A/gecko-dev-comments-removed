






const TEST_ID = "0113-VDC";



const TEST_FILES = [];

const VERSION_DOWNGRADE_ERROR = "23";

function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  
  adjustGeneralPaths();

  
  setupUpdaterTest(MAR_OLD_VERSION_FILE);

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for failure when " +
              "applying a version downgrade MAR");
  
  
  
  
  do_check_eq(exitValue, USE_EXECV ? 0 : 1);
  let updatesDir = do_get_file(TEST_ID + UPDATES_DIR_SUFFIX);

  
  let updateStatus = readStatusFile(updatesDir);
  do_check_eq(updateStatus.split(": ")[1], VERSION_DOWNGRADE_ERROR);
}

function end_test() {
  cleanupUpdaterTest();
}
