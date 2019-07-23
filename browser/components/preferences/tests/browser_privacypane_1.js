




































function test() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/browser/browser/components/preferences/tests/privacypane_tests.js", this);

  run_test_subset([
    test_locbar_emptyText,
    test_pane_visibility,
    test_dependent_elements,
    test_dependent_cookie_elements,
    test_dependent_clearonclose_elements,
    test_dependent_prefs,

    
    reset_preferences
  ]);
}
