




const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/TelemetryPing.jsm");

function run_test() {
  do_test_pending();

  Services.obs.addObserver(function observeTelemetry() {
    Services.obs.removeObserver(observeTelemetry, "gather-telemetry");
    do_test_finished();
  }, "gather-telemetry", false);

  TelemetryPing.observe(null, "idle-daily", null);
}
