






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
  setAppBundleModTime();

  runUpdate(0, STATE_SUCCEEDED, checkUpdateFinished);
}





function checkUpdateFinished() {
  if (IS_WIN || IS_MACOSX) {
    gCheckFunc = finishCheckUpdateFinished;
    checkPostUpdateAppLog();
  } else {
    finishCheckUpdateFinished();
  }
}





function finishCheckUpdateFinished() {
  if (IS_MACOSX) {
    let distributionDir = getApplyDirFile(DIR_MACOS + "distribution", true);
    Assert.ok(!distributionDir.exists(), MSG_SHOULD_NOT_EXIST);
    checkUpdateLogContains("removing old distribution directory");
  }

  checkAppBundleModTime();
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_SUCCESS);
  standardInit();
  checkCallbackAppLog();
}
