






function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  preventDistributionFiles();
  setupUpdaterTest(FILE_COMPLETE_MAR);
  if (IS_MACOSX) {
    
    
    let testFile = getApplyDirFile(DIR_MACOS + "distribution/testFile", true);
    writeFile(testFile, "test\n");
    testFile = getApplyDirFile(DIR_MACOS + "distribution/test/testFile", true);
    writeFile(testFile, "test\n");
  }

  createUpdaterINI();
  setAppBundleModTime();

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_SUCCEEDED);
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
  checkUpdateLogContents(LOG_COMPLETE_SUCCESS, true);
  standardInit();
  checkCallbackServiceLog();
}
