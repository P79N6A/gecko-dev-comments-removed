





function run_test() {
  Services.prefs.setBoolPref("extensions.defaultProviders.enabled", false);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
  startupManager();
  do_check_false(AddonManager.isInstallEnabled("application/x-xpinstall"));
  Services.prefs.clearUserPref("extensions.defaultProviders.enabled");
}
