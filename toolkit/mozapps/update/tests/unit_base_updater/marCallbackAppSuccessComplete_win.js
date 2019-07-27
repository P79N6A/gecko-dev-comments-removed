





function run_test() {
  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setupUpdaterTest(FILE_COMPLETE_MAR);

  gCallbackBinFile = "exe0.exe";

  runUpdate(0, STATE_SUCCEEDED, checkUpdateFinished);
}

function checkUpdateFinished() {
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  checkCallbackAppLog();
}
