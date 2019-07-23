





































function run_test()
{
  
  var extension = gProfD.clone()
  extension.append("extensions");
  extension.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  extension.append("bug424107@tests.mozilla.org");
  extension.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  var sourcerdf = do_get_file("data/test_bug424107_1.rdf");
  sourcerdf.copyTo(extension, "install.rdf");
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "5", "1.9");
  startupEM();
  var addon = gEM.getItemForID("bug424107@tests.mozilla.org");
  do_check_neq(addon, null);
  do_check_eq(addon.version, 1);

  
  extension.remove(true);

  restartEM();
  addon = gEM.getItemForID("bug424107@tests.mozilla.org");
  do_check_eq(addon, null);

  
  extension.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  sourcerdf = do_get_file("data/test_bug424107_2.rdf");
  sourcerdf.copyTo(extension, "install.rdf");

  restartEM();
  addon = gEM.getItemForID("bug424107@tests.mozilla.org");
  do_check_neq(addon, null);
  do_check_eq(addon.version, 2);
}

