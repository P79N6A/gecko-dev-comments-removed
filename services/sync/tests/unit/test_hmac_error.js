Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/util.js");


let hmacErrorCount = 0;
(function () {
  let hHE = Service.handleHMACEvent;
  Service.handleHMACEvent = function () {
    hmacErrorCount++;
    return hHE.call(Service);
  };
})();

function shared_setup() {
  hmacErrorCount = 0;

  
  setBasicCredentials("foo", "foo", "aabcdeabcdeabcdeabcdeabcde");
  Service.serverURL  = TEST_SERVER_URL;
  Service.clusterURL = TEST_CLUSTER_URL;

  
  Engines._engines = {};
  Engines.register(RotaryEngine);
  let engine = Engines.get("rotary");
  engine.enabled = true;
  engine.lastSync = 123; 
  engine._store.items = {flying: "LNER Class A3 4472",
                         scotsman: "Flying Scotsman"};
  engine._tracker.addChangedID('scotsman', 0);
  do_check_eq(1, Engines.getEnabled().length);

  let engines = {rotary:  {version: engine.version,
                           syncID:  engine.syncID},
                 clients: {version: Clients.version,
                           syncID:  Clients.syncID}};

  
  let global      = new ServerWBO("global", {engines: engines});
  let keysWBO     = new ServerWBO("keys");
  let rotaryColl  = new ServerCollection({}, true);
  let clientsColl = new ServerCollection({}, true);

  return [engine, rotaryColl, clientsColl, keysWBO, global];
}

add_test(function hmac_error_during_404() {
  _("Attempt to replicate the HMAC error setup.");
  let [engine, rotaryColl, clientsColl, keysWBO, global] = shared_setup();

  
  let keysHandler    = keysWBO.handler();
  let key404Counter  = 0;
  let keys404Handler = function (request, response) {
    if (key404Counter > 0) {
      let body = "Not Found";
      response.setStatusLine(request.httpVersion, 404, body);
      response.bodyOutputStream.write(body, body.length);
      key404Counter--;
      return;
    }
    keysHandler(request, response);
  };

  let collectionsHelper = track_collections_helper();
  let upd = collectionsHelper.with_updated_collection;
  let collections = collectionsHelper.collections;
  let handlers = {
    "/1.1/foo/info/collections": collectionsHelper.handler,
    "/1.1/foo/storage/meta/global": upd("meta", global.handler()),
    "/1.1/foo/storage/crypto/keys": upd("crypto", keys404Handler),
    "/1.1/foo/storage/clients": upd("clients", clientsColl.handler()),
    "/1.1/foo/storage/rotary": upd("rotary", rotaryColl.handler())
  };

  let server = sync_httpd_setup(handlers);

  try {
    _("Syncing.");
    Service.sync();
    _("Partially resetting client, as if after a restart, and forcing redownload.");
    CollectionKeys.clear();
    engine.lastSync = 0;        
    key404Counter = 1;
    _("---------------------------");
    Service.sync();
    _("---------------------------");

    
    do_check_eq(hmacErrorCount, 0)
  } finally {
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    server.stop(run_next_test);
  }
});

add_test(function hmac_error_during_node_reassignment() {
  _("Attempt to replicate an HMAC error during node reassignment.");
  let [engine, rotaryColl, clientsColl, keysWBO, global] = shared_setup();

  let collectionsHelper = track_collections_helper();
  let upd = collectionsHelper.with_updated_collection;

  
  
  function on401() {
    _("Deleting server data...");
    global.delete();
    rotaryColl.delete();
    keysWBO.delete();
    clientsColl.delete();
    delete collectionsHelper.collections.rotary;
    delete collectionsHelper.collections.crypto;
    delete collectionsHelper.collections.clients;
    _("Deleted server data.");
  }

  let should401 = false;
  function upd401(coll, handler) {
    return function (request, response) {
      if (should401 && (request.method != "DELETE")) {
        on401();
        should401 = false;
        let body = "\"reassigned!\"";
        response.setStatusLine(request.httpVersion, 401, "Node reassignment.");
        response.bodyOutputStream.write(body, body.length);
        return;
      }
      handler(request, response);
    };
  }

  function sameNodeHandler(request, response) {
    
    let url = Service.serverURL.replace("localhost", "LOCALHOST");
    _("Client requesting reassignment; pointing them to " + url);
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.bodyOutputStream.write(url, url.length);
  }

  let handlers = {
    "/user/1.0/foo/node/weave":     sameNodeHandler,
    "/1.1/foo/info/collections":    collectionsHelper.handler,
    "/1.1/foo/storage/meta/global": upd("meta", global.handler()),
    "/1.1/foo/storage/crypto/keys": upd("crypto", keysWBO.handler()),
    "/1.1/foo/storage/clients":     upd401("clients", clientsColl.handler()),
    "/1.1/foo/storage/rotary":      upd("rotary", rotaryColl.handler())
  };

  let server = sync_httpd_setup(handlers);
  _("Syncing.");
  
  
  should401 = true;

  
  
  
  function onSyncError() {
    do_throw("Should not get a sync error!");
  }
  function onSyncFinished() {}
  let obs = {
    observe: function observe(subject, topic, data) {
      switch (topic) {
        case "weave:service:sync:error":
          onSyncError();
          break;
        case "weave:service:sync:finish":
          onSyncFinished();
          break;
      }
    }
  };

  Svc.Obs.add("weave:service:sync:finish", obs);
  Svc.Obs.add("weave:service:sync:error", obs);

  
  
  function onwards() {
    _("== Invoking first sync.");
    Service.sync();
    _("We should not simultaneously have data but no keys on the server.");
    let hasData = rotaryColl.wbo("flying") ||
                  rotaryColl.wbo("scotsman");
    let hasKeys = keysWBO.modified;

    _("We correctly handle 401s by aborting the sync and starting again.");
    do_check_true(!hasData == !hasKeys);

    _("Be prepared for the second (automatic) sync...");
  }

  _("Make sure that syncing again causes recovery.");
  onSyncFinished = function() {
    _("== First sync done.");
    _("---------------------------");
    onSyncFinished = function() {
      _("== Second (automatic) sync done.");
      hasData = rotaryColl.wbo("flying") ||
                rotaryColl.wbo("scotsman");
      hasKeys = keysWBO.modified;
      do_check_true(!hasData == !hasKeys);

      
      
      Utils.nextTick(function() {
        _("Now a fresh sync will get no HMAC errors.");
        _("Partially resetting client, as if after a restart, and forcing redownload.");
        CollectionKeys.clear();
        engine.lastSync = 0;
        hmacErrorCount = 0;

        onSyncFinished = function() {
          
          do_check_eq(hmacErrorCount, 0)

          Svc.Obs.remove("weave:service:sync:finish", obs);
          Svc.Obs.remove("weave:service:sync:error", obs);
                  
          Svc.Prefs.resetBranch("");
          Records.clearCache();
          server.stop(run_next_test);
        };

        Service.sync();
      },
      this);
    };
  };

  onwards();
});

function run_test() {
  initTestLogging("Trace");
  run_next_test();
}
