





function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  gStageUpdate = true;
  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setupUpdaterTest(FILE_COMPLETE_MAR);

  gCallbackBinFile = "exe0.exe";

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_APPLIED);
}

function checkUpdateFinished() {
  
  gStageUpdate = false;
  gSwitchApp = true;
  runUpdate(0, STATE_SUCCEEDED, checkUpdateApplied);
}

function checkUpdateApplied() {
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  checkCallbackAppLog();
}
