





































const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE = "general.useragent.locale";

const ADDON = "test_bug257155";
const ID = "bug257155@tests.mozilla.org";

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");
  gPrefs.setBoolPref(PREF_MATCH_OS_LOCALE, false);
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "en-US");

  
  startupEM();
  gEM.installItemFromFile(do_get_addon(ADDON), NS_INSTALL_LOCATION_APPPROFILE);
  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "en-US Name");
  restartEM();

  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "en-US Name");
  do_check_eq(getManifestProperty(ID, "description"), "en-US Description");
  
  
  gEM.disableItem(ID);
  restartEM();
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "en-US Name");
  
  
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "de-DE");
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "de-DE Name");
  do_check_eq(getManifestProperty(ID, "description"), "");
  
  
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "nl-NL");
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "Fallback Name");
  
  shutdownEM();
}
