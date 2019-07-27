





function run_test() {
  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setTestFilesAndDirsForFailure();
  setupUpdaterTest(FILE_COMPLETE_MAR);

  
  let helperBin = getTestDirFile(FILE_HELPER_BIN);
  let helperDestDir = getApplyDirFile(DIR_RESOURCES);
  helperBin.copyTo(helperDestDir, FILE_HELPER_BIN);
  helperBin = getApplyDirFile(DIR_RESOURCES + FILE_HELPER_BIN);
  
  
  let lockFileRelPath = gTestFiles[3].relPathDir.split("/");
  if (IS_MACOSX) {
    lockFileRelPath = lockFileRelPath.slice(2);
  }
  lockFileRelPath = lockFileRelPath.join("/") + "/" + gTestFiles[3].fileName;
  let args = [getApplyDirPath() + DIR_RESOURCES, "input", "output", "-s",
              HELPER_SLEEP_TIMEOUT, lockFileRelPath];
  let lockFileProcess = Cc["@mozilla.org/process/util;1"].
                        createInstance(Ci.nsIProcess);
  lockFileProcess.init(helperBin);
  lockFileProcess.run(false, args, args.length);

  do_timeout(TEST_HELPER_TIMEOUT, waitForHelperSleep);
}

function doUpdate() {
  runUpdate(1, STATE_FAILED_WRITE_ERROR, checkUpdateFinished);
}

function checkUpdateFinished() {
  setupHelperFinish();
}

function checkUpdate() {
  checkFilesAfterUpdateFailure(getApplyDirFile, false, false);
  checkUpdateLogContains(ERR_RENAME_FILE);
  standardInit();
  checkCallbackAppLog();
}
