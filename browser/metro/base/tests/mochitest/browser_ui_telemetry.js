




"use strict";

function test() {
  runTests();
}

function getTelemetryPayload() {
  return Cu.import("resource://gre/modules/TelemetryPing.jsm", {}).
    TelemetryPing.getPayload();
}

gTests.push({
  desc: "Test browser-ui telemetry",
  run: function testBrowserUITelemetry() {
    
    is(getTelemetryPayload().info.appName, "MetroFirefox");

    let simpleMeasurements = getTelemetryPayload().simpleMeasurements;
    ok(simpleMeasurements, "simpleMeasurements are truthy");
    ok(simpleMeasurements.UITelemetry["metro-ui"]["window-width"], "window-width measurement was captured");
    ok(simpleMeasurements.UITelemetry["metro-ui"]["window-height"], "window-height measurement was captured");
  }
});
