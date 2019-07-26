





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

  setupTestCommon(false);
  do_register_cleanup(cleanupUpdaterTest);

  setupUpdaterTest(FILE_COMPLETE_MAR);

  
  runUpdateUsingService(STATE_PENDING_SVC, STATE_SUCCEEDED, checkUpdateApplied, null, false);
}

function checkUpdateApplied() {
  checkFilesAfterUpdateSuccess();

  
  
  
  checkCallbackServiceLog();
}
