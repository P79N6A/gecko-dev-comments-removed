




function run_test() {
  installTestEngine();

  
  Services.prefs.setBoolPref("browser.search.isUS", true);
  Services.prefs.setCharPref("browser.search.countryCode", "US");
  
  Services.prefs.setCharPref("browser.search.geoip.url", 'data:application/json,{"country_code": "AU"}');
  Services.search.init(() => {
    
    equal(Services.prefs.getCharPref("browser.search.region"), "US", "got the correct region.");
    equal(Services.prefs.getCharPref("browser.search.countryCode"), "US", "got the correct country code.");
    
    checkCountryResultTelemetry(null);
    do_test_finished();
    run_next_test();
  });
  do_test_pending();
}
