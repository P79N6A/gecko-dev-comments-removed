





































const ADDON = "test_bug521905";
const ID = "bug521905@tests.mozilla.org";


gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);

gPrefs.setBoolPref("extensions.checkCompatibility.2.0pre", false);

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2.0pre", "2");

  startupEM();
  gEM.installItemFromFile(do_get_addon(ADDON), NS_INSTALL_LOCATION_APPPROFILE);

  restartEM();
  do_check_neq(gEM.getItemForID(ID), null);
  do_check_eq(getManifestProperty(ID, "isDisabled"), "false");

  gPrefs.setBoolPref("extensions.checkCompatibility.2.0pre", true);

  restartEM();
  do_check_neq(gEM.getItemForID(ID), null);
  do_check_eq(getManifestProperty(ID, "isDisabled"), "true");

  gPrefs.setBoolPref("extensions.checkCompatibility.2.0p", false);

  restartEM();
  do_check_neq(gEM.getItemForID(ID), null);
  do_check_eq(getManifestProperty(ID, "isDisabled"), "true");
}
