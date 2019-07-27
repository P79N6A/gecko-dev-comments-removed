





function run_test() {
  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestDirs = gTestDirsPartialSuccess;
  setupUpdaterTest(FILE_PARTIAL_MAR);

  gCallbackBinFile = "exe0.exe";

  runUpdate(0, STATE_SUCCEEDED, checkUpdateFinished);
}

function checkUpdateFinished() {
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  checkCallbackAppLog();
}
