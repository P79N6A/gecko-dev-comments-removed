





function run_test() {
  
  
  Components.utils.unload("resource://gre/modules/AddonManager.jsm");
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
  gAppInfo.processType = AM_Ci.nsIXULRuntime.PROCESS_TYPE_CONTENT;
  try {
    Components.utils.import("resource://gre/modules/AddonManager.jsm");
    do_throw("AddonManager should have refused to load");
  }
  catch (ex) {
    do_print(ex.message);
    do_check_true(!!ex.message);
  }
}
