







































function run_test() {
  
  dump("Testing: write access is required to the application directory\n");
  removeUpdateDirsAndFiles();
  var testFile = getCurrentProcessDir();
  testFile.append("update_write_access_test");
  testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, 0644);
  do_check_true(testFile.exists());
  testFile.remove(false);
  do_check_false(testFile.exists());

  standardInit();

  
  dump("Testing: nsIApplicationUpdateService:canCheckForUpdates\n");
  do_check_true(gAUS.canCheckForUpdates);
  
  dump("Testing: nsIApplicationUpdateService:canApplyUpdates\n");
  do_check_true(gAUS.canApplyUpdates);
  cleanUp();
}
