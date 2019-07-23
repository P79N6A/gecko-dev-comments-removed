





































const ADDON = "test_bug425657";
const ID = "bug425657@tests.mozilla.org";

function run_test()
{
  
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");

  
  startupManager();
  installAllFiles([do_get_addon(ADDON)], function() {
    restartManager();
    AddonManager.getAddon(ID, function(addon) {
      do_check_neq(addon, null);
      do_check_eq(addon.name, "Deutsches W\u00f6rterbuch");
      do_check_eq(addon.name.length, 20);

      do_test_finished();
    });
  });
}
