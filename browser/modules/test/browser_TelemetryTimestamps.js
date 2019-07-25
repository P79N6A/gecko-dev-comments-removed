


function getSimpleMeasurementsFromTelemetryPing() {
  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsIObserver);
  let str = Cc['@mozilla.org/supports-string;1'].createInstance(Ci.nsISupportsString);
  TelemetryPing.observe(str, "get-payload", "");

  return JSON.parse(str.data).simpleMeasurements;
}

function test() {
  
  Cu.import("resource:///modules/TelemetryTimestamps.jsm");
  let now = Date.now();
  TelemetryTimestamps.add("foo");
  let fooValue = TelemetryTimestamps.get().foo;
  ok(fooValue, "foo was added");
  ok(fooValue >= now, "foo has a reasonable value");

  
  TelemetryTimestamps.add("bar", 1);
  ok(TelemetryTimestamps.get().bar, "bar was added");
  is(TelemetryTimestamps.get().bar, 1, "bar has the right value");

  
  TelemetryTimestamps.add("bar", 2);
  is(TelemetryTimestamps.get().bar, 1, "bar wasn't overwritten");

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
  is(simpleMeasurements.foo, fooValue, "foo was included");
  is(simpleMeasurements.bar, 1, "bar was included");
  ok(!simpleMeasurements.baz, "baz wasn't included since it wasn't added");

  
  let props = [
    
    
    
    "sessionRestoreInitialized",
    
    
  ];

  props.forEach(function (p) {
    let value = simpleMeasurements[p];
    ok(value, p + " exists");
    ok(!isNaN(value), p + " is a number");
    ok(value > 0 && value < Date.now(), p + " value is reasonable");
  });
}
