


Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/status.js");

Svc.DefaultPrefs.set("registerEngines", "");
Cu.import("resource://services-sync/service.js");

function CatapultEngine() {
  SyncEngine.call(this, "Catapult");
}
CatapultEngine.prototype = {
  __proto__: SyncEngine.prototype,
  exception: null, 
  _sync: function _sync() {
    throw this.exception;
  }
};

Engines.register(CatapultEngine);

function sync_httpd_setup() {
  let global = new ServerWBO("global", {
    syncID: Service.syncID,
    storageVersion: STORAGE_VERSION,
    engines: {clients: {version: Clients.version,
                        syncID: Clients.syncID}}
  });
  let clientsColl = new ServerCollection({}, true);

  
  let collectionsHelper = track_collections_helper();
  let upd = collectionsHelper.with_updated_collection;

  return httpd_setup({
    "/1.1/johndoe/storage/meta/global": upd("meta", global.handler()),
    "/1.1/johndoe/info/collections": collectionsHelper.handler,
    "/1.1/johndoe/storage/crypto/keys":
      upd("crypto", (new ServerWBO("keys")).handler()),
    "/1.1/johndoe/storage/clients": upd("clients", clientsColl.handler()),
    "/user/1.0/johndoe/node/weave": httpd_handler(200, "OK", "null")
  });
}

function setUp() {
  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.clusterURL = "http://localhost:8080/";

  generateNewKeys();
  let serverKeys = CollectionKeys.asWBO("crypto", "keys");
  serverKeys.encrypt(Service.syncKeyBundle);
  return serverKeys.upload(Service.cryptoKeysURL).success;
}

function cleanUpAndGo(server) {
  Utils.nextTick(function () {
    Service.startOver();
    if (server) {
      server.stop(run_next_test);
    } else {
      run_next_test();
    }
  });
}

function run_test() {
  initTestLogging("Trace");

  Log4Moz.repository.getLogger("Sync.Service").level = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.SyncScheduler").level = Log4Moz.Level.Trace;

  run_next_test();
}

add_test(function test_prefAttributes() {
  _("Test various attributes corresponding to preferences.");

  const INTERVAL = 42 * 60 * 1000;   
  const THRESHOLD = 3142;
  const SCORE = 2718;
  const TIMESTAMP1 = 1275493471649;

  _("The 'nextSync' attribute stores a millisecond timestamp rounded down to the nearest second.");
  do_check_eq(SyncScheduler.nextSync, 0);
  SyncScheduler.nextSync = TIMESTAMP1;
  do_check_eq(SyncScheduler.nextSync, Math.floor(TIMESTAMP1 / 1000) * 1000);

  _("'syncInterval' defaults to singleDeviceInterval.");
  do_check_eq(Svc.Prefs.get('syncInterval'), undefined);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  _("'syncInterval' corresponds to a preference setting.");
  SyncScheduler.syncInterval = INTERVAL;
  do_check_eq(SyncScheduler.syncInterval, INTERVAL);
  do_check_eq(Svc.Prefs.get('syncInterval'), INTERVAL);

  _("'syncThreshold' corresponds to preference, defaults to SINGLE_USER_THRESHOLD");
  do_check_eq(Svc.Prefs.get('syncThreshold'), undefined);
  do_check_eq(SyncScheduler.syncThreshold, SINGLE_USER_THRESHOLD);
  SyncScheduler.syncThreshold = THRESHOLD;
  do_check_eq(SyncScheduler.syncThreshold, THRESHOLD);

  _("'globalScore' corresponds to preference, defaults to zero.");
  do_check_eq(Svc.Prefs.get('globalScore'), 0);
  do_check_eq(SyncScheduler.globalScore, 0);
  SyncScheduler.globalScore = SCORE;
  do_check_eq(SyncScheduler.globalScore, SCORE);
  do_check_eq(Svc.Prefs.get('globalScore'), SCORE);

  _("Intervals correspond to default preferences.");
  do_check_eq(SyncScheduler.singleDeviceInterval,
              Svc.Prefs.get("scheduler.singleDeviceInterval") * 1000);
  do_check_eq(SyncScheduler.idleInterval,
              Svc.Prefs.get("scheduler.idleInterval") * 1000);
  do_check_eq(SyncScheduler.activeInterval,
              Svc.Prefs.get("scheduler.activeInterval") * 1000);
  do_check_eq(SyncScheduler.immediateInterval,
              Svc.Prefs.get("scheduler.immediateInterval") * 1000);

  _("Custom values for prefs will take effect after a restart.");
  Svc.Prefs.set("scheduler.singleDeviceInterval", 42);
  Svc.Prefs.set("scheduler.idleInterval", 23);
  Svc.Prefs.set("scheduler.activeInterval", 18);
  Svc.Prefs.set("scheduler.immediateInterval", 31415);
  SyncScheduler.setDefaults();
  do_check_eq(SyncScheduler.idleInterval, 23000);
  do_check_eq(SyncScheduler.singleDeviceInterval, 42000);
  do_check_eq(SyncScheduler.activeInterval, 18000);
  do_check_eq(SyncScheduler.immediateInterval, 31415000);

  Svc.Prefs.resetBranch("");
  SyncScheduler.setDefaults();
  run_next_test();
});

add_test(function test_updateClientMode() {
  _("Test updateClientMode adjusts scheduling attributes based on # of clients appropriately");
  do_check_eq(SyncScheduler.syncThreshold, SINGLE_USER_THRESHOLD);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.idle);

  
  Clients._store.create({id: "foo", cleartext: "bar"});
  SyncScheduler.updateClientMode();

  do_check_eq(SyncScheduler.syncThreshold, MULTI_DEVICE_THRESHOLD);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.activeInterval);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.idle);

  
  Clients.resetClient();
  SyncScheduler.updateClientMode();

  
  do_check_eq(SyncScheduler.numClients, 1);
  do_check_eq(SyncScheduler.syncThreshold, SINGLE_USER_THRESHOLD);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.idle);

  cleanUpAndGo();
});

add_test(function test_masterpassword_locked_retry_interval() {
  _("Test Status.login = MASTER_PASSWORD_LOCKED results in reschedule at MASTER_PASSWORD interval");
  let loginFailed = false;
  Svc.Obs.add("weave:service:login:error", function onLoginError() {
    Svc.Obs.remove("weave:service:login:error", onLoginError); 
    loginFailed = true;
  });

  let rescheduleInterval = false;
  SyncScheduler._scheduleAtInterval = SyncScheduler.scheduleAtInterval;
  SyncScheduler.scheduleAtInterval = function (interval) {
    rescheduleInterval = true;
    do_check_eq(interval, MASTER_PASSWORD_LOCKED_RETRY_INTERVAL);
  };

  Service._verifyLogin = Service.verifyLogin;
  Service.verifyLogin = function () {
    Status.login = MASTER_PASSWORD_LOCKED;
    return false;
  };

  let server = sync_httpd_setup();
  setUp();

  Service.sync();

  do_check_true(loginFailed);
  do_check_eq(Status.login, MASTER_PASSWORD_LOCKED);
  do_check_true(rescheduleInterval);

  Service.verifyLogin = Service._verifyLogin;
  SyncScheduler.scheduleAtInterval = SyncScheduler._scheduleAtInterval;

  cleanUpAndGo(server);
});

add_test(function test_calculateBackoff() {
  do_check_eq(Status.backoffInterval, 0);

  
  
  Status.backoffInterval = 5;
  let backoffInterval = Utils.calculateBackoff(50, MAXIMUM_BACKOFF_INTERVAL);

  do_check_eq(backoffInterval, MAXIMUM_BACKOFF_INTERVAL);

  
  
  Status.backoffInterval = MAXIMUM_BACKOFF_INTERVAL + 10;
  backoffInterval = Utils.calculateBackoff(50, MAXIMUM_BACKOFF_INTERVAL);
  
  do_check_eq(backoffInterval, MAXIMUM_BACKOFF_INTERVAL + 10);

  cleanUpAndGo();
});

add_test(function test_scheduleNextSync_nowOrPast() {
  Svc.Obs.add("weave:service:sync:finish", function onSyncFinish() {
    Svc.Obs.remove("weave:service:sync:finish", onSyncFinish);
    cleanUpAndGo(server);
  });

  let server = sync_httpd_setup();
  setUp();

  
  SyncScheduler.scheduleNextSync(-1);
});

add_test(function test_scheduleNextSync_future_noBackoff() {
  _("scheduleNextSync() uses the current syncInterval if no interval is provided.");
  
  do_check_eq(Status.backoffInterval, 0);

  _("Test setting sync interval when nextSync == 0");
  SyncScheduler.nextSync = 0;
  SyncScheduler.scheduleNextSync();

  
  
  do_check_true(SyncScheduler.nextSync - Date.now()
                <= SyncScheduler.syncInterval);
  do_check_eq(SyncScheduler.syncTimer.delay, SyncScheduler.syncInterval);

  _("Test setting sync interval when nextSync != 0");
  SyncScheduler.nextSync = Date.now() + SyncScheduler.singleDeviceInterval;
  SyncScheduler.scheduleNextSync();

  
  
  do_check_true(SyncScheduler.nextSync - Date.now()
                <= SyncScheduler.syncInterval);
  do_check_true(SyncScheduler.syncTimer.delay <= SyncScheduler.syncInterval);

  _("Scheduling requests for intervals larger than the current one will be ignored.");
  
  
  let nextSync = SyncScheduler.nextSync;
  let timerDelay = SyncScheduler.syncTimer.delay;
  let requestedInterval = SyncScheduler.syncInterval * 10;
  SyncScheduler.scheduleNextSync(requestedInterval);
  do_check_eq(SyncScheduler.nextSync, nextSync);
  do_check_eq(SyncScheduler.syncTimer.delay, timerDelay);

  
  SyncScheduler.nextSync = 0;
  SyncScheduler.scheduleNextSync(requestedInterval);
  do_check_true(SyncScheduler.nextSync <= Date.now() + requestedInterval);
  do_check_eq(SyncScheduler.syncTimer.delay, requestedInterval);

  
  SyncScheduler.scheduleNextSync(1);
  do_check_true(SyncScheduler.nextSync <= Date.now() + 1);
  do_check_eq(SyncScheduler.syncTimer.delay, 1);

  cleanUpAndGo();
});

add_test(function test_scheduleNextSync_future_backoff() {
 _("scheduleNextSync() will honour backoff in all scheduling requests.");
  
  const BACKOFF = 7337;
  Status.backoffInterval = SyncScheduler.syncInterval + BACKOFF;

  _("Test setting sync interval when nextSync == 0");
  SyncScheduler.nextSync = 0;
  SyncScheduler.scheduleNextSync();

  
  
  do_check_true(SyncScheduler.nextSync - Date.now()
                <= Status.backoffInterval);
  do_check_eq(SyncScheduler.syncTimer.delay, Status.backoffInterval);

  _("Test setting sync interval when nextSync != 0");
  SyncScheduler.nextSync = Date.now() + SyncScheduler.singleDeviceInterval;
  SyncScheduler.scheduleNextSync();

  
  
  do_check_true(SyncScheduler.nextSync - Date.now()
                <= Status.backoffInterval);
  do_check_true(SyncScheduler.syncTimer.delay <= Status.backoffInterval);

  
  
  let nextSync = SyncScheduler.nextSync;
  let timerDelay = SyncScheduler.syncTimer.delay;
  let requestedInterval = SyncScheduler.syncInterval * 10;
  do_check_true(requestedInterval > Status.backoffInterval);
  SyncScheduler.scheduleNextSync(requestedInterval);
  do_check_eq(SyncScheduler.nextSync, nextSync);
  do_check_eq(SyncScheduler.syncTimer.delay, timerDelay);

  
  SyncScheduler.nextSync = 0;
  SyncScheduler.scheduleNextSync(requestedInterval);
  do_check_true(SyncScheduler.nextSync <= Date.now() + requestedInterval);
  do_check_eq(SyncScheduler.syncTimer.delay, requestedInterval);

  
  SyncScheduler.scheduleNextSync(1);
  do_check_true(SyncScheduler.nextSync <= Date.now() + Status.backoffInterval);
  do_check_eq(SyncScheduler.syncTimer.delay, Status.backoffInterval);

  cleanUpAndGo();
});

add_test(function test_handleSyncError() {
  let server = sync_httpd_setup();
  setUp();
 
  
  Svc.Prefs.set("firstSync", "notReady");

  _("Ensure expected initial environment.");
  do_check_eq(SyncScheduler._syncErrors, 0);
  do_check_false(Status.enforceBackoff);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  do_check_eq(Status.backoffInterval, 0);

  
  
  _("Test first error calls scheduleNextSync on default interval");
  Service.sync();
  do_check_true(SyncScheduler.nextSync <= Date.now() + SyncScheduler.singleDeviceInterval);
  do_check_eq(SyncScheduler.syncTimer.delay, SyncScheduler.singleDeviceInterval);
  do_check_eq(SyncScheduler._syncErrors, 1);
  do_check_false(Status.enforceBackoff);
  SyncScheduler.syncTimer.clear();

  _("Test second error still calls scheduleNextSync on default interval");
  Service.sync();
  do_check_true(SyncScheduler.nextSync <= Date.now() + SyncScheduler.singleDeviceInterval);
  do_check_eq(SyncScheduler.syncTimer.delay, SyncScheduler.singleDeviceInterval);
  do_check_eq(SyncScheduler._syncErrors, 2);
  do_check_false(Status.enforceBackoff);
  SyncScheduler.syncTimer.clear();

  _("Test third error sets Status.enforceBackoff and calls scheduleAtInterval");
  Service.sync();
  let maxInterval = SyncScheduler._syncErrors * (2 * MINIMUM_BACKOFF_INTERVAL);
  do_check_eq(Status.backoffInterval, 0);
  do_check_true(SyncScheduler.nextSync <= (Date.now() + maxInterval));
  do_check_true(SyncScheduler.syncTimer.delay <= maxInterval);
  do_check_eq(SyncScheduler._syncErrors, 3);
  do_check_true(Status.enforceBackoff);

  
  Status.resetBackoff();
  do_check_false(Status.enforceBackoff);
  do_check_eq(SyncScheduler._syncErrors, 3);
  SyncScheduler.syncTimer.clear();

  _("Test fourth error still calls scheduleAtInterval even if enforceBackoff was reset");
  Service.sync();
  maxInterval = SyncScheduler._syncErrors * (2 * MINIMUM_BACKOFF_INTERVAL);
  do_check_true(SyncScheduler.nextSync <= Date.now() + maxInterval);
  do_check_true(SyncScheduler.syncTimer.delay <= maxInterval);
  do_check_eq(SyncScheduler._syncErrors, 4);
  do_check_true(Status.enforceBackoff);
  SyncScheduler.syncTimer.clear();

  cleanUpAndGo(server);
});

add_test(function test_client_sync_finish_updateClientMode() {
  let server = sync_httpd_setup();
  setUp();

  
  do_check_eq(SyncScheduler.syncThreshold, SINGLE_USER_THRESHOLD);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  do_check_false(SyncScheduler.idle);

  
  Clients._store.create({id: "foo", cleartext: "bar"});
  do_check_false(SyncScheduler.numClients > 1);
  SyncScheduler.updateClientMode();
  Service.sync();

  do_check_eq(SyncScheduler.syncThreshold, MULTI_DEVICE_THRESHOLD);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.activeInterval);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.idle);

  
  Clients.resetClient();
  Service.sync();

  
  do_check_eq(SyncScheduler.numClients, 1);
  do_check_eq(SyncScheduler.syncThreshold, SINGLE_USER_THRESHOLD);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.idle);

  cleanUpAndGo(server);
});

add_test(function test_autoconnect_nextSync_past() {
  

  Svc.Obs.add("weave:service:sync:finish", function onSyncFinish() {
    Svc.Obs.remove("weave:service:sync:finish", onSyncFinish);
    cleanUpAndGo(server);
  });

  let server = sync_httpd_setup();
  setUp();

  SyncScheduler.delayedAutoConnect(0);
});

add_test(function test_autoconnect_nextSync_future() {
  let previousSync = Date.now() + SyncScheduler.syncInterval / 2;
  SyncScheduler.nextSync = previousSync;
  
  let expectedSync = SyncScheduler.nextSync;
  let expectedInterval = expectedSync - Date.now() - 1000;

  
  function onLoginStart() {
    do_throw("Should not get here!");
  }
  Svc.Obs.add("weave:service:login:start", onLoginStart);

  waitForZeroTimer(function () {
    do_check_eq(SyncScheduler.nextSync, expectedSync);
    do_check_true(SyncScheduler.syncTimer.delay >= expectedInterval);

    Svc.Obs.remove("weave:service:login:start", onLoginStart);
    cleanUpAndGo();
  });

  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  SyncScheduler.delayedAutoConnect(0);
});

add_test(function test_autoconnect_mp_locked() {
  let server = sync_httpd_setup();
  setUp();

  
  let origLocked = Utils.mpLocked;
  Utils.mpLocked = function() true;

  let origPP = Service.__lookupGetter__("passphrase");
  delete Service.passphrase;
  Service.__defineGetter__("passphrase", function() {
    _("Faking Master Password entry cancelation.");
    throw "User canceled Master Password entry";
  });

  
  
  Svc.Obs.add("weave:service:login:error", function onLoginError() {
    Svc.Obs.remove("weave:service:login:error", onLoginError);
    Utils.nextTick(function aLittleBitAfterLoginError() {
      do_check_eq(Status.login, MASTER_PASSWORD_LOCKED);

      Utils.mpLocked = origLocked;
      delete Service.passphrase;
      Service.__defineGetter__("passphrase", origPP);

      cleanUpAndGo(server);
    });
  });

  SyncScheduler.delayedAutoConnect(0);
});

add_test(function test_no_autoconnect_during_wizard() {
  let server = sync_httpd_setup();
  setUp();

  
  Svc.Prefs.set("firstSync", "notReady");

  
  function onLoginStart() {
    do_throw("Should not get here!");
  }
  Svc.Obs.add("weave:service:login:start", onLoginStart);

  waitForZeroTimer(function () {
    Svc.Obs.remove("weave:service:login:start", onLoginStart);
    cleanUpAndGo(server);
  });

  SyncScheduler.delayedAutoConnect(0);
});

add_test(function test_no_autoconnect_status_not_ok() {
  let server = sync_httpd_setup();

  
  function onLoginStart() {
    do_throw("Should not get here!");
  }
  Svc.Obs.add("weave:service:login:start", onLoginStart);

  waitForZeroTimer(function () {
    Svc.Obs.remove("weave:service:login:start", onLoginStart);

    do_check_eq(Status.service, CLIENT_NOT_CONFIGURED);
    do_check_eq(Status.login, LOGIN_FAILED_NO_USERNAME);

    cleanUpAndGo(server);
  });

  SyncScheduler.delayedAutoConnect(0);
});

add_test(function test_autoconnectDelay_pref() {
  Svc.Obs.add("weave:service:sync:finish", function onSyncFinish() {
    Svc.Obs.remove("weave:service:sync:finish", onSyncFinish);
    cleanUpAndGo(server);
  });

  Svc.Prefs.set("autoconnectDelay", 1);

  let server = sync_httpd_setup();
  setUp();

  Svc.Obs.notify("weave:service:ready");

  
  do_check_eq(SyncScheduler._autoTimer.delay, 1000);
  do_check_eq(Status.service, STATUS_OK);
});

add_test(function test_idle_adjustSyncInterval() {
  
  do_check_eq(SyncScheduler.idle, false);

  
  SyncScheduler.observe(null, "idle", Svc.Prefs.get("scheduler.idleTime"));
  do_check_eq(SyncScheduler.idle, true);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  
  SyncScheduler.idle = false;
  Clients._store.create({id: "foo", cleartext: "bar"});
  SyncScheduler.updateClientMode();
  SyncScheduler.observe(null, "idle", Svc.Prefs.get("scheduler.idleTime"));
  do_check_eq(SyncScheduler.idle, true);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.idleInterval);

  cleanUpAndGo();
});

add_test(function test_back_triggersSync() {
  
  do_check_false(SyncScheduler.idle);
  do_check_eq(Status.backoffInterval, 0);

  
  SyncScheduler.numClients = 2;
  SyncScheduler.observe(null, "idle", Svc.Prefs.get("scheduler.idleTime"));
  do_check_true(SyncScheduler.idle);

  
  
  Svc.Obs.add("weave:service:login:error", function onLoginError() {
    Svc.Obs.remove("weave:service:login:error", onLoginError);
    cleanUpAndGo();
  });

  
  SyncScheduler.observe(null, "back", Svc.Prefs.get("scheduler.idleTime"));
});

add_test(function test_back_triggersSync_observesBackoff() {
  
  do_check_false(SyncScheduler.idle);

  
  const BACKOFF = 7337;
  Status.backoffInterval = SyncScheduler.idleInterval + BACKOFF;
  SyncScheduler.numClients = 2;
  SyncScheduler.observe(null, "idle", Svc.Prefs.get("scheduler.idleTime"));
  do_check_eq(SyncScheduler.idle, true);

  function onLoginStart() {
    do_throw("Shouldn't have kicked off a sync!");
  }
  Svc.Obs.add("weave:service:login:start", onLoginStart);

  timer = Utils.namedTimer(function () {
    Svc.Obs.remove("weave:service:login:start", onLoginStart);

    do_check_true(SyncScheduler.nextSync <= Date.now() + Status.backoffInterval);
    do_check_eq(SyncScheduler.syncTimer.delay, Status.backoffInterval);

    cleanUpAndGo();
  }, IDLE_OBSERVER_BACK_DELAY * 1.5, {}, "timer");

  
  SyncScheduler.observe(null, "back", Svc.Prefs.get("scheduler.idleTime"));
});

add_test(function test_back_debouncing() {
  _("Ensure spurious back-then-idle events, as observed on OS X, don't trigger a sync.");

  
  do_check_eq(SyncScheduler.idle, false);

  
  SyncScheduler.numClients = 2;
  SyncScheduler.observe(null, "idle", Svc.Prefs.get("scheduler.idleTime"));
  do_check_eq(SyncScheduler.idle, true);

  function onLoginStart() {
    do_throw("Shouldn't have kicked off a sync!");
  }
  Svc.Obs.add("weave:service:login:start", onLoginStart);

  
  SyncScheduler.observe(null, "back", Svc.Prefs.get("scheduler.idleTime"));
  SyncScheduler.observe(null, "idle", Svc.Prefs.get("scheduler.idleTime"));

  timer = Utils.namedTimer(function () {
    Svc.Obs.remove("weave:service:login:start", onLoginStart);
    cleanUpAndGo();
  }, IDLE_OBSERVER_BACK_DELAY * 1.5, {}, "timer");
});

add_test(function test_no_sync_node() {
  
  
  let server = sync_httpd_setup();
  setUp();

  Service.serverURL = "http://localhost:8080/";

  Service.sync();
  do_check_eq(Status.sync, NO_SYNC_NODE_FOUND);
  do_check_eq(SyncScheduler.syncTimer.delay, NO_SYNC_NODE_INTERVAL);

  cleanUpAndGo(server);
});

add_test(function test_sync_failed_partial_500s() {
  _("Test a 5xx status calls handleSyncError.");
  SyncScheduler._syncErrors = MAX_ERROR_COUNT_BEFORE_BACKOFF;
  let server = sync_httpd_setup();

  let engine = Engines.get("catapult");
  engine.enabled = true;
  engine.exception = {status: 500};

  do_check_eq(Status.sync, SYNC_SUCCEEDED);

  do_check_true(setUp());

  Service.sync();

  do_check_eq(Status.service, SYNC_FAILED_PARTIAL);

  let maxInterval = SyncScheduler._syncErrors * (2 * MINIMUM_BACKOFF_INTERVAL);
  do_check_eq(Status.backoffInterval, 0);
  do_check_true(Status.enforceBackoff);
  do_check_eq(SyncScheduler._syncErrors, 4);
  do_check_true(SyncScheduler.nextSync <= (Date.now() + maxInterval));
  do_check_true(SyncScheduler.syncTimer.delay <= maxInterval);

  cleanUpAndGo(server);
});

add_test(function test_sync_failed_partial_400s() {
  _("Test a non-5xx status doesn't call handleSyncError.");
  SyncScheduler._syncErrors = MAX_ERROR_COUNT_BEFORE_BACKOFF;
  let server = sync_httpd_setup();

  let engine = Engines.get("catapult");
  engine.enabled = true;
  engine.exception = {status: 400};

  
  Clients._store.create({id: "foo", cleartext: "bar"});

  do_check_eq(Status.sync, SYNC_SUCCEEDED);

  do_check_true(setUp());

  Service.sync();

  do_check_eq(Status.service, SYNC_FAILED_PARTIAL);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.activeInterval);

  do_check_eq(Status.backoffInterval, 0);
  do_check_false(Status.enforceBackoff);
  do_check_eq(SyncScheduler._syncErrors, 0);
  do_check_true(SyncScheduler.nextSync <= (Date.now() + SyncScheduler.activeInterval));
  do_check_true(SyncScheduler.syncTimer.delay <= SyncScheduler.activeInterval);

  cleanUpAndGo(server);
});

add_test(function test_sync_X_Weave_Backoff() {
  let server = sync_httpd_setup();
  setUp();

  
  
  const BACKOFF = 7337;

  
  const INFO_COLLECTIONS = "/1.1/johndoe/info/collections";
  let infoColl = server._handler._overridePaths[INFO_COLLECTIONS];
  let serverBackoff = false;
  function infoCollWithBackoff(request, response) {
    if (serverBackoff) {
      response.setHeader("X-Weave-Backoff", "" + BACKOFF);
    }
    infoColl(request, response);
  }
  server.registerPathHandler(INFO_COLLECTIONS, infoCollWithBackoff);

  
  
  Clients._store.create({id: "foo", cleartext: "bar"});
  let rec = Clients._store.createRecord("foo", "clients");
  rec.encrypt();
  rec.upload(Clients.engineURL + rec.id);

  
  
  Service.sync();
  do_check_eq(Status.backoffInterval, 0);
  do_check_eq(Status.minimumNextSync, 0);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.activeInterval);
  do_check_true(SyncScheduler.nextSync <=
                Date.now() + SyncScheduler.syncInterval);
  
  do_check_true(SyncScheduler.syncInterval < BACKOFF * 1000);

  
  serverBackoff = true;
  Service.sync();

  do_check_true(Status.backoffInterval >= BACKOFF * 1000);
  
  
  let minimumExpectedDelay = (BACKOFF - 1) * 1000;
  do_check_true(Status.minimumNextSync >= Date.now() + minimumExpectedDelay);

  
  do_check_true(SyncScheduler.nextSync >= Date.now() + minimumExpectedDelay);
  do_check_true(SyncScheduler.syncTimer.delay >= minimumExpectedDelay);

  cleanUpAndGo(server);
});

add_test(function test_sync_503_Retry_After() {
  let server = sync_httpd_setup();
  setUp();

  
  
  const BACKOFF = 7337;

  
  const INFO_COLLECTIONS = "/1.1/johndoe/info/collections";
  let infoColl = server._handler._overridePaths[INFO_COLLECTIONS];
  let serverMaintenance = false;
  function infoCollWithMaintenance(request, response) {
    if (!serverMaintenance) {
      infoColl(request, response);
      return;
    }
    response.setHeader("Retry-After", "" + BACKOFF);
    response.setStatusLine(request.httpVersion, 503, "Service Unavailable");
  }
  server.registerPathHandler(INFO_COLLECTIONS, infoCollWithMaintenance);

  
  
  Clients._store.create({id: "foo", cleartext: "bar"});
  let rec = Clients._store.createRecord("foo", "clients");
  rec.encrypt();
  rec.upload(Clients.engineURL + rec.id);

  
  
  Service.sync();
  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.backoffInterval, 0);
  do_check_eq(Status.minimumNextSync, 0);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.activeInterval);
  do_check_true(SyncScheduler.nextSync <=
                Date.now() + SyncScheduler.syncInterval);
  
  do_check_true(SyncScheduler.syncInterval < BACKOFF * 1000);

  
  serverMaintenance = true;
  Service.sync();

  do_check_true(Status.enforceBackoff);
  do_check_true(Status.backoffInterval >= BACKOFF * 1000);
  
  
  let minimumExpectedDelay = (BACKOFF - 1) * 1000;
  do_check_true(Status.minimumNextSync >= Date.now() + minimumExpectedDelay);

  
  do_check_true(SyncScheduler.nextSync >= Date.now() + minimumExpectedDelay);
  do_check_true(SyncScheduler.syncTimer.delay >= minimumExpectedDelay);

  cleanUpAndGo(server);
});

add_test(function test_loginError_recoverable_reschedules() {
  _("Verify that a recoverable login error schedules a new sync.");
  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.clusterURL = "http://localhost:8080/";
  Service.persistLogin();
  Status.resetSync(); 

  Svc.Obs.add("weave:service:login:error", function onLoginError() {
    Svc.Obs.remove("weave:service:login:error", onLoginError);
    Utils.nextTick(function aLittleBitAfterLoginError() {
      do_check_eq(Status.login, LOGIN_FAILED_NETWORK_ERROR);

      let expectedNextSync = Date.now() + SyncScheduler.syncInterval;
      do_check_true(SyncScheduler.nextSync > Date.now());
      do_check_true(SyncScheduler.nextSync <= expectedNextSync);
      do_check_true(SyncScheduler.syncTimer.delay > 0);
      do_check_true(SyncScheduler.syncTimer.delay <= SyncScheduler.syncInterval);

      Svc.Obs.remove("weave:service:sync:start", onSyncStart);
      cleanUpAndGo();
    });
  });

  
  
  
  SyncScheduler.nextSync = Date.now() - 100000;
  SyncScheduler.globalScore = SINGLE_USER_THRESHOLD + 1;
  function onSyncStart() {
    do_throw("Shouldn't have started a sync!");
  }
  Svc.Obs.add("weave:service:sync:start", onSyncStart);

  
  do_check_eq(SyncScheduler.syncTimer, null);
  do_check_eq(Status.checkSetup(), STATUS_OK);
  do_check_eq(Status.login, LOGIN_SUCCEEDED);

  SyncScheduler.scheduleNextSync(0);
});

add_test(function test_loginError_fatal_clearsTriggers() {
  _("Verify that a fatal login error clears sync triggers.");
  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.clusterURL = "http://localhost:8080/";
  Service.persistLogin();
  Status.resetSync(); 

  let server = httpd_setup({
    "/1.1/johndoe/info/collections": httpd_handler(401, "Unauthorized")
  });

  Svc.Obs.add("weave:service:login:error", function onLoginError() {
    Svc.Obs.remove("weave:service:login:error", onLoginError);
    Utils.nextTick(function aLittleBitAfterLoginError() {
      do_check_eq(Status.login, LOGIN_FAILED_LOGIN_REJECTED);

      do_check_eq(SyncScheduler.nextSync, 0);
      do_check_eq(SyncScheduler.syncTimer, null);

      cleanUpAndGo(server);
    });
  });

  
  do_check_eq(SyncScheduler.nextSync, 0);
  do_check_eq(SyncScheduler.syncTimer, null);
  do_check_eq(Status.checkSetup(), STATUS_OK);
  do_check_eq(Status.login, LOGIN_SUCCEEDED);

  SyncScheduler.scheduleNextSync(0);
});
