






function run_test() {
  if (!IS_MAR_CHECKS_ENABLED) {
    return;
  }

  setupTestCommon();
  
  
  gTestFiles = gTestFilesCommon;
  gTestDirs = [];
  setupUpdaterTest(FILE_OLD_VERSION_MAR);

  createUpdaterINI(true);

  
  
  
  
  runUpdate((USE_EXECV ? 0 : 1), STATE_FAILED_VERSION_DOWNGRADE_ERROR);
}





function checkUpdateApplied() {
  if (IS_WIN || IS_MACOSX) {
    
    do_check_false(getPostUpdateFile(".running").exists());
  }

  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  standardInit();
  doTestFinish();
}
