


function run_test() {
  removeMetadata();
  removeCacheFile();

  do_check_false(Services.search.isInitialized);

  let engineDummyFile = gProfD.clone();
  engineDummyFile.append("searchplugins");
  engineDummyFile.append("test-search-engine.xml");
  let engineDir = engineDummyFile.parent;
  engineDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  do_get_file("data/engine.xml").copyTo(engineDir, "engine.xml");

  do_register_cleanup(function() {
    removeMetadata();
    removeCacheFile();
  });

  
  Services.prefs.setCharPref("browser.search.geoip.url", 'data:application/json,{"country_code"');
  Services.search.init(() => {
    try {
      Services.prefs.getCharPref("browser.search.countryCode");
      ok(false, "should be no countryCode pref");
    } catch (_) {}
    try {
      Services.prefs.getCharPref("browser.search.isUS");
      ok(false, "should be no isUS pref yet either");
    } catch (_) {}
    
    Services.search.getEngines();
    equal(Services.prefs.getBoolPref("browser.search.isUS"),
          isUSTimezone(),
          "should have set isUS based on current timezone.");
    
    let histogram = Services.telemetry.getHistogramById("SEARCH_SERVICE_COUNTRY_SUCCESS");
    let snapshot = histogram.snapshot();
    equal(snapshot.sum, 0);

    
    histogram = Services.telemetry.getHistogramById("SEARCH_SERVICE_COUNTRY_SUCCESS_WITHOUT_DATA");
    snapshot = histogram.snapshot();
    equal(snapshot.sum, 1);

    do_test_finished();
    run_next_test();
  });
  do_test_pending();
}
