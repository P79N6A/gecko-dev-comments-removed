






function run_test() {
  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestFiles[gTestFiles.length - 1].originalContents = null;
  gTestFiles[gTestFiles.length - 1].compareContents = "FromPartial\n";
  gTestFiles[gTestFiles.length - 1].comparePerms = 0o644;
  gTestFiles[gTestFiles.length - 2].originalContents = null;
  gTestFiles[gTestFiles.length - 2].compareContents = "FromPartial\n";
  gTestFiles[gTestFiles.length - 2].comparePerms = 0o644;
  gTestDirs = gTestDirsPartialSuccess;
  setupUpdaterTest(FILE_PARTIAL_MAR);
  if (IS_MACOSX) {
    
    
    
    let testFile = getApplyDirFile(DIR_MACOS + "distribution/testFile", true);
    writeFile(testFile, "test\n");
    testFile = getApplyDirFile(DIR_MACOS + "distribution/test/testFile", true);
    writeFile(testFile, "test\n");
  }

  createUpdaterINI(true);

  
  
  
  if (IS_MACOSX) {
    let now = Date.now();
    let yesterday = now - (1000 * 60 * 60 * 24);
    let applyToDir = getApplyDirFile();
    applyToDir.lastModifiedTime = yesterday;
  }

  runUpdate(0, STATE_SUCCEEDED);
}





function checkUpdateApplied() {
  if (IS_WIN || IS_MACOSX) {
    gCheckFunc = finishCheckUpdateApplied;
    checkPostUpdateAppLog();
  } else {
    finishCheckUpdateApplied();
  }
}





function finishCheckUpdateApplied() {
  if (IS_MACOSX) {
    debugDump("testing last modified time on the apply to directory has " +
              "changed after a successful update (bug 600098)");
    let now = Date.now();
    let applyToDir = getApplyDirFile();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAC_MAX_TIME_DIFFERENCE);
  }

  if (IS_MACOSX) {
    debugDump("testing that the distribution directory is removed from the " +
              "old location when there is a distribution directory in the " +
              "new location");
    let distributionDir = getApplyDirFile(DIR_MACOS + "distribution", true);
    debugDump("testing " + distributionDir.path + " shouldn't exist");
    do_check_false(distributionDir.exists());

    checkUpdateLogContains("removing old distribution directory");
  }

  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_SUCCESS);
  checkCallbackAppLog();
}
