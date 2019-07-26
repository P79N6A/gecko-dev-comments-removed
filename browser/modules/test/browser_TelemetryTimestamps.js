


function getSimpleMeasurementsFromTelemetryPing() {
  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
  let ping = TelemetryPing.getPayload();

  return ping.simpleMeasurements;
}

function test() {
  waitForExplicitFinish()
  const Telemetry = Services.telemetry;
  Telemetry.asyncFetchTelemetryData(function () {
    actualTest();
    finish();
  });
}

function actualTest() {
  
  let tmp = {};
  Cu.import("resource:///modules/TelemetryTimestamps.jsm", tmp);
  let TelemetryTimestamps = tmp.TelemetryTimestamps;
  let now = Date.now();
  TelemetryTimestamps.add("foo");
  ok(TelemetryTimestamps.get().foo, "foo was added");
  ok(TelemetryTimestamps.get().foo >= now, "foo has a reasonable value");

  
  
  
  const YEAR_4000_IN_MS = 64060588800000;
  TelemetryTimestamps.add("bar", YEAR_4000_IN_MS);
  ok(TelemetryTimestamps.get().bar, "bar was added");
  is(TelemetryTimestamps.get().bar, YEAR_4000_IN_MS, "bar has the right value");

  
  TelemetryTimestamps.add("bar", 2);
  is(TelemetryTimestamps.get().bar, YEAR_4000_IN_MS, "bar wasn't overwritten");

  let threw = false;
  try {
    TelemetryTimestamps.add("baz", "this isn't a number");
  } catch (ex) {
    threw = true;
  }
  ok(threw, "adding baz threw");
  ok(!TelemetryTimestamps.get().baz, "no baz was added");

  
  let simpleMeasurements = getSimpleMeasurementsFromTelemetryPing();
  ok(simpleMeasurements, "got simple measurements from ping data");
  ok(simpleMeasurements.foo > 1, "foo was included");
  ok(simpleMeasurements.bar > 1, "bar was included");
  ok(!simpleMeasurements.baz, "baz wasn't included since it wasn't added");

  
  let props = [
    
    
    
    "sessionRestoreInitialized",
    
    
  ];

  props.forEach(function (p) {
    let value = simpleMeasurements[p];
    ok(value, p + " exists");
    ok(!isNaN(value), p + " is a number");
    ok(value > 0, p + " value is reasonable");
  });
}
