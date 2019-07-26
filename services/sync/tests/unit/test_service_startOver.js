


Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services/sync/utils.js");

function BlaEngine() {
  SyncEngine.call(this, "Bla", Service);
}
BlaEngine.prototype = {
  __proto__: SyncEngine.prototype,

  removed: false,
  removeClientData: function() {
    this.removed = true;
  }

};

Service.engineManager.register(BlaEngine);


function run_test() {
  initTestLogging("Trace");
  run_next_test();
}

add_test(function test_resetLocalData() {
  
  setBasicCredentials("foobar", "blablabla", 
                      "abcdeabcdeabcdeabcdeabcdea");
  Service.status.enforceBackoff = true;
  Service.status.backoffInterval = 42;
  Service.status.minimumNextSync = 23;
  Service.persistLogin();

  
  do_check_eq(Service.status.checkSetup(), STATUS_OK);

  
  let observerCalled = false;
  Svc.Obs.add("weave:service:start-over", function onStartOver() {
    Svc.Obs.remove("weave:service:start-over", onStartOver);
    observerCalled = true;

    do_check_eq(Service.status.service, CLIENT_NOT_CONFIGURED);
  });

  Service.startOver();
  do_check_true(observerCalled);

  
  do_check_eq(Svc.Prefs.get("username"), undefined);
  do_check_eq(Service.identity.basicPassword, null);
  do_check_eq(Service.identity.syncKey, null);

  do_check_eq(Service.status.service, CLIENT_NOT_CONFIGURED);
  do_check_false(Service.status.enforceBackoff);
  do_check_eq(Service.status.backoffInterval, 0);
  do_check_eq(Service.status.minimumNextSync, 0);

  run_next_test();
});

add_test(function test_removeClientData() {
  let engine = Service.engineManager.get("bla");

  
  do_check_false(engine.removed);
  Service.startOver();
  do_check_false(engine.removed);

  Service.serverURL = "https://localhost/";
  Service.clusterURL = Service.serverURL;

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
