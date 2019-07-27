






add_task(loadLoopPanel);




add_task(function* test_mozLoop_telemetryAdd_boolean() {
  for (let histogramId of [
    "LOOP_CLIENT_CALL_URL_REQUESTS_SUCCESS",
  ]) {
    let snapshot = Services.telemetry.getHistogramById(histogramId).snapshot();

    let initialFalseCount = snapshot.counts[0];
    let initialTrueCount = snapshot.counts[1];

    for (let value of [false, false, true]) {
      gMozLoopAPI.telemetryAdd(histogramId, value);
    }

    
    
    info("Waiting for update of " + histogramId);
    do {
      yield new Promise(resolve => setTimeout(resolve, 50));
      snapshot = Services.telemetry.getHistogramById(histogramId).snapshot();
    } while (snapshot.counts[0] == initialFalseCount + 2 &&
             snapshot.counts[1] == initialTrueCount + 1);
    ok(true, "Correctly updated " + histogramId);
  }
});
