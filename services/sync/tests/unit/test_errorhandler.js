


Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/status.js");

Svc.DefaultPrefs.set("registerEngines", "");
Cu.import("resource://services-sync/service.js");

const logsdir = FileUtils.getDir("ProfD", ["weave", "logs"], true);
const LOG_PREFIX_SUCCESS = "success-";
const LOG_PREFIX_ERROR   = "error-";

const PROLONGED_ERROR_DURATION =
  (Svc.Prefs.get('errorhandler.networkFailureReportTimeout') * 2) * 1000;

const NON_PROLONGED_ERROR_DURATION =
  (Svc.Prefs.get('errorhandler.networkFailureReportTimeout') / 2) * 1000;

function setLastSync(lastSyncValue) {
  Svc.Prefs.set("lastSync", (new Date(Date.now() -
    lastSyncValue)).toString());
}

function CatapultEngine() {
  SyncEngine.call(this, "Catapult");
}
CatapultEngine.prototype = {
  __proto__: SyncEngine.prototype,
  exception: null, 
  sync: function sync() {
    throw this.exception;
  }
};

Engines.register(CatapultEngine);

function run_test() {
  initTestLogging("Trace");

  Log4Moz.repository.getLogger("Sync.Service").level = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.SyncScheduler").level = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.ErrorHandler").level = Log4Moz.Level.Trace;

  run_next_test();
}

function generateCredentialsChangedFailure() {
  
  
  let newSyncKeyBundle = new SyncKeyBundle(PWDMGR_PASSPHRASE_REALM, Service.username);
  newSyncKeyBundle.keyStr = "23456234562345623456234562";
  let keys = CollectionKeys.asWBO();
  keys.encrypt(newSyncKeyBundle);
  keys.upload(Service.cryptoKeysURL);
}

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

  let handler_401 = httpd_handler(401, "Unauthorized");
  return httpd_setup({
    "/1.1/johndoe/storage/meta/global": upd("meta", global.handler()),
    "/1.1/johndoe/info/collections": collectionsHelper.handler,
    "/1.1/johndoe/storage/crypto/keys":
      upd("crypto", (new ServerWBO("keys")).handler()),
    "/1.1/johndoe/storage/clients": upd("clients", clientsColl.handler()),

    "/1.1/janedoe/storage/meta/global": handler_401,
    "/1.1/janedoe/info/collections": handler_401,
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

add_test(function test_401_logout() {
  let server = sync_httpd_setup();
  setUp();

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  
  Service.username = "janedoe";
  Service.sync();

  do_check_eq(Status.login, LOGIN_FAILED_LOGIN_REJECTED);
  do_check_false(Service.isLoggedIn);

  
  Service.startOver();
  server.stop(run_next_test);
});

add_test(function test_credentials_changed_logout() {
  let server = sync_httpd_setup();
  setUp();

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();
  Service.sync();

  do_check_eq(Status.sync, CREDENTIALS_CHANGED);
  do_check_false(Service.isLoggedIn);

  
  Service.startOver();
  server.stop(run_next_test);
});

add_test(function test_no_lastSync_pref() {
  
  Status.resetSync();
  ErrorHandler.dontIgnoreErrors = true;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  ErrorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(ErrorHandler.shouldReportError());

  run_next_test();
});

add_test(function test_shouldReportError() {
  Status.login = MASTER_PASSWORD_LOCKED;
  do_check_false(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = true;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NO_PASSWORD;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.sync = CREDENTIALS_CHANGED;
  do_check_true(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.login = LOGIN_FAILED_NETWORK_ERROR;
  do_check_false(ErrorHandler.shouldReportError());

  
  Status.resetSync();
  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.dontIgnoreErrors = false;
  Status.sync = LOGIN_FAILED_NETWORK_ERROR;
  do_check_false(ErrorHandler.shouldReportError());

  run_next_test();
});

add_test(function test_shouldReportError_master_password() {
  _("Test error ignored due to locked master password");
  let server = sync_httpd_setup();
  setUp();

  
  
  Service._verifyLogin = Service.verifyLogin;
  Service.verifyLogin = function () {
    Status.login = MASTER_PASSWORD_LOCKED;
    return false;
  };

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
  do_check_false(ErrorHandler.shouldReportError());

  
  Service.verifyLogin = Service._verifyLogin;
  Service.startOver();
  server.stop(run_next_test);
});

add_test(function test_login_syncAndReportErrors_non_network_error() {
  
  
  let server = sync_httpd_setup();
  setUp();
  Service.password = "";

  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NO_PASSWORD);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});

add_test(function test_sync_syncAndReportErrors_non_network_error() {
  
  
  let server = sync_httpd_setup();
  setUp();

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, CREDENTIALS_CHANGED);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});

add_test(function test_login_syncAndReportErrors_prolonged_non_network_error() {
  
  
  let server = sync_httpd_setup();
  setUp();
  Service.password = "";

  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});

add_test(function test_sync_syncAndReportErrors_prolonged_non_network_error() {
  
  
  let server = sync_httpd_setup();
  setUp();

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});

add_test(function test_login_syncAndReportErrors_network_error() {
  
  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.clusterURL = "http://localhost:8080/";

  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NETWORK_ERROR);

    Service.startOver();
    run_next_test();
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});


add_test(function test_sync_syncAndReportErrors_network_error() {
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, LOGIN_FAILED_NETWORK_ERROR);

    Services.io.offline = false;
    Service.startOver();
    run_next_test();
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});

add_test(function test_login_syncAndReportErrors_prolonged_network_error() {
  
  
  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.clusterURL = "http://localhost:8080/";

  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Service.startOver();
    run_next_test();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});

add_test(function test_sync_syncAndReportErrors_prolonged_network_error() {
  
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Services.io.offline = false;
    Service.startOver();
    run_next_test();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  ErrorHandler.syncAndReportErrors();
});

add_test(function test_login_prolonged_non_network_error() {
  
  let server = sync_httpd_setup();
  setUp();
  Service.password = "";

  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_test(function test_sync_prolonged_non_network_error() {
  
  let server = sync_httpd_setup();
  setUp();

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_test(function test_login_prolonged_network_error() {
  
  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.clusterURL = "http://localhost:8080/";

  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Service.startOver();
    run_next_test();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_test(function test_sync_prolonged_network_error() {
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, PROLONGED_SYNC_FAILURE);

    Services.io.offline = false;
    Service.startOver();
    run_next_test();
  });

  setLastSync(PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_test(function test_login_non_network_error() {
  
  let server = sync_httpd_setup();
  setUp();
  Service.password = "";

  Svc.Obs.add("weave:ui:login:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:login:error", onSyncError);
    do_check_eq(Status.login, LOGIN_FAILED_NO_PASSWORD);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
});


add_test(function test_sync_non_network_error() {
  
  let server = sync_httpd_setup();
  setUp();

  
  Service.sync();
  do_check_eq(Status.sync, SYNC_SUCCEEDED);
  do_check_true(Service.isLoggedIn);

  generateCredentialsChangedFailure();

  Svc.Obs.add("weave:ui:sync:error", function onSyncError() {
    Svc.Obs.remove("weave:ui:sync:error", onSyncError);
    do_check_eq(Status.sync, CREDENTIALS_CHANGED);

    Service.startOver();
    server.stop(run_next_test);
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_test(function test_login_network_error() {
  Service.username = "johndoe";
  Service.password = "ilovejane";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.clusterURL = "http://localhost:8080/";

  Svc.Obs.add("weave:ui:login:error", function() {
    do_throw("Should not get here!");
  });

  
  Svc.Obs.add("weave:service:login:error", function onUIUpdate() {
    Svc.Obs.remove("weave:service:login:error", onUIUpdate);

    
    
    Utils.nextTick(function() {
      do_check_eq(Status.login, LOGIN_FAILED_NETWORK_ERROR);

      Service.startOver();
      Services.io.offline = false;
      run_next_test();
    });
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_test(function test_sync_network_error() {
  
  Services.io.offline = true;

  Svc.Obs.add("weave:ui:sync:finish", function onUIUpdate() {
    Svc.Obs.remove("weave:ui:sync:finish", onUIUpdate);
    do_check_eq(Status.sync, LOGIN_FAILED_NETWORK_ERROR);

    Service.startOver();
    Services.io.offline = false;
    Status.resetSync();
    run_next_test();
  });

  setLastSync(NON_PROLONGED_ERROR_DURATION);
  Service.sync();
});

add_test(function test_sync_engine_generic_fail() {
  let server = sync_httpd_setup();

  let engine = Engines.get("catapult");
  engine.enabled = true;
  engine.sync = function sync() {
    Svc.Obs.notify("weave:engine:sync:error", "", "steam");
  };

  let log = Log4Moz.repository.getLogger("Sync.ErrorHandler");
  Svc.Prefs.set("log.appender.file.logOnError", true);

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    
    let entries = logsdir.directoryEntries;
    do_check_true(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    do_check_eq(logfile.leafName.slice(0, LOG_PREFIX_ERROR.length),
                LOG_PREFIX_ERROR);

    Status.resetSync();
    Service.startOver();

    server.stop(run_next_test);
  });

  do_check_eq(Status.engines["steam"], undefined);

  do_check_true(setUp());

  Service.sync();

  do_check_eq(Status.engines["steam"], ENGINE_UNKNOWN_FAIL);
  do_check_eq(Status.service, SYNC_FAILED_PARTIAL);
});



add_test(function test_engine_applyFailed() {
  let server = sync_httpd_setup();

  let engine = Engines.get("catapult");
  engine.enabled = true;
  delete engine.exception;
  engine.sync = function sync() {
    Svc.Obs.notify("weave:engine:sync:applied", {newFailed:1}, "steam");
  };

  let log = Log4Moz.repository.getLogger("Sync.ErrorHandler");
  Svc.Prefs.set("log.appender.file.logOnError", true);

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    
    let entries = logsdir.directoryEntries;
    do_check_true(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    do_check_eq(logfile.leafName.slice(0, LOG_PREFIX_ERROR.length),
                LOG_PREFIX_ERROR);

    Status.resetSync();
    Service.startOver();

    server.stop(run_next_test);
  });

  do_check_eq(Status.engines["steam"], undefined);

  do_check_true(setUp());

  Service.sync();

  do_check_eq(Status.engines["steam"], ENGINE_APPLY_FAIL);
  do_check_eq(Status.service, SYNC_FAILED_PARTIAL);
});
