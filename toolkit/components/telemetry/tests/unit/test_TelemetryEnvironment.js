


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/TelemetryEnvironment.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm", this);
Cu.import("resource://gre/modules/PromiseUtils.jsm", this);

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

add_task(function* test_prefWatchPolicies() {
  const PREF_TEST_1 = "toolkit.telemetry.test.pref_new";
  const PREF_TEST_2 = "toolkit.telemetry.test.pref1";
  const PREF_TEST_3 = "toolkit.telemetry.test.pref2";

  const expectedValue = "some-test-value";

  let prefsToWatch = {};
  prefsToWatch[PREF_TEST_1] = TelemetryEnvironment.RECORD_PREF_VALUE;
  prefsToWatch[PREF_TEST_2] = TelemetryEnvironment.RECORD_PREF_STATE;
  prefsToWatch[PREF_TEST_3] = TelemetryEnvironment.RECORD_PREF_STATE;

  yield TelemetryEnvironment.init();

  
  TelemetryEnvironment._watchPreferences(prefsToWatch);
  let deferred = PromiseUtils.defer();
  TelemetryEnvironment.registerChangeListener("testWatchPrefs", deferred.resolve);

  
  Preferences.set(PREF_TEST_1, expectedValue);
  Preferences.set(PREF_TEST_2, false);
  yield deferred.promise;

  
  TelemetryEnvironment.unregisterChangeListener("testWatchPrefs");

  
  let environmentData = yield TelemetryEnvironment.getEnvironmentData();

  let userPrefs = environmentData.settings.userPrefs;

  Assert.equal(userPrefs[PREF_TEST_1], expectedValue,
               "Environment contains the correct preference value.");
  Assert.equal(userPrefs[PREF_TEST_2], null,
               "Report that the pref was user set and has no value.");
  Assert.ok(!(PREF_TEST_3 in userPrefs),
            "Do not report if preference not user set.");

  yield TelemetryEnvironment.shutdown();
});

add_task(function* test_prefWatch_prefReset() {
  const PREF_TEST = "toolkit.telemetry.test.pref1";

  let prefsToWatch = {};
  prefsToWatch[PREF_TEST] = TelemetryEnvironment.RECORD_PREF_STATE;
  
  Preferences.set(PREF_TEST, false);

  yield TelemetryEnvironment.init();

  
  TelemetryEnvironment._watchPreferences(prefsToWatch);
  let deferred = PromiseUtils.defer();
  TelemetryEnvironment.registerChangeListener("testWatchPrefs_reset", deferred.resolve);

  
  Preferences.reset(PREF_TEST);
  yield deferred.promise;

  
  TelemetryEnvironment.unregisterChangeListener("testWatchPrefs_reset");
  yield TelemetryEnvironment.shutdown();
});

add_task(function*() {
  do_test_finished();
});
