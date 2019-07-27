






function run_test() {
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

  runUpdate(1, STATE_FAILED_LOADSOURCE_ERROR_WRONG_SIZE);
}





function checkUpdateApplied() {
  if (IS_WIN || IS_MACOSX) {
    
    do_check_false(getPostUpdateFile(".running").exists());
  }

  checkFilesAfterUpdateFailure(getApplyDirFile, true, false);
  checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  standardInit();
  waitForFilesInUse();
}
