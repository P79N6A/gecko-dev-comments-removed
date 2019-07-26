






function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  
  
  gTestFiles = gTestFilesCommon;
  gTestDirs = [];
  setupUpdaterTest(FILE_OLD_VERSION_MAR, false, false);

  createUpdaterINI(true);

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_VERSION_DOWNGRADE_ERROR);
}





function checkUpdateApplied() {
  if (IS_MACOSX || IS_WIN) {
    
    do_check_false(getPostUpdateFile(".running").exists());
  }

  checkFilesAfterUpdateSuccess();
  doTestFinish();
}
