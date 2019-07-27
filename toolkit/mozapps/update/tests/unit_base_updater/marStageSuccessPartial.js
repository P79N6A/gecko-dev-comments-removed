






function run_test() {
  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestFiles[gTestFiles.length - 2].originalContents = null;
  gTestFiles[gTestFiles.length - 2].compareContents = "FromPartial\n";
  gTestFiles[gTestFiles.length - 2].comparePerms = 0o644;
  gTestDirs = gTestDirsPartialSuccess;
  preventDistributionFiles();
  setupUpdaterTest(FILE_PARTIAL_MAR);
  if (IS_MACOSX) {
    
    
    let testFile = getApplyDirFile(DIR_MACOS + "distribution/testFile", true);
    writeFile(testFile, "test\n");
    testFile = getApplyDirFile(DIR_MACOS + "distribution/test/testFile", true);
    writeFile(testFile, "test\n");
  }

  createUpdaterINI(false);
  setAppBundleModTime();

  runUpdate(0, STATE_APPLIED, null);

  checkFilesAfterUpdateSuccess(getStageDirFile, true, false);
  checkUpdateLogContents(LOG_PARTIAL_SUCCESS, true);
  checkPostUpdateRunningFile(false);

  
  gStageUpdate = false;
  gSwitchApp = true;
  do_timeout(TEST_CHECK_TIMEOUT, function() {
    runUpdate(0, STATE_SUCCEEDED, checkUpdateApplied);
  });
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
  checkPostUpdateRunningFile(true);

  let distributionDir = getApplyDirFile(DIR_RESOURCES + "distribution", true);
  if (IS_MACOSX) {
    Assert.ok(distributionDir.exists(), MSG_SHOULD_EXIST);

    let testFile = getApplyDirFile(DIR_RESOURCES + "distribution/testFile", true);
    Assert.ok(testFile.exists(), MSG_SHOULD_EXIST);

    testFile = getApplyDirFile(DIR_RESOURCES + "distribution/test/testFile", true);
    Assert.ok(testFile.exists(), MSG_SHOULD_EXIST);

    distributionDir = getApplyDirFile(DIR_MACOS + "distribution", true);
    Assert.ok(!distributionDir.exists(), MSG_SHOULD_NOT_EXIST);

    checkUpdateLogContains("Moving old distribution directory to new location");
  } else {
    debugDump("testing that files aren't added with an add-if instruction " +
              "when the file's destination directory doesn't exist");
    Assert.ok(!distributionDir.exists(), MSG_SHOULD_NOT_EXIST);
  }

  checkAppBundleModTime();
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_SUCCESS, true);
  standardInit();
  checkCallbackAppLog();
}
