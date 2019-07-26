




function run_test() {
  setupTestCommon(true);

  
  logTestInfo("testing write access to the application directory");
  var testFile = getCurrentProcessDir();
  testFile.append("update_write_access_test");
  testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, 0o644);
  do_check_true(testFile.exists());
  testFile.remove(false);
  do_check_false(testFile.exists());

  standardInit();

  
  logTestInfo("testing nsIApplicationUpdateService:canCheckForUpdates");
  do_check_true(gAUS.canCheckForUpdates);
  
  logTestInfo("testing nsIApplicationUpdateService:canApplyUpdates");
  do_check_true(gAUS.canApplyUpdates);
  do_test_finished();
}

function end_test() {
  cleanupTestCommon();
}
