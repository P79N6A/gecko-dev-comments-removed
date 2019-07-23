




































function test() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/browser/browser/components/preferences/tests/privacypane_tests.js", this);

  run_test_subset([
    test_custom_retention("acceptCookies", "remember"),
    test_custom_retention("acceptCookies", "custom"),
    test_custom_retention("acceptThirdParty", "remember"),
    test_custom_retention("acceptThirdParty", "custom"),
    test_custom_retention("keepCookiesUntil", "remember", 1),
    test_custom_retention("keepCookiesUntil", "custom", 2),
    test_custom_retention("keepCookiesUntil", "custom", 0),
    test_custom_retention("alwaysClear", "remember"),
    test_custom_retention("alwaysClear", "custom"),
    test_historymode_retention("remember", "remember"),

    
    reset_preferences
  ]);
}
