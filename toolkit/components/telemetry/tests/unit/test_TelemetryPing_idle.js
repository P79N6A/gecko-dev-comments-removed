




Components.utils.import("resource://gre/modules/Services.jsm");

function run_test() {
  do_test_pending();

  Services.obs.addObserver(function observeTelemetry() {
    Services.obs.removeObserver(observeTelemetry, "gather-telemetry");
    do_test_finished();
  }, "gather-telemetry", false);

  Components.classes["@mozilla.org/base/telemetry-ping;1"]
    .getService(Components.interfaces.nsIObserver)
    .observe(null, "idle-daily", null);
}
