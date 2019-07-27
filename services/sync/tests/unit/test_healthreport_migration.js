


"use strict";

Cu.import("resource://gre/modules/Metrics.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://services-sync/healthreport.jsm", this);
Cu.import("resource://services-sync/FxaMigrator.jsm", this);
Cu.import("resource://testing-common/services/common/logging.js", this);
Cu.import("resource://testing-common/services/healthreport/utils.jsm", this);


function run_test() {
  initTestLogging();

  run_next_test();
}

add_task(function* test_no_data() {
  let storage = yield Metrics.Storage("collect");
  let provider = new SyncProvider();
  yield provider.init(storage);

  try {
    
    let now = new Date();
    yield provider.collectDailyData();

    let m = provider.getMeasurement("migration", 1);
    let values = yield m.getValues();
    Assert.equal(values.days.size, 0);
    Assert.ok(!values.days.hasDay(now));
  } finally {
    yield provider.shutdown();
    yield storage.close();
  }
});

function checkCorrectStateRecorded(provider, state) {
  
  yield m.storage.enqueueOperation(() => {
    return Promise.resolve();
  });

  let m = provider.getMeasurement("migration", 1);
  let values = yield m.getValues();
  Assert.equal(values.days.size, 1);
  Assert.ok(values.days.hasDay(now));
  let day = values.days.getDay(now);

  Assert.ok(day.has("state"));
  Assert.equal(day.get("state"), state);
}

add_task(function* test_state() {
  let storage = yield Metrics.Storage("collect");
  let provider = new SyncProvider();
  yield provider.init(storage);

  try {
    
    let now = new Date();

    
    
    Services.obs.notifyObservers(null, "fxa-migration:state-changed",
                                 fxaMigrator.STATE_USER_FXA_VERIFIED);
    checkCorrectStateRecorded(provider, fxaMigrator.STATE_USER_FXA_VERIFIED);

    
    Services.obs.notifyObservers(null, "fxa-migration:internal-state-changed",
                                 fxaMigrator.STATE_INTERNAL_WAITING_SYNC_COMPLETE);
    checkCorrectStateRecorded(provider, fxaMigrator.STATE_INTERNAL_WAITING_SYNC_COMPLETE);
  } finally {
    yield provider.shutdown();
    yield storage.close();
  }
});

add_task(function* test_flags() {
  let storage = yield Metrics.Storage("collect");
  let provider = new SyncProvider();
  yield provider.init(storage);

  try {
    
    let now = new Date();

    let m = provider.getMeasurement("migration", 1);

    let record = function*(what) {
      Services.obs.notifyObservers(null, "fxa-migration:internal-telemetry", what);
      
      yield m.storage.enqueueOperation(Promise.resolve);
      let values = yield m.getValues();
      Assert.equal(values.days.size, 1);
      return values.days.getDay(now);
    }

    let values = yield m.getValues();
    Assert.equal(values.days.size, 1);
    let day = values.days.getDay(now);
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_ACCEPTED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_DECLINED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_UNLINKED));

    
    day = yield record("unknown");
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_ACCEPTED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_DECLINED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_UNLINKED));

    
    day = yield record(fxaMigrator.TELEMETRY_ACCEPTED);
    Assert.ok(day.has(fxaMigrator.TELEMETRY_ACCEPTED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_DECLINED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_UNLINKED));
    Assert.equal(day.get(fxaMigrator.TELEMETRY_ACCEPTED), 1);

    
    day = yield record(fxaMigrator.TELEMETRY_ACCEPTED);
    Assert.equal(day.get(fxaMigrator.TELEMETRY_ACCEPTED), 2);

    
    day = yield record(fxaMigrator.TELEMETRY_DECLINED);
    Assert.ok(day.has(fxaMigrator.TELEMETRY_ACCEPTED));
    Assert.ok(day.has(fxaMigrator.TELEMETRY_DECLINED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_UNLINKED));
    Assert.equal(day.get(fxaMigrator.TELEMETRY_ACCEPTED), 2);
    Assert.equal(day.get(fxaMigrator.TELEMETRY_DECLINED), 1);

    day = yield record(fxaMigrator.TELEMETRY_DECLINED);
    Assert.ok(day.has(fxaMigrator.TELEMETRY_ACCEPTED));
    Assert.ok(day.has(fxaMigrator.TELEMETRY_DECLINED));
    Assert.ok(!day.has(fxaMigrator.TELEMETRY_UNLINKED));
    Assert.equal(day.get(fxaMigrator.TELEMETRY_ACCEPTED), 2);
    Assert.equal(day.get(fxaMigrator.TELEMETRY_DECLINED), 2);

    
    
    day = yield record(fxaMigrator.TELEMETRY_UNLINKED);
    Assert.ok(day.has(fxaMigrator.TELEMETRY_ACCEPTED));
    Assert.ok(day.has(fxaMigrator.TELEMETRY_DECLINED));
    Assert.ok(day.has(fxaMigrator.TELEMETRY_UNLINKED));
    Assert.equal(day.get(fxaMigrator.TELEMETRY_UNLINKED), 1);
    
    day = yield record(fxaMigrator.TELEMETRY_UNLINKED);
    Assert.equal(day.get(fxaMigrator.TELEMETRY_UNLINKED), 1);
  } finally {
    yield provider.shutdown();
    yield storage.close();
  }
});
