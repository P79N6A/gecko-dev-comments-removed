


Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/policies.js");

Svc.DefaultPrefs.set("registerEngines", "");
Cu.import("resource://services-sync/service.js");

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
    "/1.1/johndoe/storage/clients": upd("clients", clientsColl.handler())
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
  return serverKeys.upload(Service.cryptoKeysURL);
}

function run_test() {
  initTestLogging("Trace");

  Log4Moz.repository.getLogger("Sync.Service").level = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.SyncScheduler").level = Log4Moz.Level.Trace;

  run_next_test();
}

add_test(function test_successful_sync_adjustSyncInterval() {
  _("Test successful sync calling adjustSyncInterval");
  let syncSuccesses = 0;
  Svc.Obs.add("weave:service:sync:finish", function onSyncFinish() {
    _("Sync success.");
    syncSuccesses++;
  });

  let server = sync_httpd_setup();
  setUp();

  
  do_check_false(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  do_check_false(SyncScheduler.hasIncomingItems);

  _("Test as long as numClients <= 1 our sync interval is SINGLE_USER."); 
  
  SyncScheduler.idle = true;
  Service.sync();
  do_check_eq(syncSuccesses, 1);
  do_check_true(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  
  
  SyncScheduler.idle = false;
  Service.sync();
  do_check_eq(syncSuccesses, 2);
  do_check_false(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  
  SyncScheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncSuccesses, 3);
  do_check_false(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_true(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  
  SyncScheduler.idle = true;
  Service.sync();
  do_check_eq(syncSuccesses, 4);
  do_check_true(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_true(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  _("Test as long as idle && numClients > 1 our sync interval is idleInterval.");
  
  Clients._store.create({id: "foo", cleartext: "bar"});
  Service.sync();
  do_check_eq(syncSuccesses, 5);
  do_check_true(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_true(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.idleInterval);

  
  SyncScheduler.hasIncomingItems = false;
  Service.sync();
  do_check_eq(syncSuccesses, 6);
  do_check_true(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.idleInterval);

  _("Test non-idle, numClients > 1, no incoming items => activeInterval.");
  
  SyncScheduler.idle = false;
  Service.sync();
  do_check_eq(syncSuccesses, 7);
  do_check_false(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.activeInterval);
  
  _("Test non-idle, numClients > 1, incoming items => immediateInterval.");
  
  SyncScheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncSuccesses, 8);
  do_check_false(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems); 
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.immediateInterval);

  Records.clearCache();
  Svc.Prefs.resetBranch("");
  SyncScheduler.setDefaults();
  Clients.resetClient();

  server.stop(run_next_test);
});

add_test(function test_unsuccessful_sync_adjustSyncInterval() {
  _("Test unsuccessful sync calling adjustSyncInterval");

  let syncFailures = 0;
  Svc.Obs.add("weave:service:sync:error", function onSyncError() {
    _("Sync error.");
    syncFailures++;
  });
    
  _("Test unsuccessful sync calls adjustSyncInterval");
  let origLockedSync = Service._lockedSync;
  Service._lockedSync = function () {
    
    Service._loggedIn = false;
    origLockedSync.call(Service);
  };
  
  let server = sync_httpd_setup();
  setUp();

  
  do_check_false(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  do_check_false(SyncScheduler.hasIncomingItems);

  _("Test as long as numClients <= 1 our sync interval is SINGLE_USER."); 
  
  SyncScheduler.idle = true;
  Service.sync();
  do_check_eq(syncFailures, 1);
  do_check_true(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  
  
  SyncScheduler.idle = false;
  Service.sync();
  do_check_eq(syncFailures, 2);
  do_check_false(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  
  SyncScheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncFailures, 3);
  do_check_false(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_true(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);

  
  SyncScheduler.idle = true;
  Service.sync();
  do_check_eq(syncFailures, 4);
  do_check_true(SyncScheduler.idle);
  do_check_false(SyncScheduler.numClients > 1);
  do_check_true(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.singleDeviceInterval);
  
  _("Test as long as idle && numClients > 1 our sync interval is idleInterval.");
  
  
  
  
  Clients._store.create({id: "foo", cleartext: "bar"});
  SyncScheduler.updateClientMode();  

  Service.sync();
  do_check_eq(syncFailures, 5);
  do_check_true(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_true(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.idleInterval);

  
  SyncScheduler.hasIncomingItems = false;
  Service.sync();
  do_check_eq(syncFailures, 6);
  do_check_true(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.idleInterval);

  _("Test non-idle, numClients > 1, no incoming items => activeInterval.");
  
  SyncScheduler.idle = false;
  Service.sync();
  do_check_eq(syncFailures, 7);
  do_check_false(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems);
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.activeInterval);
  
  _("Test non-idle, numClients > 1, incoming items => immediateInterval.");
  
  SyncScheduler.hasIncomingItems = true;
  Service.sync();
  do_check_eq(syncFailures, 8);
  do_check_false(SyncScheduler.idle);
  do_check_true(SyncScheduler.numClients > 1);
  do_check_false(SyncScheduler.hasIncomingItems); 
  do_check_eq(SyncScheduler.syncInterval, SyncScheduler.immediateInterval);

  Records.clearCache();
  Svc.Prefs.resetBranch("");
  SyncScheduler.setDefaults();
  Clients.resetClient();
  Service._lockedSync = origLockedSync;

  server.stop(run_next_test);
});

add_test(function test_back_triggers_sync() {
  let server = sync_httpd_setup();
  setUp();

  
  SyncScheduler.idle = true;
  SyncScheduler.observe(null, "back", Svc.Prefs.get("scheduler.idleTime"));
  do_check_false(SyncScheduler.idle);

  
  Clients._store.create({id: "foo", cleartext: "bar"});
  SyncScheduler.updateClientMode();

  Svc.Obs.add("weave:service:sync:finish", function onSyncFinish() {
    Svc.Obs.remove("weave:service:sync:finish", onSyncFinish);

    Records.clearCache();
    Svc.Prefs.resetBranch("");
    SyncScheduler.setDefaults();
    Clients.resetClient();

    server.stop(run_next_test);
  });

  SyncScheduler.idle = true;
  SyncScheduler.observe(null, "back", Svc.Prefs.get("scheduler.idleTime"));
  do_check_false(SyncScheduler.idle);
});
