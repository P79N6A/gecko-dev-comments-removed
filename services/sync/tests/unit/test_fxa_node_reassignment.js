


_("Test that node reassignment happens correctly using the FxA identity mgr.");





Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services/sync/rotaryengine.js");
Cu.import("resource://services-sync/browserid_identity.js");
Cu.import("resource://testing-common/services/sync/utils.js");

Service.engineManager.clear();

function run_test() {
  Log.repository.getLogger("Sync.AsyncResource").level = Log.Level.Trace;
  Log.repository.getLogger("Sync.ErrorHandler").level  = Log.Level.Trace;
  Log.repository.getLogger("Sync.Resource").level      = Log.Level.Trace;
  Log.repository.getLogger("Sync.RESTRequest").level   = Log.Level.Trace;
  Log.repository.getLogger("Sync.Service").level       = Log.Level.Trace;
  Log.repository.getLogger("Sync.SyncScheduler").level = Log.Level.Trace;
  initTestLogging();

  Service.engineManager.register(RotaryEngine);

  
  Status.__authManager = Service.identity = new BrowserIDManager();
  Service._clusterManager = Service.identity.createClusterManager(Service);

  
  function onUIError() {
    do_throw("Errors should not be presented in the UI.");
  }
  Svc.Obs.add("weave:ui:login:error", onUIError);
  Svc.Obs.add("weave:ui:sync:error", onUIError);

  run_next_test();
}




function handleReassign(handler, req, resp) {
  resp.setStatusLine(req.httpVersion, 401, "Node reassignment");
  resp.setHeader("Content-Type", "application/json");
  let reassignBody = JSON.stringify({error: "401inator in place"});
  resp.bodyOutputStream.write(reassignBody, reassignBody.length);
}

let numTokenRequests = 0;

function prepareServer(cbAfterTokenFetch) {
  let config = makeIdentityConfig({username: "johndoe"});
  let server = new SyncServer();
  server.registerUser("johndoe");
  server.start();

  
  
  config.fxaccount.token.endpoint = server.baseURI + "1.1/johndoe";
  
  let numReassigns = 0;
  return configureIdentity(config).then(() => {
    Service.identity._tokenServerClient = {
      getTokenFromBrowserIDAssertion: function(uri, assertion, cb) {
        
        
        
        numReassigns += 1;
        let trailingZeros = new Array(numReassigns + 1).join('0');
        let token = config.fxaccount.token;
        token.endpoint = server.baseURI + "1.1" + trailingZeros + "/johndoe";
        token.uid = config.username;
        numTokenRequests += 1;
        cb(null, token);
        if (cbAfterTokenFetch) {
          cbAfterTokenFetch();
        }
      },
    };
    Service.clusterURL = config.fxaccount.token.endpoint;
    return server;
  });
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
  _("Starting syncAndExpectNodeReassignment\n");
  let deferred = Promise.defer();
  function onwards() {
    let numTokenRequestsBefore;
    function onFirstSync() {
      _("First sync completed.");
      Svc.Obs.remove(firstNotification, onFirstSync);
      Svc.Obs.add(secondNotification, onSecondSync);

      do_check_eq(Service.clusterURL, "");

      
      numTokenRequestsBefore = numTokenRequests;

      
      between();
    }
    function onSecondSync() {
      _("Second sync completed.");
      Svc.Obs.remove(secondNotification, onSecondSync);
      Service.scheduler.clearSyncTriggers();

      
      
      waitForZeroTimer(function () {
        _("Second sync nextTick.");
        do_check_eq(numTokenRequests, numTokenRequestsBefore + 1, "fetched a new token");
        Service.startOver();
        server.stop(deferred.resolve);
      });
    }

    Svc.Obs.add(firstNotification, onFirstSync);
    Service.sync();
  }

  
  _("Making request to " + url + " which should 401");
  let request = new RESTRequest(url);
  request.get(function () {
    do_check_eq(request.response.status, 401);
    Utils.nextTick(onwards);
  });
  yield deferred.promise;
}

add_task(function test_momentary_401_engine() {
  _("Test a failure for engine URLs that's resolved by reassignment.");
  let server = yield prepareServer();
  let john   = server.user("johndoe");

  _("Enabling the Rotary engine.");
  let engine = Service.engineManager.get("rotary");
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

  yield syncAndExpectNodeReassignment(server,
                                      "weave:service:sync:finish",
                                      between,
                                      "weave:service:sync:finish",
                                      Service.storageURL + "rotary");
});


add_task(function test_momentary_401_info_collections_loggedin() {
  _("Test a failure for info/collections after login that's resolved by reassignment.");
  let server = yield prepareServer();

  _("First sync to prepare server contents.");
  Service.sync();

  _("Arrange for info/collections to return a 401.");
  let oldHandler = server.toplevelHandlers.info;
  server.toplevelHandlers.info = handleReassign;

  function undo() {
    _("Undoing test changes.");
    server.toplevelHandlers.info = oldHandler;
  }

  do_check_true(Service.isLoggedIn, "already logged in");

  yield syncAndExpectNodeReassignment(server,
                                      "weave:service:sync:error",
                                      undo,
                                      "weave:service:sync:finish",
                                      Service.infoURL);
});




add_task(function test_momentary_401_info_collections_loggedout() {
  _("Test a failure for info/collections before login that's resolved by reassignment.");

  let oldHandler;
  let sawTokenFetch = false;

  function afterTokenFetch() {
    
    
    server.toplevelHandlers.info = oldHandler;
    sawTokenFetch = true;
  }

  let server = yield prepareServer(afterTokenFetch);

  
  
  oldHandler = server.toplevelHandlers.info
  server.toplevelHandlers.info = handleReassign;

  do_check_false(Service.isLoggedIn, "not already logged in");

  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED, "sync succeeded");
  
  do_check_true(sawTokenFetch, "a new token was fetched by this test.")
  
  Service.startOver();
  let deferred = Promise.defer();
  server.stop(deferred.resolve);
  yield deferred.promise;
});


add_task(function test_momentary_401_storage_loggedin() {
  _("Test a failure for any storage URL after login that's resolved by" +
    "reassignment.");
  let server = yield prepareServer();

  _("First sync to prepare server contents.");
  Service.sync();

  _("Arrange for meta/global to return a 401.");
  let oldHandler = server.toplevelHandlers.storage;
  server.toplevelHandlers.storage = handleReassign;

  function undo() {
    _("Undoing test changes.");
    server.toplevelHandlers.storage = oldHandler;
  }

  do_check_true(Service.isLoggedIn, "already logged in");

  yield syncAndExpectNodeReassignment(server,
                                      "weave:service:sync:error",
                                      undo,
                                      "weave:service:sync:finish",
                                      Service.storageURL + "meta/global");
});


add_task(function test_momentary_401_storage_loggedout() {
  _("Test a failure for any storage URL before login, not just engine parts. " +
    "Resolved by reassignment.");
  let server = yield prepareServer();

  
  let oldHandler = server.toplevelHandlers.storage;
  server.toplevelHandlers.storage = handleReassign;

  function undo() {
    _("Undoing test changes.");
    server.toplevelHandlers.storage = oldHandler;
  }

  do_check_false(Service.isLoggedIn, "already logged in");

  yield syncAndExpectNodeReassignment(server,
                                      "weave:service:login:error",
                                      undo,
                                      "weave:service:sync:finish",
                                      Service.storageURL + "meta/global");
});

