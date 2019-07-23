





































function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  
  var dest = gProfD.clone();
  dest.append("extensions");
  dest.append("bug470377_1@tests.mozilla.org");
  dest.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  var source = do_get_file("data/test_bug470377/install_1.rdf");
  source.copyTo(dest, "install.rdf");
  dest = gProfD.clone();
  dest.append("extensions");
  dest.append("bug470377_2@tests.mozilla.org");
  dest.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  source = do_get_file("data/test_bug470377/install_2.rdf");
  source.copyTo(dest, "install.rdf");
  dest = gProfD.clone();
  dest.append("extensions");
  dest.append("bug470377_3@tests.mozilla.org");
  dest.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  source = do_get_file("data/test_bug470377/install_3.rdf");
  source.copyTo(dest, "install.rdf");
  dest = gProfD.clone();
  dest.append("extensions");
  dest.append("bug470377_4@tests.mozilla.org");
  dest.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  source = do_get_file("data/test_bug470377/install_4.rdf");
  source.copyTo(dest, "install.rdf");
  dest = gProfD.clone();
  dest.append("extensions");
  dest.append("bug470377_5@tests.mozilla.org");
  dest.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  source = do_get_file("data/test_bug470377/install_5.rdf");
  source.copyTo(dest, "install.rdf");

  
  gPrefs.setBoolPref("extensions.checkCompatibility", false);
  startupEM();

  do_check_neq(gEM.getItemForID("bug470377_1@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_1@tests.mozilla.org", "isDisabled"), "true");
  do_check_neq(gEM.getItemForID("bug470377_2@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_2@tests.mozilla.org", "isDisabled"), "false");
  do_check_neq(gEM.getItemForID("bug470377_3@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_3@tests.mozilla.org", "isDisabled"), "false");
  do_check_neq(gEM.getItemForID("bug470377_4@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_4@tests.mozilla.org", "isDisabled"), "false");
  do_check_neq(gEM.getItemForID("bug470377_5@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_5@tests.mozilla.org", "isDisabled"), "false");

  
  gPrefs.setBoolPref("extensions.checkCompatibility", true);
  restartEM();

  do_check_neq(gEM.getItemForID("bug470377_1@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_1@tests.mozilla.org", "isDisabled"), "true");
  do_check_neq(gEM.getItemForID("bug470377_2@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_2@tests.mozilla.org", "isDisabled"), "true");
  do_check_neq(gEM.getItemForID("bug470377_3@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_3@tests.mozilla.org", "isDisabled"), "true");
  do_check_neq(gEM.getItemForID("bug470377_4@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_4@tests.mozilla.org", "isDisabled"), "false");
  do_check_neq(gEM.getItemForID("bug470377_5@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug470377_5@tests.mozilla.org", "isDisabled"), "false");
}
