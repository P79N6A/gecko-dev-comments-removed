






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

  
  
  
  if (IS_MACOSX) {
    let now = Date.now();
    let yesterday = now - (1000 * 60 * 60 * 24);
    let applyToDir = getApplyDirFile();
    applyToDir.lastModifiedTime = yesterday;
  }

  runUpdate(0, STATE_APPLIED, null);

  checkFilesAfterUpdateSuccess(getStageDirFile, true, false);
  checkUpdateLogContents(LOG_PARTIAL_SUCCESS, true);

  if (IS_WIN || IS_MACOSX) {
    
    
    do_check_false(getPostUpdateFile(".running").exists());
  }

  
  gStageUpdate = false;
  gSwitchApp = true;
  do_timeout(TEST_CHECK_TIMEOUT, function() {
    runUpdate(0, STATE_SUCCEEDED);
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
  if (IS_MACOSX) {
    debugDump("testing last modified time on the apply to directory has " +
              "changed after a successful update (bug 600098)");
    let now = Date.now();
    let applyToDir = getApplyDirFile();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAC_MAX_TIME_DIFFERENCE);
  }

  let distributionDir = getApplyDirFile(DIR_RESOURCES + "distribution", true);
  if (IS_MACOSX) {
    debugDump("testing that the distribution directory is moved from the " +
              "old location to the new location");
    debugDump("testing " + distributionDir.path + " should exist");
    do_check_true(distributionDir.exists());

    let testFile = getApplyDirFile(DIR_RESOURCES + "distribution/testFile", true);
    debugDump("testing " + testFile.path + " should exist");
    do_check_true(testFile.exists());

    testFile = getApplyDirFile(DIR_RESOURCES + "distribution/test/testFile", true);
    debugDump("testing " + testFile.path + " should exist");
    do_check_true(testFile.exists());

    distributionDir = getApplyDirFile(DIR_MACOS + "distribution", true);
    debugDump("testing " + distributionDir.path + " shouldn't exist");
    do_check_false(distributionDir.exists());

    checkUpdateLogContains("Moving old distribution directory to new location");
  } else {
    debugDump("testing that files aren't added with an add-if instruction " +
              "when the file's destination directory doesn't exist");
    debugDump("testing " + distributionDir.path + " shouldn't exist");
    do_check_false(distributionDir.exists());
  }

  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  checkUpdateLogContents(LOG_PARTIAL_SUCCESS, true);
  standardInit();
  checkCallbackAppLog();
}
