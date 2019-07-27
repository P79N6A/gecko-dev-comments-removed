


function getCountryCodePref() {
  try {
    return Services.prefs.getCharPref("browser.search.countryCode");
  } catch (_) {
    return undefined;
  }
}

function getIsUSPref() {
  try {
    return Services.prefs.getBoolPref("browser.search.isUS");
  } catch (_) {
    return undefined;
  }
}


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

  run_next_test();
}



add_task(function* test_simple() {
  deepEqual(getCountryCodePref(), undefined, "no countryCode pref");
  deepEqual(getIsUSPref(), undefined, "no isUS pref");

  
  Services.prefs.setCharPref("browser.search.geoip.url", 'data:application/json,{"country_code": "AU"}');

  ok(!Services.search.isInitialized);

  
  let promiseTzMessage = promiseTimezoneMessage();

  
  
  let engines = Services.search.getEngines();
  ok(Services.search.isInitialized);

  
  yield new Promise(resolve => {
    do_timeout(500, resolve);
  });

  let msg = yield promiseTzMessage;
  print("Timezone message:", msg.message);
  ok(msg.message.endsWith(isUSTimezone().toString()), "fell back to timezone and it matches our timezone");

  deepEqual(getCountryCodePref(), undefined, "didn't do the geoip xhr");
  
  for (let hid of [
    "SEARCH_SERVICE_COUNTRY_FETCH_RESULT",
    "SEARCH_SERVICE_COUNTRY_FETCH_TIME_MS",
    "SEARCH_SERVICE_COUNTRY_TIMEOUT",
    "SEARCH_SERVICE_US_COUNTRY_MISMATCHED_TIMEZONE",
    "SEARCH_SERVICE_US_TIMEZONE_MISMATCHED_COUNTRY",
    "SEARCH_SERVICE_COUNTRY_FETCH_CAUSED_SYNC_INIT",
    ]) {
      let histogram = Services.telemetry.getHistogramById(hid);
      let snapshot = histogram.snapshot();
      equal(snapshot.sum, 0, hid);
      switch (snapshot.histogram_type) {
        case Ci.nsITelemetry.HISTOGRAM_FLAG:
          
          
          deepEqual(snapshot.counts, [1,0,0], hid);
          break;
        case Ci.nsITelemetry.HISTOGRAM_BOOLEAN:
          
          deepEqual(snapshot.counts, [0,0,0], hid);
          break;
        case Ci.nsITelemetry.HISTOGRAM_EXPONENTIAL:
        case Ci.nsITelemetry.HISTOGRAM_LINEAR:
          equal(snapshot.counts.reduce((a, b) => a+b), 0, hid);
          break;
        default:
          ok(false, "unknown histogram type " + snapshot.histogram_type + " for " + hid);
      }
    }
});
