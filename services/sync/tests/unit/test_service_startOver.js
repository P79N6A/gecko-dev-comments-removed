


Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/status.js");
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
  
  setBasicCredentials("foobar", "blablabla", 
                      "abcdeabcdeabcdeabcdeabcdea");
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
  do_check_eq(Identity.basicPassword, null);
  do_check_eq(Identity.syncKey, null);

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

  Svc.Prefs.set("serverURL", TEST_SERVER_URL);
  Svc.Prefs.set("clusterURL", TEST_CLUSTER_URL);
  
  do_check_false(engine.removed);
  Service.startOver();
  do_check_true(engine.removed);

  run_next_test();
});

add_test(function test_reset_SyncScheduler() {
  
  Service.scheduler.idle = true;
  Service.scheduler.hasIncomingItems = true;
  Service.scheduler.numClients = 42;
  Service.scheduler.nextSync = Date.now();
  Service.scheduler.syncThreshold = MULTI_DEVICE_THRESHOLD;
  Service.scheduler.syncInterval = Service.scheduler.activeInterval;

  Service.startOver();

  do_check_false(Service.scheduler.idle);
  do_check_false(Service.scheduler.hasIncomingItems);
  do_check_eq(Service.scheduler.numClients, 0);
  do_check_eq(Service.scheduler.nextSync, 0);
  do_check_eq(Service.scheduler.syncThreshold, SINGLE_USER_THRESHOLD);
  do_check_eq(Service.scheduler.syncInterval, Service.scheduler.singleDeviceInterval);

  run_next_test();
});
