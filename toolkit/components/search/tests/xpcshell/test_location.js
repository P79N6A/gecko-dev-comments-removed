


function run_test() {
  installTestEngine();

  Services.prefs.setCharPref("browser.search.geoip.url", 'data:application/json,{"country_code": "AU"}');
  Services.search.init(() => {
    equal(Services.prefs.getCharPref("browser.search.countryCode"), "AU", "got the correct country code.");
    equal(Services.prefs.getCharPref("browser.search.region"), "AU", "region pref also set to the countryCode.")
    
    ok(!Services.prefs.prefHasUserValue("browser.search.isUS"), "no isUS pref")
    
    checkCountryResultTelemetry(TELEMETRY_RESULT_ENUM.SUCCESS);
    
    for (let hid of ["SEARCH_SERVICE_COUNTRY_TIMEOUT",
                     "SEARCH_SERVICE_COUNTRY_FETCH_CAUSED_SYNC_INIT"]) {
      let histogram = Services.telemetry.getHistogramById(hid);
      let snapshot = histogram.snapshot();
      deepEqual(snapshot.counts, [1,0,0]); 

    }

    
    
    
    
    
    if (Services.appinfo.OS == "Darwin") {
      let gfxInfo2 = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo2);
      print("OSX says the country-code is", gfxInfo2.countryCode);
      let expectedResult;
      let hid;
      
      
      if (gfxInfo2.countryCode == "US") {
        hid = "SEARCH_SERVICE_US_COUNTRY_MISMATCHED_PLATFORM_OSX";
        expectedResult = [0,1,0]; 
      } else {
        
        
        hid = "SEARCH_SERVICE_NONUS_COUNTRY_MISMATCHED_PLATFORM_OSX";
        expectedResult = gfxInfo2.countryCode == "AU" ? [1,0,0] : [0,1,0];
      }

      let histogram = Services.telemetry.getHistogramById(hid);
      let snapshot = histogram.snapshot();
      deepEqual(snapshot.counts, expectedResult);
    }
    do_test_finished();
    run_next_test();
  });
  do_test_pending();
}
