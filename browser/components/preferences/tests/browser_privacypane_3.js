




































function test() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/browser/browser/components/preferences/tests/privacypane_tests.js", this);

  run_test_subset([
    test_custom_retention("rememberHistoryDays", "remember"),
    test_custom_retention("rememberHistoryDays", "custom"),
    test_custom_retention("historyDays", "remember", 1),
    test_custom_retention("historyDays", "custom", -1),
    test_custom_retention("rememberDownloads", "remember"),
    test_custom_retention("rememberDownloads", "custom"),
    test_custom_retention("rememberForms", "remember"),
    test_custom_retention("rememberForms", "custom"),
    test_historymode_retention("remember", "remember"),

    
    reset_preferences
  ]);
}
