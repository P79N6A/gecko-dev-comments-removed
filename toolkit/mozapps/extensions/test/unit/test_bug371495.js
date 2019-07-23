





































const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE = "general.useragent.locale";

const ADDON = "test_bug371495";
const ID = "bug371495@tests.mozilla.org";

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");

  
  startupEM();
  gEM.installItemFromFile(do_get_addon(ADDON), NS_INSTALL_LOCATION_APPPROFILE);
  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "Test theme");
  restartEM();

  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(getManifestProperty(ID, "optionsURL"), "");
  do_check_eq(getManifestProperty(ID, "aboutURL"), "");

  shutdownEM();
}
