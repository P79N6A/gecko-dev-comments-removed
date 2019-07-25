







































const TEST_ID = "0114";



const TEST_FILES = [];

const MAR_CHANNEL_MISMATCH_ERROR = "22";

function run_test() {
  
  do_register_cleanup(cleanupUpdaterTest);
  setupUpdaterTest(MAR_WRONG_CHANNEL_FILE);

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for failure when " +
              "applying a wrong product and channel MAR file");
  
  do_check_eq(exitValue, 0);
  let updatesDir = do_get_file(TEST_ID + UPDATES_DIR_SUFFIX);

  
  let updateStatus = readStatusFile(updatesDir);
  do_check_eq(updateStatus.split(": ")[1], MAR_CHANNEL_MISMATCH_ERROR);
}
