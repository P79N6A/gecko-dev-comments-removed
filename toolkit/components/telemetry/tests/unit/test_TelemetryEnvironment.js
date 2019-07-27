


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/TelemetryEnvironment.jsm", this);

function run_test() {
  do_test_pending();
  do_get_profile();
  run_next_test();
}

function isRejected(promise) {
  return new Promise((resolve, reject) => {
    promise.then(() => resolve(false), () => resolve(true));
  });
}

add_task(function* test_initAndShutdown() {
  
  TelemetryEnvironment.init();
  yield TelemetryEnvironment.shutdown();
  TelemetryEnvironment.init();
  yield TelemetryEnvironment.shutdown();

  
  TelemetryEnvironment.init();
  TelemetryEnvironment.init();

  
  let data = yield TelemetryEnvironment.getEnvironmentData();
  Assert.ok(!!data);

  
  yield TelemetryEnvironment.shutdown();
  TelemetryEnvironment.registerChangeListener("foo", () => {});
  TelemetryEnvironment.unregisterChangeListener("foo");

  
  Assert.ok(yield isRejected(TelemetryEnvironment.shutdown()));
  Assert.ok(yield isRejected(TelemetryEnvironment.getEnvironmentData()));
});

add_task(function* test_changeNotify() {
  TelemetryEnvironment.init();

  
  let results = new Array(4).fill(false);
  for (let i=0; i<results.length; ++i) {
    let k = i;
    TelemetryEnvironment.registerChangeListener("test"+k, () => results[k] = true);
  }
  
  
  TelemetryEnvironment._onEnvironmentChange("foo");
  Assert.ok(results.every(val => val), "All change listeners should have been notified.");
  results.fill(false);
  TelemetryEnvironment._onEnvironmentChange("bar");
  Assert.ok(results.every(val => val), "All change listeners should have been notified.");

  
  for (let i=0; i<4; ++i) {
    TelemetryEnvironment.unregisterChangeListener("test"+i);
  }
});

add_task(function*() {
  do_test_finished();
});
