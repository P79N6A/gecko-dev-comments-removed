





































const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE = "general.useragent.locale";

const ADDON = "test_bug397778";
const ID = "bug397778@tests.mozilla.org";

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");
  gPrefs.setBoolPref(PREF_MATCH_OS_LOCALE, false);
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "fr-FR");

  
  startupEM();
  gEM.installItemFromFile(do_get_addon(ADDON), NS_INSTALL_LOCATION_APPPROFILE);
  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "fr Name");
  restartEM();

  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "fr Name");
  do_check_eq(getManifestProperty(ID, "description"), "fr Description");
  
  
  gEM.disableItem(ID);
  restartEM();
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "fr Name");
  
  
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "de");
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "de-DE Name");
  do_check_eq(getManifestProperty(ID, "description"), "");
  
  
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "DE-de");
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "de-DE Name");
  do_check_eq(getManifestProperty(ID, "description"), "");
  
  
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "es-AR");
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "es-ES Name");
  do_check_eq(getManifestProperty(ID, "description"), "es-ES Description");
  
  
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "zh");
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  if (addon.name != "zh-TW Name" && addon.name != "zh-CN Name")
    do_throw("zh matched to " + addon.name);
  
  
  
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "nl-NL");
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "en Name");
  do_check_eq(getManifestProperty(ID, "description"), "en Description");
  
  shutdownEM();
}
