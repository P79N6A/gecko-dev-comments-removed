






function run_test() {
  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_COMPLETE_MAR);

  let fileInUseBin = getApplyDirFile(gTestDirs[4].relPathDir +
                                     gTestDirs[4].subDirs[0] +
                                     gTestDirs[4].subDirFiles[0]);
  
  
  fileInUseBin.remove(false);

  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let fileInUseDir = getApplyDirFile(gTestDirs[4].relPathDir +
                                     gTestDirs[4].subDirs[0]);
  helperBin.copyTo(fileInUseDir, gTestDirs[4].subDirFiles[0]);

  
  let args = [getApplyDirPath() + DIR_RESOURCES, "input", "output", "-s",
              HELPER_SLEEP_TIMEOUT];
  let fileInUseProcess = Cc["@mozilla.org/process/util;1"].
                         createInstance(Ci.nsIProcess);
  fileInUseProcess.init(fileInUseBin);
  fileInUseProcess.run(false, args, args.length);

  do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
}

function doUpdate() {
  runUpdate(0, STATE_APPLIED, null);

  
  gStageUpdate = false;
  gSwitchApp = true;
  runUpdate(1, STATE_PENDING, checkUpdateApplied);
}

function checkUpdateApplied() {
  setupHelperFinish();
}

function checkUpdate() {
  checkFilesAfterUpdateFailure(getApplyDirFile, false, false);
  checkUpdateLogContains(ERR_RENAME_FILE);
  standardInit();
  checkCallbackAppLog();
}
