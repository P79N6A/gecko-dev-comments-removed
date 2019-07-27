


const Telemetry = Cc["@mozilla.org/base/telemetry;1"]
                  .getService(Ci.nsITelemetry);

let tmpScope = {};
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", tmpScope);
let TelemetryStopwatch = tmpScope.TelemetryStopwatch;



const HIST_NAME = "TELEMETRY_PING";
const HIST_NAME2 = "RANGE_CHECKSUM_ERRORS";

let refObj = {}, refObj2 = {};

let originalCount1, originalCount2;

function run_test() {
  let histogram = Telemetry.getHistogramById(HIST_NAME);
  let snapshot = histogram.snapshot();
  originalCount1 = snapshot.counts.reduce(function (a,b) a += b);

  histogram = Telemetry.getHistogramById(HIST_NAME2);
  snapshot = histogram.snapshot();
  originalCount2 = snapshot.counts.reduce(function (a,b) a += b);

  do_check_false(TelemetryStopwatch.start(3));
  do_check_false(TelemetryStopwatch.start({}));
  do_check_false(TelemetryStopwatch.start("", 3));
  do_check_false(TelemetryStopwatch.start("", ""));
  do_check_false(TelemetryStopwatch.start({}, {}));

  do_check_true(TelemetryStopwatch.start("mark1"));
  do_check_true(TelemetryStopwatch.start("mark2"));

  do_check_true(TelemetryStopwatch.start("mark1", refObj));
  do_check_true(TelemetryStopwatch.start("mark2", refObj));

  
  do_check_false(TelemetryStopwatch.start("mark1"));
  do_check_false(TelemetryStopwatch.start("mark1", refObj));

  
  do_check_false(TelemetryStopwatch.finish("mark1"));
  do_check_false(TelemetryStopwatch.finish("mark1", refObj));

  do_check_true(TelemetryStopwatch.start("NON-EXISTENT_HISTOGRAM"));
  try {
    TelemetryStopwatch.finish("NON-EXISTENT_HISTOGRAM");
    do_throw("Non-existent histogram name should throw an error.");
  } catch (e) {}

  do_check_true(TelemetryStopwatch.start("NON-EXISTENT_HISTOGRAM", refObj));
  try {
    TelemetryStopwatch.finish("NON-EXISTENT_HISTOGRAM", refObj);
    do_throw("Non-existent histogram name should throw an error.");
  } catch (e) {}

  do_check_true(TelemetryStopwatch.start(HIST_NAME));
  do_check_true(TelemetryStopwatch.start(HIST_NAME2));
  do_check_true(TelemetryStopwatch.start(HIST_NAME, refObj));
  do_check_true(TelemetryStopwatch.start(HIST_NAME2, refObj));
  do_check_true(TelemetryStopwatch.start(HIST_NAME, refObj2));
  do_check_true(TelemetryStopwatch.start(HIST_NAME2, refObj2));

  do_check_true(TelemetryStopwatch.finish(HIST_NAME));
  do_check_true(TelemetryStopwatch.finish(HIST_NAME2));
  do_check_true(TelemetryStopwatch.finish(HIST_NAME, refObj));
  do_check_true(TelemetryStopwatch.finish(HIST_NAME2, refObj));
  do_check_true(TelemetryStopwatch.finish(HIST_NAME, refObj2));
  do_check_true(TelemetryStopwatch.finish(HIST_NAME2, refObj2));

  
  do_check_false(TelemetryStopwatch.finish(HIST_NAME));
  do_check_false(TelemetryStopwatch.finish(HIST_NAME, refObj));

  
  do_check_true(TelemetryStopwatch.start(HIST_NAME));
  do_check_true(TelemetryStopwatch.start(HIST_NAME, refObj));
  do_check_true(TelemetryStopwatch.finish(HIST_NAME));
  do_check_true(TelemetryStopwatch.finish(HIST_NAME, refObj));

  do_check_false(TelemetryStopwatch.finish("unknown-mark")); 
  do_check_false(TelemetryStopwatch.finish("unknown-mark", {})); 
  do_check_false(TelemetryStopwatch.finish(HIST_NAME, {})); 

  
  do_check_true(TelemetryStopwatch.start(HIST_NAME));
  do_check_true(TelemetryStopwatch.start(HIST_NAME, refObj));
  do_check_true(TelemetryStopwatch.cancel(HIST_NAME));
  do_check_true(TelemetryStopwatch.cancel(HIST_NAME, refObj));

  
  do_check_false(TelemetryStopwatch.cancel(HIST_NAME));
  do_check_false(TelemetryStopwatch.cancel(HIST_NAME, refObj));

  
  do_check_false(TelemetryStopwatch.finish(HIST_NAME));
  do_check_false(TelemetryStopwatch.finish(HIST_NAME, refObj));

  finishTest();
}

function finishTest() {
  let histogram = Telemetry.getHistogramById(HIST_NAME);
  let snapshot = histogram.snapshot();
  let newCount = snapshot.counts.reduce(function (a,b) a += b);

  do_check_eq(newCount - originalCount1, 5, "The correct number of histograms were added for histogram 1.");

  histogram = Telemetry.getHistogramById(HIST_NAME2);
  snapshot = histogram.snapshot();
  newCount = snapshot.counts.reduce(function (a,b) a += b);

  do_check_eq(newCount - originalCount2, 3, "The correct number of histograms were added for histogram 2.");
}
