





function run_test() {
  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestDirs = gTestDirsPartialSuccess;
  setupUpdaterTest(FILE_PARTIAL_MAR, false, false);

  gCallbackBinFile = "exe0.exe";

  runUpdate(0, STATE_APPLIED, null);

  
  gStageUpdate = false;
  gSwitchApp = true;
  runUpdate(0, STATE_SUCCEEDED);
}

function checkUpdateApplied() {
  checkFilesAfterUpdateSuccess();
  checkCallbackAppLog();
}
