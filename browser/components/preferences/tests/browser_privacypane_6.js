




































function test() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/browser/browser/components/preferences/tests/privacypane_tests.js", this);

  run_test_subset([
    test_privatebrowsing_toggle,
    enter_private_browsing, 
    test_privatebrowsing_toggle,
    test_privatebrowsing_ui,
    enter_private_browsing, 
    test_privatebrowsing_ui,

    
    reset_preferences
  ]);
}
