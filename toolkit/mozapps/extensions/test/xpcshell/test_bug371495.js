





































const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE = "general.useragent.locale";

const ADDON = "test_bug371495";
const ID = "bug371495@tests.mozilla.org";

function run_test()
{
  
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");

  
  startupManager();
  installAllFiles([do_get_addon(ADDON)], function() {
    AddonManager.getAddon(ID, function(addon) {
      do_check_neq(addon, null);
      do_check_eq(addon.name, "Test theme");
      restartManager();

      AddonManager.getAddon(ID, function(addon) {
        do_check_neq(addon, null);
        do_check_eq(addon.optionsURL, null);
        do_check_eq(addon.aboutURL, null);

        do_test_finished();
      });
    });
  });
}
