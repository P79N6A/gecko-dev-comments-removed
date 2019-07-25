





const TEST_ID = "0181";
const MAR_IN_USE_WIN_FILE = "data/partial_in_use_win.mar";


var TEST_FILES = [
{
  fileName         : "00png0.png",
  relPathDir       : "0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/partial.png"
}, {
  fileName         : "00text0",
  relPathDir       : "0/00/",
  originalContents : "ToBeModified\n",
  compareContents  : "Modified\n",
  originalFile     : null,
  compareFile      : null
}, {
  fileName         : "00text1",
  relPathDir       : "0/00/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null
}, {
  fileName         : "0exe0.exe",
  relPathDir       : "0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/partial_in_use_win_before.exe",
  compareFile      : "data/partial_in_use_win_after.exe"
}, {
  fileName         : "10text0",
  relPathDir       : "1/10/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null
}, {
  fileName         : "00text2",
  relPathDir       : "0/00/",
  originalContents : null,
  compareContents  : "Added\n",
  originalFile     : null,
  compareFile      : null
}, {
  fileName         : "20text0",
  relPathDir       : "2/20/",
  originalContents : null,
  compareContents  : "Added\n",
  originalFile     : null,
  compareFile      : null
}, {
  fileName         : "exe0.exe",
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/partial_in_use_win_before.exe",
  compareFile      : "data/partial_in_use_win_after.exe"
}];

function run_test() {
  if (!IS_WIN || IS_WINCE) {
    logTestInfo("this test is only applicable to Windows... returning early");
    return;
  }

  do_test_pending();
  do_register_cleanup(cleanupUpdaterTest);

  setupUpdaterTest(MAR_IN_USE_WIN_FILE);

  
  let fileInUseBin = getApplyDirFile(TEST_FILES[3].relPathDir +
                                    TEST_FILES[3].fileName);
  let args = [getApplyDirPath(), "input", "output", "-s", "20"];
  let fileInUseProcess = AUS_Cc["@mozilla.org/process/util;1"].
                         createInstance(AUS_Ci.nsIProcess);
  fileInUseProcess.init(fileInUseBin);
  fileInUseProcess.run(false, args, args.length);

  do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
}

function doUpdate() {
  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for success when " +
              "applying a partial mar");
  do_check_eq(exitValue, 0);

  setupHelperFinish();
}

function checkUpdate() {
  let updatesDir = do_get_file(TEST_ID + UPDATES_DIR_SUFFIX);
  let applyToDir = getApplyDirFile();

  logTestInfo("testing update.status should be " + STATE_SUCCEEDED);
  do_check_eq(readStatusFile(updatesDir), STATE_SUCCEEDED);

  checkFilesAfterUpdateSuccess();

  logTestInfo("testing tobedeleted directory exists");
  let toBeDeletedDir = applyToDir.clone();
  toBeDeletedDir.append("tobedeleted");
  do_check_true(toBeDeletedDir.exists());

  checkCallbackAppLog();
}
