



function run_test() {
    let loadContext = { get usePrivateBrowsing() { return gInPrivateBrowsing; } };

    ContentPrefTest.deleteDatabase();
    var cp = new ContentPrefInstance(loadContext);
    do_check_neq(cp, null, "Retrieving the content prefs service failed");

    try {
      const uri1 = ContentPrefTest.getURI("http://www.example.com/");
      const uri2 = ContentPrefTest.getURI("http://www.anotherexample.com/");
      const pref_name = "browser.content.full-zoom";
      const zoomA = 1.5, zoomA_new = 0.8, zoomB = 1.3;
      
      cp.setPref(uri1, pref_name, zoomA);
      
      do_check_eq(cp.getPref(uri1, pref_name), zoomA);
      
      enterPBMode();
      
      do_check_eq(cp.getPref(uri1, pref_name), zoomA);
      
      cp.setPref(uri2, pref_name, zoomB);
      
      do_check_eq(cp.getPref(uri2, pref_name), zoomB);
      
      cp.setPref(uri1, pref_name, zoomA_new);
      
      do_check_eq(cp.getPref(uri1, pref_name), zoomA_new);
      
      exitPBMode();
      
      do_check_eq(cp.getPref(uri1, pref_name), zoomA);
      
      do_check_eq(cp.hasPref(uri2, pref_name), false);
    } catch (e) {
      do_throw("Unexpected exception: " + e);
    }
}
