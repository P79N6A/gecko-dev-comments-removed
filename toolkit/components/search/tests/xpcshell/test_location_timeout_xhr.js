





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
    try {
      Services.prefs.getCharPref("browser.search.countryCode");
      ok(false, "not expecting countryCode to be set");
    } catch (ex) {}
    
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
        
        try {
          Services.prefs.getCharPref("browser.search.countryCode");
          ok(false, "not expecting countryCode to be set");
        } catch (ex) {}

        
        resolveContinuePromise();

        do_test_finished();
        server.stop(run_next_test);
      });
    });
  });
  do_test_pending();
}
