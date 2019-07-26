


Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services/sync/utils.js");

Svc.DefaultPrefs.set("registerEngines", "");
Cu.import("resource://services-sync/service.js");

let scheduler = Service.scheduler;
let clientsEngine = Service.clientsEngine;

function sync_httpd_setup() {
  let global = new ServerWBO("global", {
    syncID: Service.syncID,
    storageVersion: STORAGE_VERSION,
    engines: {clients: {version: clientsEngine.version,
                        syncID: clientsEngine.syncID}}
  });
  let clientsColl = new ServerCollection({}, true);

  
  let collectionsHelper = track_collections_helper();
  let upd = collectionsHelper.with_updated_collection;

  return httpd_setup({
    "/1.1/johndoe/storage/meta/global": upd("meta", global.handler()),
    "/1.1/johndoe/info/collections": collectionsHelper.handler,
    "/1.1/johndoe/storage/crypto/keys":
      upd("crypto", (new ServerWBO("keys")).handler()),
    "/1.1/johndoe/storage/clients": upd("clients", clientsColl.handler())
  });
}

function setUp(server) {
  setBasicCredentials("johndoe", "ilovejane", "abcdeabcdeabcdeabcdeabcdea");
  Service.serverURL = server.baseURI + "/";
  Service.clusterURL = server.baseURI + "/";

  generateNewKeys(Service.collectionKeys);
  let serverKeys = Service.collectionKeys.asWBO("crypto", "keys");
  serverKeys.encrypt(Service.identity.syncKeyBundle);
  return serverKeys.upload(Service.resource(Service.cryptoKeysURL));
}

function run_test() {
  initTestLogging("Trace");

  Log.repository.getLogger("Sync.Service").level = Log.Level.Trace;
  Log.repository.getLogger("Sync.SyncScheduler").level = Log.Level.Trace;

  run_next_test();
}

add_test(function test_successful_sync_adjustSyncInterval() {
  _("Test successful sync calling adjustSyncInterval");
  let syncSuccesses = 0;
  function onSyncFinish() {
    _("Sync success.");
    syncSuccesses++;
  };
  Svc.Obs.add("weave:service:sync:finish", onSyncFinish);

  let server = sync_httpd_setup();
  setUp(server);

  
  do_check_false(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);
  do_check_false(scheduler.hasIncomingItems);

  _("Test as long as numClients <= 1 our sync interval is SINGLE_USER.");
  
  scheduler.idle = true;
  Service.sync();
  do_check_eq(syncSuccesses, 1);
  do_check_true(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  
  scheduler.idle = false;
  Service.sync();
  do_check_eq(syncSuccesses, 2);
  do_check_false(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  
  scheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncSuccesses, 3);
  do_check_false(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_true(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  
  scheduler.idle = true;
  Service.sync();
  do_check_eq(syncSuccesses, 4);
  do_check_true(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_true(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  _("Test as long as idle && numClients > 1 our sync interval is idleInterval.");
  
  Service.clientsEngine._store.create({id: "foo", cleartext: "bar"});
  Service.sync();
  do_check_eq(syncSuccesses, 5);
  do_check_true(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_true(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.idleInterval);

  
  scheduler.hasIncomingItems = false;
  Service.sync();
  do_check_eq(syncSuccesses, 6);
  do_check_true(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.idleInterval);

  _("Test non-idle, numClients > 1, no incoming items => activeInterval.");
  
  scheduler.idle = false;
  Service.sync();
  do_check_eq(syncSuccesses, 7);
  do_check_false(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.activeInterval);

  _("Test non-idle, numClients > 1, incoming items => immediateInterval.");
  
  scheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncSuccesses, 8);
  do_check_false(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems); 
  do_check_eq(scheduler.syncInterval, scheduler.immediateInterval);

  Svc.Obs.remove("weave:service:sync:finish", onSyncFinish);
  Service.startOver();
  server.stop(run_next_test);
});

add_test(function test_unsuccessful_sync_adjustSyncInterval() {
  _("Test unsuccessful sync calling adjustSyncInterval");

  let syncFailures = 0;
  function onSyncError() {
    _("Sync error.");
    syncFailures++;
  }
  Svc.Obs.add("weave:service:sync:error", onSyncError);

  _("Test unsuccessful sync calls adjustSyncInterval");
  
  Svc.Prefs.set("firstSync", "notReady");

  let server = sync_httpd_setup();
  setUp(server);

  
  do_check_false(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);
  do_check_false(scheduler.hasIncomingItems);

  _("Test as long as numClients <= 1 our sync interval is SINGLE_USER.");
  
  scheduler.idle = true;
  Service.sync();
  do_check_eq(syncFailures, 1);
  do_check_true(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  
  scheduler.idle = false;
  Service.sync();
  do_check_eq(syncFailures, 2);
  do_check_false(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  
  scheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncFailures, 3);
  do_check_false(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_true(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  
  scheduler.idle = true;
  Service.sync();
  do_check_eq(syncFailures, 4);
  do_check_true(scheduler.idle);
  do_check_false(scheduler.numClients > 1);
  do_check_true(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  _("Test as long as idle && numClients > 1 our sync interval is idleInterval.");
  
  Service.clientsEngine._store.create({id: "foo", cleartext: "bar"});

  Service.sync();
  do_check_eq(syncFailures, 5);
  do_check_true(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_true(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.idleInterval);

  
  scheduler.hasIncomingItems = false;
  Service.sync();
  do_check_eq(syncFailures, 6);
  do_check_true(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.idleInterval);

  _("Test non-idle, numClients > 1, no incoming items => activeInterval.");
  
  scheduler.idle = false;
  Service.sync();
  do_check_eq(syncFailures, 7);
  do_check_false(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems);
  do_check_eq(scheduler.syncInterval, scheduler.activeInterval);

  _("Test non-idle, numClients > 1, incoming items => immediateInterval.");
  
  scheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncFailures, 8);
  do_check_false(scheduler.idle);
  do_check_true(scheduler.numClients > 1);
  do_check_false(scheduler.hasIncomingItems); 
  do_check_eq(scheduler.syncInterval, scheduler.immediateInterval);

  Service.startOver();
  Svc.Obs.remove("weave:service:sync:error", onSyncError);
  server.stop(run_next_test);
});

add_test(function test_back_triggers_sync() {
  let server = sync_httpd_setup();
  setUp(server);

  
  scheduler.idle = true;
  scheduler.observe(null, "back", Svc.Prefs.get("scheduler.idleTime"));
  do_check_false(scheduler.idle);

  
  clientsEngine._store.create({id: "foo", cleartext: "bar"});
  scheduler.updateClientMode();

  Svc.Obs.add("weave:service:sync:finish", function onSyncFinish() {
    Svc.Obs.remove("weave:service:sync:finish", onSyncFinish);

    Service.recordManager.clearCache();
    Svc.Prefs.resetBranch("");
    scheduler.setDefaults();
    clientsEngine.resetClient();

    Service.startOver();
    server.stop(run_next_test);
  });

  scheduler.idle = true;
  scheduler.observe(null, "back", Svc.Prefs.get("scheduler.idleTime"));
  do_check_false(scheduler.idle);
});

add_test(function test_adjust_interval_on_sync_error() {
  let server = sync_httpd_setup();
  setUp(server);

  let syncFailures = 0;
  function onSyncError() {
    _("Sync error.");
    syncFailures++;
  }
  Svc.Obs.add("weave:service:sync:error", onSyncError);

  _("Test unsuccessful sync updates client mode & sync intervals");
  
  Svc.Prefs.set("firstSync", "notReady");

  do_check_eq(syncFailures, 0);
  do_check_false(scheduler.numClients > 1);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  clientsEngine._store.create({id: "foo", cleartext: "bar"});
  Service.sync();

  do_check_eq(syncFailures, 1);
  do_check_true(scheduler.numClients > 1);
  do_check_eq(scheduler.syncInterval, scheduler.activeInterval);

  Svc.Obs.remove("weave:service:sync:error", onSyncError);
  Service.startOver();
  server.stop(run_next_test);
});

add_test(function test_bug671378_scenario() {
  
  
  
  
  
  let server = sync_httpd_setup();
  setUp(server);

  let syncSuccesses = 0;
  function onSyncFinish() {
    _("Sync success.");
    syncSuccesses++;
  };
  Svc.Obs.add("weave:service:sync:finish", onSyncFinish);

  
  Service.sync();
  do_check_eq(syncSuccesses, 1);
  do_check_false(scheduler.numClients > 1);
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);
  do_check_eq(scheduler.syncTimer.delay, scheduler.singleDeviceInterval);

  
  scheduler._scheduleNextSync = scheduler.scheduleNextSync;
  scheduler.scheduleNextSync = function() {
    scheduler._scheduleNextSync();

    
    
    if (syncSuccesses == 2) {
      do_check_neq(scheduler.nextSync, 0);
      do_check_eq(scheduler.syncInterval, scheduler.activeInterval);
      do_check_true(scheduler.syncTimer.delay <= scheduler.activeInterval);

      scheduler.scheduleNextSync = scheduler._scheduleNextSync;
      Svc.Obs.remove("weave:service:sync:finish", onSyncFinish);
      Service.startOver();
      server.stop(run_next_test);
    }
  };

  
  
  
  
  Svc.Obs.add("weave:service:sync:start", function onSyncStart() {
    
    
    Utils.nextTick(function() {
      Svc.Obs.remove("weave:service:sync:start", onSyncStart);

      scheduler.scheduleNextSync();
      do_check_neq(scheduler.nextSync, 0);
      do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);
      do_check_eq(scheduler.syncTimer.delay, scheduler.singleDeviceInterval);
    });
  });

  clientsEngine._store.create({id: "foo", cleartext: "bar"});
  Service.sync();
});

add_test(function test_adjust_timer_larger_syncInterval() {
  _("Test syncInterval > current timout period && nextSync != 0, syncInterval is NOT used.");
  clientsEngine._store.create({id: "foo", cleartext: "bar"});
  scheduler.updateClientMode();
  do_check_eq(scheduler.syncInterval, scheduler.activeInterval);

  scheduler.scheduleNextSync();

  
  do_check_neq(scheduler.nextSync, 0);
  do_check_eq(scheduler.syncTimer.delay, scheduler.activeInterval);

  
  clientsEngine._wipeClient();
  scheduler.updateClientMode();
  do_check_eq(scheduler.syncInterval, scheduler.singleDeviceInterval);

  scheduler.scheduleNextSync();

  
  do_check_neq(scheduler.nextSync, 0);
  do_check_true(scheduler.syncTimer.delay <= scheduler.activeInterval);

  
  Service.startOver();
  run_next_test();
});

add_test(function test_adjust_timer_smaller_syncInterval() {
  _("Test current timout > syncInterval period && nextSync != 0, syncInterval is used.");
  scheduler.scheduleNextSync();

  
  do_check_neq(scheduler.nextSync, 0);
  do_check_eq(scheduler.syncTimer.delay, scheduler.singleDeviceInterval);

  
  clientsEngine._store.create({id: "foo", cleartext: "bar"});
  scheduler.updateClientMode();
  do_check_eq(scheduler.syncInterval, scheduler.activeInterval);

  scheduler.scheduleNextSync();

  
  do_check_neq(scheduler.nextSync, 0);
  do_check_true(scheduler.syncTimer.delay <= scheduler.activeInterval);

  
  Service.startOver();
  run_next_test();
});
