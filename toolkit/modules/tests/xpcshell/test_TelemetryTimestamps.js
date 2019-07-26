


const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
Cu.import("resource://gre/modules/Services.jsm");




Cu.import("resource://testing-common/AppInfo.jsm");
updateAppInfo();

function getSimpleMeasurementsFromTelemetryPing() {
  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
  let ping = TelemetryPing.getPayload();

  return ping.simpleMeasurements;
}

function run_test() {
  do_test_pending();
  const Telemetry = Services.telemetry;
  Telemetry.asyncFetchTelemetryData(function () {
    try {
      actualTest();
    }
    catch(e) {
      do_throw("Failed: " + e);
    }
    do_test_finished();
  });
}

function actualTest() {
  
  let tmp = {};
  Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", tmp);
  let TelemetryTimestamps = tmp.TelemetryTimestamps;
  let now = Date.now();
  TelemetryTimestamps.add("foo");
  do_check_true(TelemetryTimestamps.get().foo != null); 
  do_check_true(TelemetryTimestamps.get().foo >= now); 

  
  
  
  const YEAR_4000_IN_MS = 64060588800000;
  TelemetryTimestamps.add("bar", YEAR_4000_IN_MS);
  do_check_eq(TelemetryTimestamps.get().bar, YEAR_4000_IN_MS); 

  
  TelemetryTimestamps.add("bar", 2);
  do_check_eq(TelemetryTimestamps.get().bar, YEAR_4000_IN_MS); 

  let threw = false;
  try {
    TelemetryTimestamps.add("baz", "this isn't a number");
  } catch (ex) {
    threw = true;
  }
  do_check_true(threw); 
  do_check_null(TelemetryTimestamps.get().baz); 

  
  let simpleMeasurements = getSimpleMeasurementsFromTelemetryPing();
  do_check_true(simpleMeasurements != null); 
  do_check_true(simpleMeasurements.foo > 1); 
  do_check_true(simpleMeasurements.bar > 1); 
  do_check_null(simpleMeasurements.baz); 
}
