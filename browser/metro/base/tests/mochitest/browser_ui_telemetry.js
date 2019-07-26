




"use strict";

function test() {
  runTests();
}

function getSimpleMeasurementsFromTelemetryPing() {
  return Cu.import("resource://gre/modules/TelemetryPing.jsm", {}).
    TelemetryPing.getPayload().simpleMeasurements;
}

gTests.push({
  desc: "Test browser-ui telemetry",
  run: function testBrowserUITelemetry() {
    
    let simpleMeasurements = getSimpleMeasurementsFromTelemetryPing();
    ok(simpleMeasurements, "simpleMeasurements are truthy");
    ok(simpleMeasurements.UITelemetry["metro-ui"]["window-width"], "window-width measurement was captured");
    ok(simpleMeasurements.UITelemetry["metro-ui"]["window-height"], "window-height measurement was captured");
  }
});
