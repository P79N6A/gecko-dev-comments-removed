


Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/keys.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services/sync/utils.js");
Cu.import("resource://gre/modules/FileUtils.jsm");

const FAKE_SERVER_URL = "http://dummy:9000/";
const logsdir = FileUtils.getDir("ProfD", ["weave", "logs"], true);

const PROLONGED_ERROR_DURATION =
  (Svc.Prefs.get('errorhandler.networkFailureReportTimeout') * 2) * 1000;

const NON_PROLONGED_ERROR_DURATION =
  (Svc.Prefs.get('errorhandler.networkFailureReportTimeout') / 2) * 1000;

Service.engineManager.clear();

function setLastSync(lastSyncValue) {
  Svc.Prefs.set("lastSync", (new Date(Date.now() - lastSyncValue)).toString());
}

function CatapultEngine() {
  SyncEngine.call(this, "Catapult", Service);
}
CatapultEngine.prototype = {
  __proto__: SyncEngine.prototype,
  exception: null, 
  _sync: function _sync() {
    if (this.exception) {
      throw this.exception;
    }
  }
};

let engineManager = Service.engineManager;
engineManager.register(CatapultEngine);



let errorHandler = Service.errorHandler;

function run_test() {
  initTestLogging("Trace");

  Log.repository.getLogger("Sync.Service").level = Log.Level.Trace;
  Log.repository.getLogger("Sync.SyncScheduler").level = Log.Level.Trace;
  Log.repository.getLogger("Sync.ErrorHandler").level = Log.Level.Trace;

  ensureLegacyIdentityManager();

  run_next_test();
}

function generateCredentialsChangedFailure() {
  
  
  let newSyncKeyBundle = new SyncKeyBundle("johndoe", "23456234562345623456234562");
  let keys = Service.collectionKeys.asWBO();
  keys.encrypt(newSyncKeyBundle);
  keys.upload(Service.resource(Service.cryptoKeysURL));
}

function service_unavailable(request, response) {
  let body = "Service Unavailable";
  response.setStatusLine(request.httpVersion, 503, "Service Unavailable");
  response.setHeader("Retry-After", "42");
  response.bodyOutputStream.write(body, body.length);
}

function sync_httpd_setup() {
  let global = new ServerWBO("global", {
    syncID: Service.syncID,
    storageVersion: STORAGE_VERSION,
    engines: {clients: {version: Service.clientsEngine.version,
                        syncID: Service.clientsEngine.syncID},
              catapult: {version: engineManager.get("catapult").version,
                         syncID: engineManager.get("catapult").syncID}}
  });
  let clientsColl = new ServerCollection({}, true);

  
  let collectionsHelper = track_collections_helper();
  let upd = collectionsHelper.with_updated_collection;

  let handler_401 = httpd_handler(401, "Unauthorized");
  return httpd_setup({
    
    "/1.1/johndoe/storage/meta/global": upd("meta", global.handler()),
    "/1.1/johndoe/info/collections": collectionsHelper.handler,
    "/1.1/johndoe/storage/crypto/keys":
      upd("crypto", (new ServerWBO("keys")).handler()),
    "/1.1/johndoe/storage/clients": upd("clients", clientsColl.handler()),

    
    "/1.1/janedoe/storage/meta/global": handler_401,
    "/1.1/janedoe/info/collections": handler_401,

    
    "/maintenance/1.1/broken.info/info/collections": service_unavailable,

    
    "/maintenance/1.1/broken.meta/storage/meta/global": service_unavailable,
    "/maintenance/1.1/broken.meta/info/collections": collectionsHelper.handler,

    
    "/maintenance/1.1/broken.keys/storage/meta/global": upd("meta", global.handler()),
    "/maintenance/1.1/broken.keys/info/collections": collectionsHelper.handler,
    "/maintenance/1.1/broken.keys/storage/crypto/keys": service_unavailable,

    
    "/maintenance/1.1/broken.wipe/info/collections": collectionsHelper.handler,
    "/maintenance/1.1/broken.wipe/storage/meta/global": upd("meta", global.handler()),
    "/maintenance/1.1/broken.wipe/storage/crypto/keys":
      upd("crypto", (new ServerWBO("keys")).handler()),
    "/maintenance/1.1/broken.wipe/storage": service_unavailable,
    "/maintenance/1.1/broken.wipe/storage/clients": upd("clients", clientsColl.handler()),
    "/maintenance/1.1/broken.wipe/storage/catapult": service_unavailable
  });
}

function setUp(server) {
  return configureIdentity({username: "johndoe"}).then(
    () => {
      Service.serverURL  = server.baseURI + "/";
      Service.clusterURL = server.baseURI + "/";
    }
  ).then(
    () => generateAndUploadKeys()
  );
}

function generateAndUploadKeys() {
  generateNewKeys(Service.collectionKeys);
  let serverKeys = Service.collectionKeys.asWBO("crypto", "keys");
  serverKeys.encrypt(Service.identity.syncKeyBundle);
  return serverKeys.upload(Service.resource(Service.cryptoKeysURL)).success;
}

function clean() {
  Service.startOver();
  Status.resetSync();
  Status.resetBackoff();
  errorHandler.didReportProlongedError = false;
}

add_identity_test(this, function test_401_logout() {
  let server = sync_httpd_setup();
  yield setUp(server);

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  let deferred = Promise.defer();
  Svc.Obs.add("weave:service:sync:error", onSyncError);
  function onSyncError() {
    _("Got weave:service:sync:error in first sync.");
    Svc.Obs.remove("weave:service:sync:error", onSyncError);

    
    function onLoginError() {
      _("Got weave:service:login:error in second sync.");
      Svc.Obs.remove("weave:service:login:error", onLoginError);

      do_check_eq(Status.login, LOGIN_FAILED_LOGIN_REJECTED);
      do_check_false(Service.isLoggedIn);

      
      Utils.nextTick(function () {
        Service.startOver();
        server.stop(deferred.resolve);
      });
    }
    Svc.Obs.add("weave:service:login:error", onLoginError);
  }

  
  yield configureIdentity({username: "janedoe"});
  Service._updateCachedURLs();

  _("Starting first sync.");
  Service.sync();
  _("First sync done.");
  yield deferred.promise;
});

add_identity_test(this, function test_credentials_changed_logout() {
  let server = sync_httpd_setup();
  yield setUp(server);

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();
  Service.sync();

  do_check_eq(Status.sync, CREDENTIALS_CHANGED);
  do_check_false(Service.isLoggedIn);

  
  Service.startOver();
  let deferred = Promise.defer();
  server.stop(deferred.resolve);
  yield deferred.promise;
});

add_identity_test(this, function test_no_lastSync_pref() {
  
  Status.resetSync();
  errorHandler.dontIgnoreErrors = true;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  errorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(errorHandler.shouldReportError());

});

add_identity_test(this, function test_shouldReportError() {
  Status.login = MASTER_PASSWORD_LOCKED;
  do_check_false(errorHandler.shouldReportError());

  
  
  Service.serverURL  = FAKE_SERVER_URL;
  Service.clusterURL = FAKE_SERVER_URL;

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(errorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(errorHandler.shouldReportError());

  
  do_check_false(errorHandler.didReportProlongedError);
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(errorHandler.shouldReportError());
  do_check_true(errorHandler.didReportProlongedError);

  
  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_false(errorHandler.shouldReportError());
  do_check_true(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  errorHandler.didReportProlongedError = false;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(errorHandler.shouldReportError());
  do_check_true(errorHandler.didReportProlongedError);
  errorHandler.didReportProlongedError = false;

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(errorHandler.shouldReportError());
  do_check_true(errorHandler.didReportProlongedError);
  errorHandler.didReportProlongedError = false;

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(errorHandler.shouldReportError());
  do_check_true(errorHandler.didReportProlongedError);
  errorHandler.didReportProlongedError = false;

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_false(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_false(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.sync = SERVER_MAINTENANCE;
  do_check_false(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.login = SERVER_MAINTENANCE;
  do_check_false(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.sync = SERVER_MAINTENANCE;
  do_check_true(errorHandler.shouldReportError());
  do_check_true(errorHandler.didReportProlongedError);
  errorHandler.didReportProlongedError = false;

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = false;
  Status.login = SERVER_MAINTENANCE;
  do_check_true(errorHandler.shouldReportError());
  do_check_true(errorHandler.didReportProlongedError);
  errorHandler.didReportProlongedError = false;

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.sync = SERVER_MAINTENANCE;
  do_check_true(errorHandler.shouldReportError());
  
  do_check_false(errorHandler.didReportProlongedError);

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.login = SERVER_MAINTENANCE;
  do_check_true(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.sync = SERVER_MAINTENANCE;
  do_check_true(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);

  
  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.dontIgnoreErrors = true;
  Status.login = SERVER_MAINTENANCE;
  do_check_true(errorHandler.shouldReportError());
  do_check_false(errorHandler.didReportProlongedError);
});

add_identity_test(this, function test_shouldReportError_master_password() {
  _("Test error ignored due to locked master password");
  let server = sync_httpd_setup();
  yield setUp(server);

  
  
  Service._verifyLogin = Service.verifyLogin;
  Service.verifyLogin = function () {
    Status.login = MASTER_PASSWORD_LOCKED;
    return false;
  };

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  do_check_false(errorHandler.shouldReportError());

  
  Service.verifyLogin = Service._verifyLogin;
  clean();
  let deferred = Promise.defer();
  server.stop(deferred.resolve);
  yield deferred.promise;
});



add_identity_test(this, function test_shouldReportLoginFailureWithNoCluster() {
  
  Service.serverURL  = "";
  Service.clusterURL = "";

  
  Status.resetSync();
  
  Status.login = LOGIN_FAILED_LOGIN_REJECTED;
  do_check_true(errorHandler.shouldReportError());
  
  
  Status.login = LOGIN_SUCCEEDED;
  do_check_false(errorHandler.shouldReportError());
});



add_task(function test_login_syncAndReportErrors_non_network_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);
  Service.identity.basicPassword = null;

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NO_PASSWORD);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_sync_syncAndReportErrors_non_network_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, CREDENTIALS_CHANGED);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});



add_task(function test_login_syncAndReportErrors_prolonged_non_network_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);
  Service.identity.basicPassword = null;

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NO_PASSWORD);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_sync_syncAndReportErrors_prolonged_non_network_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, CREDENTIALS_CHANGED);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_login_syncAndReportErrors_network_error() {
  
  yield configureIdentity({username: "broken.wipe"});
  Service.serverURL  = FAKE_SERVER_URL;
  Service.clusterURL = FAKE_SERVER_URL;

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NETWORK_ERROR);

    clean();
    deferred.resolve();
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});


add_test(function test_sync_syncAndReportErrors_network_error() {
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, LOGIN_FAILED_NETWORK_ERROR);

    Services.io.offline = false;
    clean();
    run_next_test();
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
});

add_identity_test(this, function test_login_syncAndReportErrors_prolonged_network_error() {
  
  
  yield configureIdentity({username: "johndoe"});

  Service.serverURL  = FAKE_SERVER_URL;
  Service.clusterURL = FAKE_SERVER_URL;

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NETWORK_ERROR);

    clean();
    deferred.resolve();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_test(function test_sync_syncAndReportErrors_prolonged_network_error() {
  
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, LOGIN_FAILED_NETWORK_ERROR);

    Services.io.offline = false;
    clean();
    run_next_test();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
});

add_task(function test_login_prolonged_non_network_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);
  Service.identity.basicPassword = null;

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_task(function test_sync_prolonged_non_network_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_login_prolonged_network_error() {
  
  yield configureIdentity({username: "johndoe"});
  Service.serverURL  = FAKE_SERVER_URL;
  Service.clusterURL = FAKE_SERVER_URL;

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    deferred.resolve();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_test(function test_sync_prolonged_network_error() {
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    Services.io.offline = false;
    clean();
    run_next_test();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_task(function test_login_non_network_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);
  Service.identity.basicPassword = null;

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NO_PASSWORD);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_task(function test_sync_non_network_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, CREDENTIALS_CHANGED);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_login_network_error() {
  yield configureIdentity({username: "johndoe"});
  Service.serverURL  = FAKE_SERVER_URL;
  Service.clusterURL = FAKE_SERVER_URL;

  let deferred = Promise.defer();
  
  Svc.Obs.add("weave:ui:clear-error", function onClearError() {
    Svc.Obs.remove("weave:ui:clear-error", onClearError);

    do_check_eq(Status.login, LOGIN_FAILED_NETWORK_ERROR);
    do_check_false(errorHandler.didReportProlongedError);

    Services.io.offline = false;
    clean();
    deferred.resolve()
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_test(function test_sync_network_error() {
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:finish", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:sync:finish", onUIUpdate);
    do_check_eq(Status.sync, LOGIN_FAILED_NETWORK_ERROR);
    do_check_false(errorHandler.didReportProlongedError);

    Services.io.offline = false;
    clean();
    run_next_test();
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_identity_test(this, function test_sync_server_maintenance_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);

  const BACKOFF = 42;
  let engine = engineManager.get("catapult");
  engine.enabled = true;
  engine.exception = {status: 503,
                      headers: {"retry-after": BACKOFF}};

  function onSyncError() {
    do_throw("Shouldn't get here!");
  }
  Svc.Obs.add("weave:ui:sync:error", onSyncError);

  do_check_eq(Status.service, STATUS_OK);

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:finish", function onSyncFinish() {
    Svc.Obs.remove("weave:ui:sync:finish", onSyncFinish);

    do_check_eq(Status.service, SYNC_FAILED_PARTIAL);
    do_check_eq(Status.sync, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_info_collections_login_server_maintenance_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);

  Service.username = "broken.info";
  yield configureIdentity({username: "broken.info"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  function onUIUpdate() {
    do_throw("Shouldn't experience UI update!");
  }
  Svc.Obs.add("weave:ui:login:error", onUIUpdate);

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:clear-error", function onLoginFinish() {
    Svc.Obs.remove("weave:ui:clear-error", onLoginFinish);

    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_meta_global_login_server_maintenance_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.meta"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  function onUIUpdate() {
    do_throw("Shouldn't get here!");
  }
  Svc.Obs.add("weave:ui:login:error", onUIUpdate);

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:clear-error", function onLoginFinish() {
    Svc.Obs.remove("weave:ui:clear-error", onLoginFinish);

    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_crypto_keys_login_server_maintenance_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.keys"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  
  Service.collectionKeys.clear();

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  function onUIUpdate() {
    do_throw("Shouldn't get here!");
  }
  Svc.Obs.add("weave:ui:login:error", onUIUpdate);

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:clear-error", function onLoginFinish() {
    Svc.Obs.remove("weave:ui:clear-error", onLoginFinish);

    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    clean();
    server.stop(deferred.resolve);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_task(function test_sync_prolonged_server_maintenance_error() {
  
  let server = sync_httpd_setup();
  yield setUp(server);

  const BACKOFF = 42;
  let engine = engineManager.get("catapult");
  engine.enabled = true;
  engine.exception = {status: 503,
                      headers: {"retry-after": BACKOFF}};

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:sync:error", onUIUpdate);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_info_collections_login_prolonged_server_maintenance_error(){
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.info"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_meta_global_login_prolonged_server_maintenance_error(){
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.meta"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_download_crypto_keys_login_prolonged_server_maintenance_error(){
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.keys"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";
  
  Service.collectionKeys.clear();

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_upload_crypto_keys_login_prolonged_server_maintenance_error(){
  
  let server = sync_httpd_setup();

  
  yield configureIdentity({username: "broken.keys"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_wipeServer_login_prolonged_server_maintenance_error(){
  
  
  let server = sync_httpd_setup();

  
  yield configureIdentity({username: "broken.wipe"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_identity_test(this, function test_wipeRemote_prolonged_server_maintenance_error(){
  
  
  let server = sync_httpd_setup();

  server.registerPathHandler("/1.1/broken.wipe/storage/catapult", service_unavailable);
  yield configureIdentity({username: "broken.wipe"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";
  generateAndUploadKeys();

  let engine = engineManager.get("catapult");
  engine.exception = null;
  engine.enabled = true;

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:sync:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);
    do_check_eq(Svc.Prefs.get("firstSync"), "wipeRemote");
    do_check_true(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  Svc.Prefs.set("firstSync", "wipeRemote");
  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
  yield deferred.promise;
});

add_task(function test_sync_syncAndReportErrors_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  const BACKOFF = 42;
  let engine = engineManager.get("catapult");
  engine.enabled = true;
  engine.exception = {status: 503,
                      headers: {"retry-after": BACKOFF}};

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:sync:error", onUIUpdate);
    do_check_eq(Status.service, SYNC_FAILED_PARTIAL);
    do_check_eq(Status.sync, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_eq(Status.service, STATUS_OK);

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_info_collections_login_syncAndReportErrors_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.info"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_meta_global_login_syncAndReportErrors_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.meta"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_download_crypto_keys_login_syncAndReportErrors_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.keys"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";
  
  Service.collectionKeys.clear();

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_upload_crypto_keys_login_syncAndReportErrors_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();

  
  yield configureIdentity({username: "broken.keys"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_wipeServer_login_syncAndReportErrors_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();

  
  yield configureIdentity({username: "broken.wipe"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_wipeRemote_syncAndReportErrors_server_maintenance_error(){
  
  
  let server = sync_httpd_setup();

  yield configureIdentity({username: "broken.wipe"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";
  generateAndUploadKeys();

  let engine = engineManager.get("catapult");
  engine.exception = null;
  engine.enabled = true;

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:sync:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, SYNC_FAILED);
    do_check_eq(Status.sync, SERVER_MAINTENANCE);
    do_check_eq(Svc.Prefs.get("firstSync"), "wipeRemote");
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  Svc.Prefs.set("firstSync", "wipeRemote");
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_task(function test_sync_syncAndReportErrors_prolonged_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  const BACKOFF = 42;
  let engine = engineManager.get("catapult");
  engine.enabled = true;
  engine.exception = {status: 503,
                      headers: {"retry-after": BACKOFF}};

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:sync:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:sync:error", onUIUpdate);
    do_check_eq(Status.service, SYNC_FAILED_PARTIAL);
    do_check_eq(Status.sync, SERVER_MAINTENANCE);
    
    
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_info_collections_login_syncAndReportErrors_prolonged_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.info"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    
    
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_meta_global_login_syncAndReportErrors_prolonged_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.meta"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    
    
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_download_crypto_keys_login_syncAndReportErrors_prolonged_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();
  yield setUp(server);

  yield configureIdentity({username: "broken.keys"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";
  
  Service.collectionKeys.clear();

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    
    
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_upload_crypto_keys_login_syncAndReportErrors_prolonged_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();

  
  yield configureIdentity({username: "broken.keys"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    
    
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_identity_test(this, function test_wipeServer_login_syncAndReportErrors_prolonged_server_maintenance_error() {
  
  
  let server = sync_httpd_setup();

  
  yield configureIdentity({username: "broken.wipe"});
  Service.serverURL = server.baseURI + "/maintenance/";
  Service.clusterURL = server.baseURI + "/maintenance/";

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function observe(subject, data) {
    Svc.Obs.remove("weave:service:backoff:interval", observe);
    backoffInterval = subject;
  });

  let deferred = Promise.defer();
  Svc.Obs.add("weave:ui:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:login:error", onUIUpdate);
    do_check_true(Status.enforceBackoff);
    do_check_eq(backoffInterval, 42);
    do_check_eq(Status.service, LOGIN_FAILED);
    do_check_eq(Status.login, SERVER_MAINTENANCE);
    
    
    do_check_false(errorHandler.didReportProlongedError);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_false(Status.enforceBackoff);
  do_check_eq(Status.service, STATUS_OK);

  setLastSync(PROLONGED_ERROR_DURATION);
  errorHandler.syncAndReportErrors();
  yield deferred.promise;
});

add_task(function test_sync_engine_generic_fail() {
  let server = sync_httpd_setup();

  let engine = engineManager.get("catapult");
  engine.enabled = true;
  engine.sync = function sync() {
    Svc.Obs.notify("weave:engine:sync:error", "", "catapult");
  };

  let log = Log.repository.getLogger("Sync.ErrorHandler");
  Svc.Prefs.set("log.appender.file.logOnError", true);

  do_check_eq(Status.engines["catapult"], undefined);

  let deferred = Promise.defer();
  
  
  Svc.Obs.add("weave:engine:sync:finish", function onEngineFinish() {
    Svc.Obs.remove("weave:engine:sync:finish", onEngineFinish);

    log.info("Adding reset-file-log observer.");
    Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
      Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

      
      
      _("Status.engines: " + JSON.stringify(Status.engines));
      do_check_eq(Status.engines["catapult"], ENGINE_UNKNOWN_FAIL);
      do_check_eq(Status.service, SYNC_FAILED_PARTIAL);

      
      let entries = logsdir.directoryEntries;
      do_check_true(entries.hasMoreElements());
      let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
      do_check_true(logfile.leafName.startsWith("error-sync-"), logfile.leafName);

      clean();
      server.stop(deferred.resolve);
    });
  });

  do_check_true(yield setUp(server));
  Service.sync();
  yield deferred.promise;
});

add_test(function test_logs_on_sync_error_despite_shouldReportError() {
  _("Ensure that an error is still logged when weave:service:sync:error " +
    "is notified, despite shouldReportError returning false.");

  let log = Log.repository.getLogger("Sync.ErrorHandler");
  Svc.Prefs.set("log.appender.file.logOnError", true);
  log.info("TESTING");

  
  Status.login = MASTER_PASSWORD_LOCKED;
  do_check_false(errorHandler.shouldReportError());

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    
    let entries = logsdir.directoryEntries;
    do_check_true(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    do_check_true(logfile.leafName.startsWith("error-sync-"), logfile.leafName);

    clean();
    run_next_test();
  });
  Svc.Obs.notify("weave:service:sync:error", {});
});

add_test(function test_logs_on_login_error_despite_shouldReportError() {
  _("Ensure that an error is still logged when weave:service:login:error " +
    "is notified, despite shouldReportError returning false.");

  let log = Log.repository.getLogger("Sync.ErrorHandler");
  Svc.Prefs.set("log.appender.file.logOnError", true);
  log.info("TESTING");

  
  Status.login = MASTER_PASSWORD_LOCKED;
  do_check_false(errorHandler.shouldReportError());

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    
    let entries = logsdir.directoryEntries;
    do_check_true(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    do_check_true(logfile.leafName.startsWith("error-sync-"), logfile.leafName);

    clean();
    run_next_test();
  });
  Svc.Obs.notify("weave:service:login:error", {});
});



add_task(function test_engine_applyFailed() {
  let server = sync_httpd_setup();

  let engine = engineManager.get("catapult");
  engine.enabled = true;
  delete engine.exception;
  engine.sync = function sync() {
    Svc.Obs.notify("weave:engine:sync:applied", {newFailed:1}, "catapult");
  };

  let log = Log.repository.getLogger("Sync.ErrorHandler");
  Svc.Prefs.set("log.appender.file.logOnError", true);

  let deferred = Promise.defer();
  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    do_check_eq(Status.engines["catapult"], ENGINE_APPLY_FAIL);
    do_check_eq(Status.service, SYNC_FAILED_PARTIAL);

    
    let entries = logsdir.directoryEntries;
    do_check_true(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    do_check_true(logfile.leafName.startsWith("error-sync-"), logfile.leafName);

    clean();
    server.stop(deferred.resolve);
  });

  do_check_eq(Status.engines["catapult"], undefined);
  do_check_true(yield setUp(server));
  Service.sync();
  yield deferred.promise;
});
