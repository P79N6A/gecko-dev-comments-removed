




function startServer(continuePromise) {
  let srv = new HttpServer();
  function lookupCountry(metadata, response) {
    response.processAsync();
    
    
    
    
    continuePromise.then(() => {
      response.setStatusLine("1.1", 200, "OK");
      response.write('{"country_code" : "AU"}');
      response.finish();
    });
  }
  srv.registerPathHandler("/lookup_country", lookupCountry);
  srv.start(-1);
  return srv;
}

function getProbeSum(probe, sum) {
  let histogram = Services.telemetry.getHistogramById(probe);
  return histogram.snapshot().sum;
}

function run_test() {
  installTestEngine();

  let resolveContinuePromise;
  let continuePromise = new Promise(resolve => {
    resolveContinuePromise = resolve;
  });

  let server = startServer(continuePromise);
  let url = "http://localhost:" + server.identity.primaryPort + "/lookup_country";
  Services.prefs.setCharPref("browser.search.geoip.url", url);
  Services.prefs.setIntPref("browser.search.geoip.timeout", 50);
  Services.search.init(() => {
    ok(!Services.prefs.prefHasUserValue("browser.search.countryCode"), "should be no countryCode pref");
    ok(!Services.prefs.prefHasUserValue("browser.search.region"), "should be no region pref");
    
    checkCountryResultTelemetry(null);

    
    let histogram = Services.telemetry.getHistogramById("SEARCH_SERVICE_COUNTRY_TIMEOUT");
    let snapshot = histogram.snapshot();
    deepEqual(snapshot.counts, [0,1,0]);
    
    
    equal(getProbeSum("SEARCH_SERVICE_COUNTRY_FETCH_TIME_MS"), 0);

    waitForSearchNotification("geoip-lookup-xhr-complete").then(() => {
      
      
      
      ok(getProbeSum("SEARCH_SERVICE_COUNTRY_FETCH_TIME_MS") != 0);
      
      checkCountryResultTelemetry(TELEMETRY_RESULT_ENUM.SUCCESS);

      
      
      equal(Services.prefs.getCharPref("browser.search.countryCode"), "AU");
      equal(Services.prefs.getCharPref("browser.search.region"), "AU");
      ok(!Services.prefs.prefHasUserValue("browser.search.isUS"), "should never have an isUS pref");

      do_test_finished();
      server.stop(run_next_test);
    });
    
    
    resolveContinuePromise();
  });
  do_test_pending();
}
