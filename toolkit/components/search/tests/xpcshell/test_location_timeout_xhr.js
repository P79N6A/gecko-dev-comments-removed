





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

function verifyProbeSum(probe, sum) {
  let histogram = Services.telemetry.getHistogramById(probe);
  let snapshot = histogram.snapshot();
  equal(snapshot.sum, sum, probe);
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
  
  Services.prefs.setIntPref("browser.search.geoip.timeout", 10);
  let promiseXHRStarted = waitForSearchNotification("geoip-lookup-xhr-starting");
  Services.search.init(() => {
    ok(!Services.prefs.prefHasUserValue("browser.search.countryCode"), "should be no countryCode pref");
    ok(!Services.prefs.prefHasUserValue("browser.search.region"), "should be no region pref");
    
    checkCountryResultTelemetry(null);

    
    let histogram = Services.telemetry.getHistogramById("SEARCH_SERVICE_COUNTRY_TIMEOUT");
    let snapshot = histogram.snapshot();
    deepEqual(snapshot.counts, [0,1,0]);

    
    
    verifyProbeSum("SEARCH_SERVICE_COUNTRY_FETCH_TIME_MS", 0);

    promiseXHRStarted.then(xhr => {
      
      
      xhr.timeout = 10;
      
      waitForSearchNotification("geoip-lookup-xhr-complete").then(() => {
        
        checkCountryResultTelemetry(TELEMETRY_RESULT_ENUM.XHRTIMEOUT);
        
        
        verifyProbeSum("SEARCH_SERVICE_COUNTRY_FETCH_TIME_MS", 0);
        
        ok(!Services.prefs.prefHasUserValue("browser.search.countryCode"), "should be no countryCode pref");
        ok(!Services.prefs.prefHasUserValue("browser.search.region"), "should be no region pref");

        
        resolveContinuePromise();

        do_test_finished();
        server.stop(run_next_test);
      });
    });
  });
  do_test_pending();
}
