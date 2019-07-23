





































function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  const dataDir = do_get_file("data");

  startupEM();

  gEM.installItemFromFile(do_get_file("data/test_bug526598_1.xpi"),
                          NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_file("data/test_bug526598_2.xpi"),
                          NS_INSTALL_LOCATION_APPPROFILE);

  restartEM();
  do_check_neq(gEM.getItemForID("bug526598_1@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("bug526598_2@tests.mozilla.org"), null);

  var il = gEM.getInstallLocation("bug526598_1@tests.mozilla.org");
  var file = il.getItemFile("bug526598_1@tests.mozilla.org", "install.rdf");
  do_check_true(file.exists());
  do_check_true(file.isReadable());
  do_check_true(file.isWritable());

  il = gEM.getInstallLocation("bug526598_2@tests.mozilla.org");
  file = il.getItemFile("bug526598_2@tests.mozilla.org", "install.rdf");
  do_check_true(file.exists());
  do_check_true(file.isReadable());
  do_check_true(file.isWritable());

  gEM.uninstallItem("bug526598_1@tests.mozilla.org");
  gEM.uninstallItem("bug526598_2@tests.mozilla.org");

  restartEM();
  do_check_eq(gEM.getItemForID("bug526598_1@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug526598_2@tests.mozilla.org"), null);
}
