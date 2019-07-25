


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
  server.registerPathHandler(nodePath, handleNodeRequest);
  _("Registered node handler at " + nodePath);
}





let reassignments = {
  "crypto": false,
  "info": false,
  "meta": false,
  "rotary": false
};
function maybeReassign(handler, name) {
  return function (request, response) {
    if (reassignments[name]) {
      return handleReassign(null, request, response);
    }
    return handler(request, response);
  };
}

function prepareServer() {
  Service.username   = "johndoe";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.password   = "ilovejane";
  Service.serverURL  = "http://localhost:8080/";
  Service.clusterURL = "http://localhost:8080/";

  do_check_eq(Service.userAPI, "http://localhost:8080/user/1.0/");

  let collectionsHelper = track_collections_helper();
  let upd = collectionsHelper.with_updated_collection;
  let collections = collectionsHelper.collections;

  let engine  = Engines.get("rotary");
  let engines = {rotary: {version: engine.version,
                          syncID:  engine.syncID}};
  let global  = new ServerWBO("global", {engines: engines});
  let rotary  = new ServerCollection({}, true);
  let clients = new ServerCollection({}, true);
  let keys    = new ServerWBO("keys");

  let rotaryHandler = maybeReassign(upd("rotary", rotary.handler()), "rotary");
  let cryptoHandler = maybeReassign(upd("crypto", keys.handler()), "crypto");
  let metaHandler   = maybeReassign(upd("meta", global.handler()), "meta");
  let infoHandler   = maybeReassign(collectionsHelper.handler, "info");

  let server = httpd_setup({
    "/1.1/johndoe/storage/clients":     upd("clients", clients.handler()),
    "/1.1/johndoe/storage/crypto/keys": cryptoHandler,
    "/1.1/johndoe/storage/meta/global": metaHandler,
    "/1.1/johndoe/storage/rotary":      rotaryHandler,
    "/1.1/johndoe/info/collections":    infoHandler
  });

  return [server, global, rotary, collectionsHelper];
}






function syncAndExpectNodeReassignment(server, firstNotification, undo,
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

      
      _("Undoing test changes.");
      undo();
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
  let [server, global, rotary, collectionsHelper] = prepareServer();

  _("Enabling the Rotary engine.");
  let engine = Engines.get("rotary");
  engine.enabled = true;

  
  
  let g = {syncID: Service.syncID,
           storageVersion: STORAGE_VERSION,
           rotary: {version: engine.version,
                    syncID:  engine.syncID}}

  global.payload = JSON.stringify(g);
  global.modified = new_timestamp();
  collectionsHelper.update_collection("meta", global.modified);

  _("First sync to prepare server contents.");
  Service.sync();

  _("Setting up Rotary collection to 401.");
  reassignments["rotary"] = true;

  
  
  rotary.modified = new_timestamp() + 10;
  collectionsHelper.update_collection("rotary", rotary.modified);

  function undo() {
    reassignments["rotary"] = false;
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:sync:finish",
                                undo,
                                "weave:service:sync:finish",
                                Service.storageURL + "rotary");
});


add_test(function test_momentary_401_info_collections() {
  _("Test a failure for info/collections that's resolved by reassignment.");
  let [server, global, rotary] = prepareServer();

  _("First sync to prepare server contents.");
  Service.sync();

  
  reassignments["info"] = true;

  function undo() {
    reassignments["info"] = false;
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
  let [server, global, rotary] = prepareServer();

  
  reassignments["crypto"] = true;
  reassignments["meta"]   = true;
  reassignments["rotary"] = true;

  function undo() {
    reassignments["crypto"] = false;
    reassignments["meta"]   = false;
    reassignments["rotary"] = false;
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:login:error",
                                undo,
                                "weave:service:sync:finish",
                                Service.storageURL + "meta/global");
});

add_test(function test_loop_avoidance() {
  _("Test that a repeated failure doesn't result in a sync loop " +
    "if node reassignment cannot resolve the failure.");

  let [server, global, rotary] = prepareServer();

  
  reassignments["crypto"] = true;
  reassignments["meta"]   = true;
  reassignments["rotary"] = true;

  let firstNotification  = "weave:service:login:error";
  let secondNotification = "weave:service:login:error";
  let thirdNotification  = "weave:service:sync:finish";

  let nodeFetched = false;

  
  
  
  let now;

  function getReassigned() {
    return Services.prefs.getBoolPref("services.sync.lastSyncReassigned");
  }

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

    
    reassignments["crypto"] = false;
    reassignments["meta"]   = false;
    reassignments["rotary"] = false;

    
    
    SyncScheduler.scheduleNextSync(0);
  }
  function onThirdSync() {
    Svc.Obs.remove(thirdNotification, onThirdSync);

    
    SyncScheduler.clearSyncTriggers();

    
    
    waitForZeroTimer(function () {
      _("Third sync nextTick.");

      
      do_check_throws(getReassigned, Cr.NS_ERROR_UNEXPECTED);
      do_check_true(nodeFetched);
      Service.startOver();
      server.stop(run_next_test);
    });
  }

  Svc.Obs.add(firstNotification, onFirstSync);

  now = Date.now();
  Service.sync();
});
