




const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/TelemetrySession.jsm", this);

function run_test() {
  do_test_pending();

  Services.obs.addObserver(function observeTelemetry() {
    Services.obs.removeObserver(observeTelemetry, "gather-telemetry");
    do_test_finished();
  }, "gather-telemetry", false);

  TelemetrySession.observe(null, "idle-daily", null);
}
