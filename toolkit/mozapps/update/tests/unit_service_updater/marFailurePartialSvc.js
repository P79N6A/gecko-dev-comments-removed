






function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestFiles[11].originalFile = "partial.png";
  gTestDirs = gTestDirsPartialSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_PARTIAL_MAR);

  createUpdaterINI();

  
  
  
  if (IS_MACOSX) {
    let now = Date.now();
    let yesterday = now - (1000 * 60 * 60 * 24);
    let applyToDir = getApplyDirFile();
    applyToDir.lastModifiedTime = yesterday;
  }

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_FAILED);
}





function checkUpdateFinished() {
  if (IS_MACOSX) {
    debugDump("testing last modified time on the apply to directory has " +
              "changed after a successful update (bug 600098)");
    let now = Date.now();
    let applyToDir = getApplyDirFile();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAC_MAX_TIME_DIFFERENCE);
  }

  if (IS_WIN || IS_MACOSX) {
    let running = getPostUpdateFile(".running");
    debugDump("checking that the post update process running file doesn't " +
              "exist. Path: " + running.path);
    do_check_false(running.exists());
  }

  checkFilesAfterUpdateFailure(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  checkCallbackServiceLog();
}
