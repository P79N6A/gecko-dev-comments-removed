





function run_test() {
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  startupManager();

  
  let old = AddonManager.STATE_AVAILABLE;
  AddonManager.STATE_AVAILABLE = 28;
  do_check_eq(AddonManager.STATE_AVAILABLE, old);

  
  AddonManager.isInstallEnabled = function() {
    do_throw("Should not be able to replace a function");
  }
  AddonManager.isInstallEnabled("application/x-xpinstall");

  
  AddonManager.foo = "bar";
  do_check_false("foo" in AddonManager);
}
