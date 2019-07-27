





Components.utils.import("resource://gre/modules/Promise.jsm", this);

add_task(loadLoopPanel);




add_task(function* test_initialize() {
  let oldCanRecord = Services.telemetry.canRecord;
  Services.telemetry.canRecord = true;
  registerCleanupFunction(function () {
    Services.telemetry.canRecord = oldCanRecord;
  });
});




add_task(function* test_mozLoop_telemetryAdd_buckets() {
  let histogramId = "LOOP_TWO_WAY_MEDIA_CONN_LENGTH";
  let histogram = Services.telemetry.getKeyedHistogramById(histogramId);
  let CONN_LENGTH = gMozLoopAPI.TWO_WAY_MEDIA_CONN_LENGTH;

  histogram.clear();
  for (let value of [CONN_LENGTH.SHORTER_THAN_10S,
                     CONN_LENGTH.BETWEEN_10S_AND_30S,
                     CONN_LENGTH.BETWEEN_10S_AND_30S,
                     CONN_LENGTH.BETWEEN_30S_AND_5M,
                     CONN_LENGTH.BETWEEN_30S_AND_5M,
                     CONN_LENGTH.BETWEEN_30S_AND_5M,
                     CONN_LENGTH.MORE_THAN_5M,
                     CONN_LENGTH.MORE_THAN_5M,
                     CONN_LENGTH.MORE_THAN_5M,
                     CONN_LENGTH.MORE_THAN_5M]) {
    gMozLoopAPI.telemetryAddKeyedValue(histogramId, value);
  }

  let snapshot = histogram.snapshot();
  is(snapshot["SHORTER_THAN_10S"].sum, 1, "TWO_WAY_MEDIA_CONN_LENGTH.SHORTER_THAN_10S");
  is(snapshot["BETWEEN_10S_AND_30S"].sum, 2, "TWO_WAY_MEDIA_CONN_LENGTH.BETWEEN_10S_AND_30S");
  is(snapshot["BETWEEN_30S_AND_5M"].sum, 3, "TWO_WAY_MEDIA_CONN_LENGTH.BETWEEN_30S_AND_5M");
  is(snapshot["MORE_THAN_5M"].sum, 4, "TWO_WAY_MEDIA_CONN_LENGTH.MORE_THAN_5M");
});
