







































const TEST_ID = "0111";



const MAX_TIME_DIFFERENCE = 60000;


const TEST_FILES = [
{
  fileName         : "00png0.png",
  relPathDir       : "0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/partial.png",
  originalPerms    : 0644,
  comparePerms     : null
}, {
  fileName         : "00text0",
  relPathDir       : "0/00/",
  originalContents : "ToBeModified\n",
  compareContents  : "Modified\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0644,
  comparePerms     : null
}, {
  fileName         : "00text1",
  relPathDir       : "0/00/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  fileName         : "0exe0.exe",
  relPathDir       : "0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/partial.png",
  originalPerms    : 0755,
  comparePerms     : null
}, {
  fileName         : "10text0",
  relPathDir       : "1/10/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  fileName         : "00text2",
  relPathDir       : "0/00/",
  originalContents : null,
  compareContents  : "Added\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0644
}, {
  fileName         : "20text0",
  relPathDir       : "2/20/",
  originalContents : null,
  compareContents  : "Added\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0644
}, {
  fileName         : "exe0.exe",
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/partial.png",
  originalPerms    : 0755,
  comparePerms     : null
}];

function run_test() {
  if (IS_ANDROID) {
    logTestInfo("this test is not applicable to Android... returning early");
    return;
  }

  do_test_pending();
  do_register_cleanup(cleanupUpdaterTest);

  setupUpdaterTest(MAR_PARTIAL_FILE);

  let updatesDir = do_get_file(TEST_ID + UPDATES_DIR_SUFFIX);
  let applyToDir = getApplyDirFile();

  
  
  
  if (IS_MACOSX) {
    let now = Date.now();
    let yesterday = now - (1000 * 60 * 60 * 24);
    applyToDir.lastModifiedTime = yesterday;
  }

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for success when " +
              "applying a partial mar");
  do_check_eq(exitValue, 0);

  logTestInfo("testing update.status should be " + STATE_SUCCEEDED);
  do_check_eq(readStatusFile(updatesDir), STATE_SUCCEEDED);

  
  
  if (IS_MACOSX) {
    logTestInfo("testing last modified time on the apply to directory has " +
                "changed after a successful update (bug 600098)");
    let now = Date.now();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAX_TIME_DIFFERENCE);
  }

  checkFilesAfterUpdateSuccess();

  logTestInfo("testing tobedeleted directory doesn't exist");
  let toBeDeletedDir = applyToDir.clone();
  toBeDeletedDir.append("tobedeleted");
  do_check_false(toBeDeletedDir.exists());

  checkCallbackAppLog();
}
