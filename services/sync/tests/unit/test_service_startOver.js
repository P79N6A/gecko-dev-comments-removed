Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/util.js");

function BlaEngine() {
  SyncEngine.call(this, "Bla");
}
BlaEngine.prototype = {
  __proto__: SyncEngine.prototype,

  removed: false,
  removeClientData: function() {
    this.removed = true;
  }

};
Engines.register(BlaEngine);


function run_test() {
  initTestLogging("Trace");
  run_next_test();
}

add_test(function test_resetLocalData() {
  
  Service.username = "foobar";
  Service.password = "blablabla";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Status.enforceBackoff = true;
  Status.backoffInterval = 42;
  Status.minimumNextSync = 23;
  Service.persistLogin();

  
  do_check_eq(Status.checkSetup(), STATUS_OK);

  
  let observerCalled = false;
  Svc.Obs.add("weave:service:start-over", function onStartOver() {
    Svc.Obs.remove("weave:service:start-over", onStartOver);
    observerCalled = true;

    do_check_eq(Status.service, CLIENT_NOT_CONFIGURED);
  });

  Service.startOver();
  do_check_true(observerCalled);

  
  do_check_eq(Svc.Prefs.get("username"), undefined);
  do_check_eq(Service.password, "");
  do_check_eq(Service.passphrase, "");

  do_check_eq(Status.service, CLIENT_NOT_CONFIGURED);
  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.backoffInterval, 0);
  do_check_eq(Status.minimumNextSync, 0);

  run_next_test();
});

add_test(function test_removeClientData() {
  let engine = Engines.get("bla");

  
  do_check_false(engine.removed);
  Service.startOver();
  do_check_false(engine.removed);

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  do_check_false(engine.removed);
  Service.startOver();
  do_check_true(engine.removed);

  run_next_test();
});

add_test(function test_reset_SyncScheduler() {
  
  SyncScheduler.idle = true;
  SyncScheduler.hasIncomingItems = true;
  SyncScheduler.numClients = 42;
  SyncScheduler.nextSync = Date.now();
  SyncScheduler.syncThreshold = MULTI_DEVICE_THRESHOLD;
  SyncScheduler.syncInterval = SyncScheduler.activeInterval;

  Service.startOver();

  do_check_false(SyncScheduler.idle);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.numClients, 0);
  do_check_eq(SyncScheduler.nextSync, 0);
  do_check_eq(SyncScheduler.syncThreshold, SINGLE_USER_THRESHOLD);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  run_next_test();
});
