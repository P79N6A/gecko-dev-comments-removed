





const TEST_FILES = [
{
  description      : "the dummy file to make sure that the update worked",
  fileName         : "dummy",
  relPathDir       : "/",
  originalContents : null,
  compareContents  : "",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}
];

function run_test() {
  if (!shouldRunServiceTest(true)) {
    return;
  }

  setupTestCommon();
  setupUpdaterTest(FILE_COMPLETE_MAR);

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  runUpdateUsingService(STATE_PENDING_SVC, STATE_SUCCEEDED, false);
}

function checkUpdateFinished() {
  checkFilesAfterUpdateSuccess();

  
  
  
  checkCallbackServiceLog();
}
