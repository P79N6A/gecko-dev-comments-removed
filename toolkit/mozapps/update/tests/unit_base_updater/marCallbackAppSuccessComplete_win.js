





function run_test() {
  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setupUpdaterTest(FILE_COMPLETE_MAR, false, false);

  gCallbackBinFile = "exe0.exe";

  runUpdate(0, STATE_SUCCEEDED);
}

function checkUpdateApplied() {
  checkFilesAfterUpdateSuccess();
  checkCallbackAppLog();
}
