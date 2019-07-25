







































const TEST_ID = "0112";


const TEST_FILES = [
{
  fileName         : "00png0.png",
  relPathDir       : "0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : 0644,
  comparePerms     : null
}, {
  fileName         : "00text0",
  relPathDir       : "0/00/",
  originalContents : "ShouldNotBeDeleted\n",
  compareContents  : "ShouldNotBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0644,
  comparePerms     : null
}, {
  fileName         : "00text1",
  relPathDir       : "0/00/",
  originalContents : "ShouldNotBeDeleted\n",
  compareContents  : "ShouldNotBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0644,
  comparePerms     : null
}, {
  fileName         : "0exe0.exe",
  relPathDir       : "0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/partial.png",
  compareFile      : "data/partial.png",
  originalPerms    : 0755,
  comparePerms     : null
}, {
  fileName         : "10text0",
  relPathDir       : "1/10/",
  originalContents : "ShouldNotBeDeleted\n",
  compareContents  : "ShouldNotBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0644,
  comparePerms     : null
}, {
  fileName         : "exe0.exe",
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/partial.png",
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

  
  
  
  let lastModTime;
  if (IS_MACOSX) {
    
    
    let now = Date.now();
    lastModTime = now - (1000 * 60 * 60 * 24);
    applyToDir.lastModifiedTime = lastModTime;
    
    
    lastModTime = applyToDir.lastModifiedTime;
  }

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for success when " +
              "applying a partial mar");
  do_check_eq(exitValue, 0);

  logTestInfo("testing update.status should be " + STATE_FAILED);
  
  
  do_check_eq(readStatusFile(updatesDir).split(": ")[0], STATE_FAILED);

  
  
  if (IS_MACOSX) {
    logTestInfo("testing last modified time on the apply to directory has " +
                "not changed after a failed update (bug 600098)");
    do_check_eq(applyToDir.lastModifiedTime, lastModTime);
  }

  checkFilesAfterUpdateFailure();

  logTestInfo("testing tobedeleted directory doesn't exist");
  let toBeDeletedDir = applyToDir.clone();
  toBeDeletedDir.append("tobedeleted");
  do_check_false(toBeDeletedDir.exists());

  checkCallbackAppLog();
}
