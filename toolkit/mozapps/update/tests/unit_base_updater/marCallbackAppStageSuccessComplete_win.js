





function run_test() {
  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setupUpdaterTest(FILE_COMPLETE_MAR);

  gCallbackBinFile = "exe0.exe";

  runUpdate(0, STATE_APPLIED, null);

  
  gStageUpdate = false;
  gSwitchApp = true;
  runUpdate(0, STATE_SUCCEEDED, checkUpdateApplied);
}

function checkUpdateApplied() {
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  checkCallbackAppLog();
}
