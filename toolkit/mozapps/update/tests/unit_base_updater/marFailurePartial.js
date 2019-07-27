






function run_test() {
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

  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_LOADSOURCE_ERROR_WRONG_SIZE);
}





function checkUpdateApplied() {
  if (IS_WIN || IS_MACOSX) {
    
    do_check_false(getPostUpdateFile(".running").exists());
  }

  if (IS_MACOSX) {
    debugDump("testing last modified time on the apply to directory has " +
              "changed after a successful update (bug 600098)");
    let now = Date.now();
    let applyToDir = getApplyDirFile();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAC_MAX_TIME_DIFFERENCE);
  }

  checkFilesAfterUpdateFailure(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  standardInit();
  checkCallbackAppLog();
}
