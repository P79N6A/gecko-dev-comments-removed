



function promiseTimezoneMessage() {
  return new Promise(resolve => {
    let listener = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIConsoleListener]),
      observe : function (msg) {
        if (msg.message.startsWith("getIsUS() fell back to a timezone check with the result=")) {
          Services.console.unregisterListener(listener);
          resolve(msg);
        }
      }
    };
    Services.console.registerListener(listener);
  });
}

function run_test() {
  installTestEngine();

  
  let promiseTzMessage = promiseTimezoneMessage();

  
  Services.prefs.setCharPref("browser.search.geoip.url", 'data:application/json,{"country_code"');
  Services.search.init(() => {
    ok(!Services.prefs.prefHasUserValue("browser.search.countryCode"), "should be no countryCode pref");
    ok(!Services.prefs.prefHasUserValue("browser.search.region"), "should be no region pref");
    ok(!Services.prefs.prefHasUserValue("browser.search.isUS"), "should never be an isUS pref");
    
    
    Services.search.getEngines();
    ok(!Services.prefs.prefHasUserValue("browser.search.countryCode"), "should be no countryCode pref");
    ok(!Services.prefs.prefHasUserValue("browser.search.region"), "should be no region pref");
    ok(!Services.prefs.prefHasUserValue("browser.search.isUS"), "should never be an isUS pref");
    
    checkCountryResultTelemetry(TELEMETRY_RESULT_ENUM.SUCCESS_WITHOUT_DATA);
    
    for (let hid of ["SEARCH_SERVICE_COUNTRY_TIMEOUT",
                     "SEARCH_SERVICE_COUNTRY_FETCH_CAUSED_SYNC_INIT"]) {
      let histogram = Services.telemetry.getHistogramById(hid);
      let snapshot = histogram.snapshot();
      deepEqual(snapshot.counts, [1,0,0]); 
    }

    
    promiseTzMessage.then(msg => {
      print("Timezone message:", msg.message);
      ok(msg.message.endsWith(isUSTimezone().toString()), "fell back to timezone and it matches our timezone");
      do_test_finished();
      run_next_test();
    });
  });
  do_test_pending();
}
