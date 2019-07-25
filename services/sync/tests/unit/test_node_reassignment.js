


_("Test that node reassignment responses are respected on all kinds of " +
  "requests.");


Svc.DefaultPrefs.set("registerEngines", "")

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/rest.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/log4moz.js");

function run_test() {
  Log4Moz.repository.getLogger("Sync.AsyncResource").level = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.ErrorHandler").level  = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.Resource").level      = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.RESTRequest").level   = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.Service").level       = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.SyncScheduler").level = Log4Moz.Level.Trace;
  initTestLogging();

  Engines.register(RotaryEngine);

  
  function onUIError() {
    do_throw("Errors should not be presented in the UI.");
  }
  Svc.Obs.add("weave:ui:login:error", onUIError);
  Svc.Obs.add("weave:ui:sync:error", onUIError);

  run_next_test();
}











const reassignBody = "\"server request: node reassignment\"";



function handleReassign(handler, req, resp) {
  resp.setStatusLine(req.httpVersion, 401, "Node reassignment");
  resp.setHeader("Content-Type", "application/json");
  resp.bodyOutputStream.write(reassignBody, reassignBody.length);
}




const newNodeBody = "http://localhost:8080/";
function installNodeHandler(server, next) {
  function handleNodeRequest(req, resp) {
    _("Client made a request for a node reassignment.");
    resp.setStatusLine(req.httpVersion, 200, "OK");
    resp.setHeader("Content-Type", "text/plain");
    resp.bodyOutputStream.write(newNodeBody, newNodeBody.length);
    Utils.nextTick(next);
  }
  let nodePath = "/user/1.0/johndoe/node/weave";
  server.server.registerPathHandler(nodePath, handleNodeRequest);
  _("Registered node handler at " + nodePath);
}

function prepareServer() {
  Service.username   = "johndoe";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.password   = "ilovejane";
  Service.serverURL  = TEST_SERVER_URL;
  Service.clusterURL = TEST_CLUSTER_URL;

  do_check_eq(Service.userAPI, "http://localhost:8080/user/1.0/");
  let server = new SyncServer();
  server.registerUser("johndoe");
  server.start();
  return server;
}

function getReassigned() {
  try {
    return Services.prefs.getBoolPref("services.sync.lastSyncReassigned");
  } catch (ex if (ex.result == Cr.NS_ERROR_UNEXPECTED)) {
    return false;
  } catch (ex) {
    do_throw("Got exception retrieving lastSyncReassigned: " +
             Utils.exceptionStr(ex));
  }
}







function syncAndExpectNodeReassignment(server, firstNotification, between,
                                       secondNotification, url) {
  function onwards() {
    let nodeFetched = false;
    function onFirstSync() {
      _("First sync completed.");
      Svc.Obs.remove(firstNotification, onFirstSync);
      Svc.Obs.add(secondNotification, onSecondSync);

      do_check_eq(Service.clusterURL, "");

      
      
      
      nodeFetched = false;

      
      
      installNodeHandler(server, function () {
        nodeFetched = true;
      });

      
      between();
    }
    function onSecondSync() {
      _("Second sync completed.");
      Svc.Obs.remove(secondNotification, onSecondSync);
      SyncScheduler.clearSyncTriggers();

      
      
      waitForZeroTimer(function () {
        _("Second sync nextTick.");
        do_check_true(nodeFetched);
        Service.startOver();
        server.stop(run_next_test);
      });
    }

    Svc.Obs.add(firstNotification, onFirstSync);
    Service.sync();
  }

  
  let request = new RESTRequest(url);
  request.get(function () {
    do_check_eq(request.response.status, 401);
    Utils.nextTick(onwards);
  });
}

add_test(function test_momentary_401_engine() {
  _("Test a failure for engine URLs that's resolved by reassignment.");
  let server = prepareServer();
  let john   = server.user("johndoe");

  _("Enabling the Rotary engine.");
  let engine = Engines.get("rotary");
  engine.enabled = true;

  
  
  let global = {syncID: Service.syncID,
                storageVersion: STORAGE_VERSION,
                rotary: {version: engine.version,
                         syncID:  engine.syncID}}
  john.createCollection("meta").insert("global", global);

  _("First sync to prepare server contents.");
  Service.sync();

  _("Setting up Rotary collection to 401.");
  let rotary = john.createCollection("rotary");
  let oldHandler = rotary.collectionHandler;
  rotary.collectionHandler = handleReassign.bind(this, undefined);

  
  
  john.collection("rotary").timestamp += 1000;

  function between() {
    _("Undoing test changes.");
    rotary.collectionHandler = oldHandler;

    function onLoginStart() {
      
      _("Ensuring that lastSyncReassigned is still set at next sync start.");
      Svc.Obs.remove("weave:service:login:start", onLoginStart);
      do_check_true(getReassigned());
    }

    _("Adding observer that lastSyncReassigned is still set on login.");
    Svc.Obs.add("weave:service:login:start", onLoginStart);
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:sync:finish",
                                between,
                                "weave:service:sync:finish",
                                Service.storageURL + "rotary");
});


add_test(function test_momentary_401_info_collections() {
  _("Test a failure for info/collections that's resolved by reassignment.");
  let server = prepareServer();

  _("First sync to prepare server contents.");
  Service.sync();

  
  let oldHandler = server.toplevelHandlers.info;
  server.toplevelHandlers.info = handleReassign;

  function undo() {
    _("Undoing test changes.");
    server.toplevelHandlers.info = oldHandler;
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:sync:error",
                                undo,
                                "weave:service:sync:finish",
                                Service.infoURL);
});

add_test(function test_momentary_401_storage() {
  _("Test a failure for any storage URL, not just engine parts. " +
    "Resolved by reassignment.");
  let server = prepareServer();

  
  let oldHandler = server.toplevelHandlers.storage;
  server.toplevelHandlers.storage = handleReassign;

  function undo() {
    _("Undoing test changes.");
    server.toplevelHandlers.storage = oldHandler;
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:login:error",
                                undo,
                                "weave:service:sync:finish",
                                Service.storageURL + "meta/global");
});

add_test(function test_loop_avoidance_storage() {
  _("Test that a repeated failure doesn't result in a sync loop " +
    "if node reassignment cannot resolve the failure.");

  let server = prepareServer();

  
  let oldHandler = server.toplevelHandlers.storage;
  server.toplevelHandlers.storage = handleReassign;

  let firstNotification  = "weave:service:login:error";
  let secondNotification = "weave:service:login:error";
  let thirdNotification  = "weave:service:sync:finish";

  let nodeFetched = false;

  
  
  
  let now;

  function onFirstSync() {
    _("First sync completed.");
    Svc.Obs.remove(firstNotification, onFirstSync);
    Svc.Obs.add(secondNotification, onSecondSync);

    do_check_eq(Service.clusterURL, "");

    
    do_check_true(Services.prefs.getBoolPref("services.sync.lastSyncReassigned"));

    
    
    
    nodeFetched = false;

    
    
    installNodeHandler(server, function () {
      nodeFetched = true;
    });

    
    now = Date.now();
  }

  function onSecondSync() {
    _("Second sync completed.");
    Svc.Obs.remove(secondNotification, onSecondSync);
    Svc.Obs.add(thirdNotification, onThirdSync);

    
    let elapsedTime = Date.now() - now;
    do_check_true(elapsedTime < MINIMUM_BACKOFF_INTERVAL);

    
    do_check_true(getReassigned());

    
    
    
    let expectedNextSync = 1000 * Math.floor((now + MINIMUM_BACKOFF_INTERVAL) / 1000);
    _("Next sync scheduled for " + SyncScheduler.nextSync);
    _("Expected to be slightly greater than " + expectedNextSync);

    do_check_true(SyncScheduler.nextSync >= expectedNextSync);
    do_check_true(!!SyncScheduler.syncTimer);

    
    server.toplevelHandlers.storage = oldHandler;

    
    
    SyncScheduler.scheduleNextSync(0);
  }
  function onThirdSync() {
    Svc.Obs.remove(thirdNotification, onThirdSync);

    
    SyncScheduler.clearSyncTriggers();

    
    
    waitForZeroTimer(function () {
      _("Third sync nextTick.");
      do_check_false(getReassigned());
      do_check_true(nodeFetched);
      Service.startOver();
      server.stop(run_next_test);
    });
  }

  Svc.Obs.add(firstNotification, onFirstSync);

  now = Date.now();
  Service.sync();
});

add_test(function test_loop_avoidance_engine() {
  _("Test that a repeated 401 in an engine doesn't result in a sync loop " +
    "if node reassignment cannot resolve the failure.");
  let server = prepareServer();
  let john   = server.user("johndoe");

  _("Enabling the Rotary engine.");
  let engine = Engines.get("rotary");
  engine.enabled = true;

  
  
  let global = {syncID: Service.syncID,
                storageVersion: STORAGE_VERSION,
                rotary: {version: engine.version,
                         syncID:  engine.syncID}}
  john.createCollection("meta").insert("global", global);

  _("First sync to prepare server contents.");
  Service.sync();

  _("Setting up Rotary collection to 401.");
  let rotary = john.createCollection("rotary");
  let oldHandler = rotary.collectionHandler;
  rotary.collectionHandler = handleReassign.bind(this, undefined);

  
  john.collection("rotary").timestamp += 1000;

  function onLoginStart() {
    
    _("Ensuring that lastSyncReassigned is still set at next sync start.");
    do_check_true(getReassigned());
  }

  function beforeSuccessfulSync() {
    _("Undoing test changes.");
    rotary.collectionHandler = oldHandler;
  }

  function afterSuccessfulSync() {
    Svc.Obs.remove("weave:service:login:start", onLoginStart);
    Service.startOver();
    server.stop(run_next_test);
  }

  let firstNotification  = "weave:service:sync:finish";
  let secondNotification = "weave:service:sync:finish";
  let thirdNotification  = "weave:service:sync:finish";

  let nodeFetched = false;

  
  
  
  let now;

  function onFirstSync() {
    _("First sync completed.");
    Svc.Obs.remove(firstNotification, onFirstSync);
    Svc.Obs.add(secondNotification, onSecondSync);

    do_check_eq(Service.clusterURL, "");

    _("Adding observer that lastSyncReassigned is still set on login.");
    Svc.Obs.add("weave:service:login:start", onLoginStart);

    
    do_check_true(Services.prefs.getBoolPref("services.sync.lastSyncReassigned"));

    
    
    
    nodeFetched = false;

    
    
    installNodeHandler(server, function () {
      nodeFetched = true;
    });

    
    now = Date.now();
  }

  function onSecondSync() {
    _("Second sync completed.");
    Svc.Obs.remove(secondNotification, onSecondSync);
    Svc.Obs.add(thirdNotification, onThirdSync);

    
    let elapsedTime = Date.now() - now;
    do_check_true(elapsedTime < MINIMUM_BACKOFF_INTERVAL);

    
    do_check_true(getReassigned());

    
    
    
    let expectedNextSync = 1000 * Math.floor((now + MINIMUM_BACKOFF_INTERVAL) / 1000);
    _("Next sync scheduled for " + SyncScheduler.nextSync);
    _("Expected to be slightly greater than " + expectedNextSync);

    do_check_true(SyncScheduler.nextSync >= expectedNextSync);
    do_check_true(!!SyncScheduler.syncTimer);

    
    beforeSuccessfulSync();

    
    
    SyncScheduler.scheduleNextSync(0);
  }

  function onThirdSync() {
    Svc.Obs.remove(thirdNotification, onThirdSync);

    
    SyncScheduler.clearSyncTriggers();

    
    
    waitForZeroTimer(function () {
      _("Third sync nextTick.");
      do_check_false(getReassigned());
      do_check_true(nodeFetched);
      afterSuccessfulSync();
    });
  }

  Svc.Obs.add(firstNotification, onFirstSync);

  now = Date.now();
  Service.sync();
});
