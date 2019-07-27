






function run_test() {
  
  DEBUG_AUS_TEST = true;

  if (!shouldRunServiceTest()) {
    return;
  }

  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestFiles[11].originalFile = "partial.png";
  gTestDirs = gTestDirsPartialSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_PARTIAL_MAR);

  createUpdaterINI(true);

  
  
  
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
  if (IS_WIN || IS_MACOSX) {
    let running = getPostUpdateFile(".running");
    debugDump("checking that the post update process running file doesn't " +
              "exist. Path: " + running.path);
    do_check_false(running.exists());
  }

  checkFilesAfterUpdateFailure(getApplyDirFile, true, false);
  checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  waitForFilesInUse();
}
