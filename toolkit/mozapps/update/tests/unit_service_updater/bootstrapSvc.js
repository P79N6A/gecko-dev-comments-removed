





function run_test() {
  if (!shouldRunServiceTest(true)) {
    return;
  }

  setupTestCommon();
  
  
  gTestFiles = gTestFilesCommon;
  gTestDirs = [];
  setupUpdaterTest(FILE_COMPLETE_MAR, false, false);

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_SUCCEEDED, false);
}

function checkUpdateFinished() {
  checkFilesAfterUpdateSuccess();

  
  
  
  checkCallbackServiceLog();
}
