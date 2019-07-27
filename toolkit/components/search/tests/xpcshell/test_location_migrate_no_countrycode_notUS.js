




function run_test() {
  installTestEngine();

  
  Services.prefs.setBoolPref("browser.search.isUS", false);
  
  Services.prefs.setCharPref("browser.search.geoip.url", 'data:application/json,{"country_code": "US"}');
  Services.search.init(() => {
    
    equal(Services.prefs.getCharPref("browser.search.region"), "US", "got the correct region.");
    equal(Services.prefs.getCharPref("browser.search.countryCode"), "US", "got the correct country code.");
    
    checkCountryResultTelemetry(TELEMETRY_RESULT_ENUM.SUCCESS);
    
    for (let hid of ["SEARCH_SERVICE_COUNTRY_TIMEOUT",
                     "SEARCH_SERVICE_COUNTRY_FETCH_CAUSED_SYNC_INIT"]) {
      let histogram = Services.telemetry.getHistogramById(hid);
      let snapshot = histogram.snapshot();
      deepEqual(snapshot.counts, [1,0,0]); 
    }
    do_test_finished();
    run_next_test();
  });
  do_test_pending();
}
