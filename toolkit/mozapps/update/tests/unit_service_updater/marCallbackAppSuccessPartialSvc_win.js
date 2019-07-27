





function run_test() {
  if (!shouldRunServiceTest()) {
    return;
  }

  setupTestCommon();
  gTestFiles = gTestFilesPartialSuccess;
  gTestDirs = gTestDirsPartialSuccess;
  setupUpdaterTest(FILE_PARTIAL_MAR);

  gCallbackBinFile = "exe0.exe";

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_SUCCEEDED);
}

function checkUpdateFinished() {
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  checkCallbackServiceLog();
}
