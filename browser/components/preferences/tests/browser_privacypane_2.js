




































function test() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/browser/browser/components/preferences/tests/privacypane_tests.js", this);

  run_test_subset([
    test_historymode_retention("remember", undefined),
    test_historymode_retention("dontremember", "remember"),
    test_historymode_retention("custom", "dontremember"),
    
    test_historymode_retention("remember", "dontremember"),
    test_historymode_retention("custom", "remember"),
    
    test_historymode_retention("remember", "remember"),

    
    reset_preferences
  ]);
}
