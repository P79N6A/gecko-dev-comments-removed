




































function test() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/browser/browser/components/preferences/tests/privacypane_tests.js", this);

  run_test_subset([
    test_locbar_suggestion_retention(-1, undefined),
    test_locbar_suggestion_retention(1, -1),
    test_locbar_suggestion_retention(2, 1),
    test_locbar_suggestion_retention(0, 2),
    test_locbar_suggestion_retention(0, 0),

    
    reset_preferences
  ]);
}
