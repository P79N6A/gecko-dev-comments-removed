







































const TEST_ID = "0113";



const TEST_FILES = [];

const VERSION_DOWNGRADE_ERROR = "23";

function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  
  do_register_cleanup(cleanupUpdaterTest);
  setupUpdaterTest(MAR_OLD_VERSION_FILE);

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for failure when " +
              "applying a version downgrade MAR");
  
  do_check_eq(exitValue, 0);
  let updatesDir = do_get_file(TEST_ID + UPDATES_DIR_SUFFIX);

  
  let updateStatus = readStatusFile(updatesDir);
  do_check_eq(updateStatus.split(": ")[1], VERSION_DOWNGRADE_ERROR);
}
