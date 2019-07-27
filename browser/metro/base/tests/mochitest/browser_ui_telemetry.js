




"use strict";

Cu.import("resource://gre/modules/TelemetrySession.jsm", this);

function test() {
  runTests();
}

function getTelemetryPayload() {
  return TelemetrySession.getPayload();
}

gTests.push({
  desc: "Setup",
  run: () => { yield TelemetrySession.setup(); }
});

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

gTests.push({
  desc: "Test tab count telemetry",
  run: function() {
    
    yield waitForCondition(() => window.__SSID);

    Services.obs.notifyObservers(null, "reset-telemetry-vars", null);
    yield waitForCondition(function () {
      let simpleMeasurements = getTelemetryPayload().simpleMeasurements;
      return simpleMeasurements.UITelemetry["metro-tabs"]["currTabCount"] == 1;
    });

    let simpleMeasurements = getTelemetryPayload().simpleMeasurements;
    is(simpleMeasurements.UITelemetry["metro-tabs"]["currTabCount"], 1);
    is(simpleMeasurements.UITelemetry["metro-tabs"]["maxTabCount"], 1);

    let tab2 = Browser.addTab("about:mozilla");
    simpleMeasurements = getTelemetryPayload().simpleMeasurements;
    is(simpleMeasurements.UITelemetry["metro-tabs"]["currTabCount"], 2);
    is(simpleMeasurements.UITelemetry["metro-tabs"]["maxTabCount"], 2);

    let tab3 = Browser.addTab("about:config");
    simpleMeasurements = getTelemetryPayload().simpleMeasurements;
    is(simpleMeasurements.UITelemetry["metro-tabs"]["currTabCount"], 3);
    is(simpleMeasurements.UITelemetry["metro-tabs"]["maxTabCount"], 3);

    Browser.closeTab(tab2, { forceClose: true } );
    simpleMeasurements = getTelemetryPayload().simpleMeasurements;
    is(simpleMeasurements.UITelemetry["metro-tabs"]["currTabCount"], 2);
    is(simpleMeasurements.UITelemetry["metro-tabs"]["maxTabCount"], 3);

    Browser.closeTab(tab3, { forceClose: true } );
    simpleMeasurements = getTelemetryPayload().simpleMeasurements;
    is(simpleMeasurements.UITelemetry["metro-tabs"]["currTabCount"], 1);
    is(simpleMeasurements.UITelemetry["metro-tabs"]["maxTabCount"], 3);
  }
});

gTests.push({
  desc: "Shutdown",
  run: () => { yield TelemetrySession.shutdown(); }
});
