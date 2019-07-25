





const TEST_ID = "0160";


const TEST_FILES = [
{
  fileName         : "1_1_image1.png",
  destinationDir   : TEST_ID + APPLY_TO_DIR_SUFFIX + "/mar_test/1/1_1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/partial.png",
  compareFile      : "data/partial.png"
}, {
  fileName         : "1_1_text1",
  destinationDir   : TEST_ID + APPLY_TO_DIR_SUFFIX + "/mar_test/1/1_1/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null
}, {
  fileName         : "1_1_text2",
  destinationDir   : TEST_ID + APPLY_TO_DIR_SUFFIX + "/mar_test/1/1_1/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null
}, {
  fileName         : "1_exe1.exe",
  destinationDir   : TEST_ID + APPLY_TO_DIR_SUFFIX + "/mar_test/1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/partial.png",
  compareFile      : "data/partial.png"
}, {
  fileName         : "2_1_text1",
  destinationDir   : TEST_ID + APPLY_TO_DIR_SUFFIX + "/mar_test/2/2_1/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null
}];

let gCallbackAppProcess;

function run_test() {
  if (!IS_WIN || IS_WINCE) {
    logTestInfo("this test is only applicable to Windows... returning early");
    return;
  }

  do_test_pending();
  do_register_cleanup(end_test);

  setupUpdaterTest(TEST_ID, MAR_COMPLETE_FILE, TEST_FILES);

  
  let callbackApp = do_get_file(TEST_ID + APPLY_TO_DIR_SUFFIX);
  callbackApp.append(AFTER_APPLY_DIR);
  callbackApp.append(CALLBACK_BIN_FILE);
  let args = ["-s", "20"];
  gCallbackAppProcess = AUS_Cc["@mozilla.org/process/util;1"].
                        createInstance(AUS_Ci.nsIProcess);
  gCallbackAppProcess.init(callbackApp);
  gCallbackAppProcess.run(false, args, args.length);

  
  
  do_timeout(100, testUpdate);
}

function end_test() {
  cleanupUpdaterTest(TEST_ID);
}

function testUpdate() {
  let updatesDir = do_get_file(TEST_ID + UPDATES_DIR_SUFFIX);
  let applyToDir = do_get_file(TEST_ID + APPLY_TO_DIR_SUFFIX);

  
  let exitValue = runUpdate(TEST_ID);
  logTestInfo("testing updater binary process exitValue for success when " +
              "applying a complete mar");
  do_check_eq(exitValue, 1);

  gCallbackAppProcess.kill();

  logTestInfo("testing update.status should be " + STATE_FAILED);
  
  
  do_check_eq(readStatusFile(updatesDir).split(": ")[0], STATE_FAILED);

  checkFilesAfterUpdateFailure(TEST_ID, TEST_FILES);

  logTestInfo("testing tobedeleted directory doesn't exist");
  let toBeDeletedDir = applyToDir.clone();
  toBeDeletedDir.append("tobedeleted");
  do_check_false(toBeDeletedDir.exists());

  checkCallbackAppLog(TEST_ID);
}
