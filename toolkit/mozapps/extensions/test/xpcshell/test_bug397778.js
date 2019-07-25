





































const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE = "general.useragent.locale";

const ADDON = "test_bug397778";
const ID = "bug397778@tests.mozilla.org";

function run_test()
{
  
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");
  Services.prefs.setBoolPref(PREF_MATCH_OS_LOCALE, false);
  Services.prefs.setCharPref(PREF_SELECTED_LOCALE, "fr-FR");

  
  startupManager();
  installAllFiles([do_get_addon(ADDON)], function() {
    restartManager();

    run_test_1();
  });
}

function run_test_1() {
  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    do_check_eq(addon.name, "fr Name");
    do_check_eq(addon.description, "fr Description");

    
    addon.userDisabled = true;
    restartManager();

    AddonManager.getAddonByID(ID, function(newAddon) {
      do_check_neq(newAddon, null);
      do_check_eq(newAddon.name, "fr Name");

      run_test_2();
    });
  });
}

function run_test_2() {
  
  Services.prefs.setCharPref(PREF_SELECTED_LOCALE, "de");
  restartManager();

  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    do_check_eq(addon.name, "de-DE Name");
    do_check_eq(addon.description, null);

    run_test_3();
  });
}

function run_test_3() {
  
  Services.prefs.setCharPref(PREF_SELECTED_LOCALE, "DE-de");
  restartManager();

  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    do_check_eq(addon.name, "de-DE Name");
    do_check_eq(addon.description, null);

    run_test_4();
  });
}

function run_test_4() {
  
  Services.prefs.setCharPref(PREF_SELECTED_LOCALE, "es-AR");
  restartManager();

  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    do_check_eq(addon.name, "es-ES Name");
    do_check_eq(addon.description, "es-ES Description");

    run_test_5();
  });
}

function run_test_5() {
  
  Services.prefs.setCharPref(PREF_SELECTED_LOCALE, "zh");
  restartManager();

  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    if (addon.name != "zh-TW Name" && addon.name != "zh-CN Name")
      do_throw("zh matched to " + addon.name);

    run_test_6();
  });
}

function run_test_6() {
  
  
  Services.prefs.setCharPref(PREF_SELECTED_LOCALE, "nl-NL");
  restartManager();

  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    do_check_eq(addon.name, "en Name");
    do_check_eq(addon.description, "en Description");

    do_test_finished();
  });
}
