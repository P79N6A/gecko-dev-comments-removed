







































function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);

  
  logTestInfo("testing write access to the application directory");
  removeUpdateDirsAndFiles();
  var testFile = getCurrentProcessDir();
  testFile.append("update_write_access_test");
  testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, 0644);
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
  cleanUp();
}
