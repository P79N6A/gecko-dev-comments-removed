





Components.utils.import("resource://gre/modules/Promise.jsm", this);

add_task(loadLoopPanel);




add_task(function* test_initialize() {
  let oldCanRecord = Services.telemetry.canRecord;
  Services.telemetry.canRecord = true;
  registerCleanupFunction(function () {
    Services.telemetry.canRecord = oldCanRecord;
  });
});




add_task(function* test_mozLoop_telemetryAdd_boolean() {
  for (let histogramId of [
    "LOOP_CLIENT_CALL_URL_REQUESTS_SUCCESS",
    "LOOP_CLIENT_CALL_URL_SHARED",
  ]) {
    let histogram = Services.telemetry.getHistogramById(histogramId);

    histogram.clear();
    for (let value of [false, false, true]) {
      gMozLoopAPI.telemetryAdd(histogramId, value);
    }

    let snapshot = histogram.snapshot();
    is(snapshot.counts[0], 2, "snapshot.counts[0] == 2");
    is(snapshot.counts[1], 1, "snapshot.counts[1] == 1");
  }
});
