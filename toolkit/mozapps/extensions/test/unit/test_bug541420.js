






































function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  startupEM();

  gEM.installItemFromFile(do_get_file("data/test_bug541420.xpi"),
                          NS_INSTALL_LOCATION_APPPROFILE);

  restartEM();
  do_check_neq(gEM.getItemForID("bug541420@tests.mozilla.org"), null);

  var il = gEM.getInstallLocation("bug541420@tests.mozilla.org");
  var file = il.getItemFile("bug541420@tests.mozilla.org", "binary");
  do_check_true(file.exists());
  do_check_true(file.isReadable());
  do_check_true(file.isWritable());

  
  
  
  
  if (!("nsIWindowsRegKey" in Components.interfaces))
    do_check_true((file.permissions & 0100) == 0100);

  gEM.uninstallItem("bug541420@tests.mozilla.org");

  restartEM();
  do_check_eq(gEM.getItemForID("bug541420@tests.mozilla.org"), null);
}
