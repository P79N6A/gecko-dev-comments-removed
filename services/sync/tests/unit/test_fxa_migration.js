
Cu.import("resource://services-sync/FxaMigrator.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://services-sync/browserid_identity.js");


Services.prefs.setCharPref("services.sync.username", "foo");

Services.prefs.setCharPref("services.sync.log.appender.dump", "Debug");


Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/util.js");


Services.prefs.clearUserPref("services.sync.username");

Cu.import("resource://testing-common/services/sync/utils.js");
Cu.import("resource://testing-common/services/common/logging.js");
Cu.import("resource://testing-common/services/sync/rotaryengine.js");

const FXA_USERNAME = "someone@somewhere";


function promiseOneObserver(topic) {
  return new Promise((resolve, reject) => {
    let observer = function(subject, topic, data) {
      Services.obs.removeObserver(observer, topic);
      resolve({ subject: subject, data: data });
    }
    Services.obs.addObserver(observer, topic, false);
  });
}

function promiseStopServer(server) {
  return new Promise((resolve, reject) => {
    server.stop(resolve);
  });
}



function configureLegacySync() {
  let engine = new RotaryEngine(Service);
  engine.enabled = true;
  Svc.Prefs.set("registerEngines", engine.name);
  Svc.Prefs.set("log.logger.engine.rotary", "Trace");

  let contents = {
    meta: {global: {engines: {rotary: {version: engine.version,
                                       syncID:  engine.syncID}}}},
    crypto: {},
    rotary: {}
  };

  const USER = "foo";
  const PASSPHRASE = "abcdeabcdeabcdeabcdeabcdea";

  setBasicCredentials(USER, "password", PASSPHRASE);

  let onRequest = function(request, response) {
    
    response.setHeader("x-weave-alert", JSON.stringify({code: "soft-eol"}));
  }
  let server = new SyncServer({onRequest: onRequest});
  server.registerUser(USER, "password");
  server.createContents(USER, contents);
  server.start();

  Service.serverURL = server.baseURI;
  Service.clusterURL = server.baseURI;
  Service.identity.username = USER;
  Service._updateCachedURLs();

  Service.engineManager._engines[engine.name] = engine;

  return [engine, server];
}

function configureFxa() {
  Services.prefs.setCharPref("identity.fxaccounts.auth.uri", "http://localhost");
}

add_task(function *testMigration() {
  configureFxa();

  
  let oldValue = Services.prefs.getBoolPref("services.sync-testing.startOverKeepIdentity");
  Services.prefs.setBoolPref("services.sync-testing.startOverKeepIdentity", false);

  
  
  Services.prefs.setBoolPref("services.sync.engine.addons", false);

  do_register_cleanup(() => {
    Services.prefs.setBoolPref("services.sync-testing.startOverKeepIdentity", oldValue)
    Services.prefs.setBoolPref("services.sync.engine.addons", true);
  });

  
  Assert.deepEqual((yield fxaMigrator._queueCurrentUserState()), null,
                   "no user state when complete");

  
  let [engine, server] = configureLegacySync();

  
  
  Assert.ok(!Service.engineManager.get("addons").enabled, "addons is disabled");
  Assert.ok(Service.engineManager.get("passwords").enabled, "passwords is enabled");

  
  let haveStartedSentinel = false;
  let origSetFxAMigrationSentinel = Service.setFxAMigrationSentinel;
  let promiseSentinelWritten = new Promise((resolve, reject) => {
    Service.setFxAMigrationSentinel = function(arg) {
      haveStartedSentinel = true;
      return origSetFxAMigrationSentinel.call(Service, arg).then(result => {
        Service.setFxAMigrationSentinel = origSetFxAMigrationSentinel;
        resolve(result);
        return result;
      });
    }
  });

  
  
  Assert.deepEqual((yield fxaMigrator._queueCurrentUserState()), null,
                   "no user state before server EOL");

  
  
  let promise = promiseOneObserver("fxa-migration:state-changed");
  _("Starting sync");
  Service.sync();
  _("Finished sync");

  
  Assert.equal((yield promise).data, fxaMigrator.STATE_USER_FXA, "now waiting for FxA.")

  
  Assert.equal((yield fxaMigrator._queueCurrentUserState()),
               fxaMigrator.STATE_USER_FXA,
               "still waiting for FxA.");

  
  let config = makeIdentityConfig({username: FXA_USERNAME});
  let fxa = new FxAccounts({});
  config.fxaccount.user.email = config.username;
  delete config.fxaccount.user.verified;
  
  fxa.internal.currentAccountState.getCertificate = function(data, keyPair, mustBeValidUntil) {
    this.cert = {
      validUntil: fxa.internal.now() + CERT_LIFETIME,
      cert: "certificate",
    };
    return Promise.resolve(this.cert.cert);
  };

  
  
  promise = promiseOneObserver("fxa-migration:state-changed");
  fxAccounts.setSignedInUser(config.fxaccount.user);

  let observerInfo = yield promise;
  Assert.equal(observerInfo.data,
               fxaMigrator.STATE_USER_FXA_VERIFIED,
               "now waiting for verification");
  Assert.ok(observerInfo.subject instanceof Ci.nsISupportsString,
            "email was passed to observer");
  Assert.equal(observerInfo.subject.data,
               FXA_USERNAME,
               "email passed to observer is correct");

  
  Assert.equal((yield fxaMigrator._queueCurrentUserState()),
               fxaMigrator.STATE_USER_FXA_VERIFIED,
               "now waiting for verification");

  
  
  

  let wasWaiting = false;
  
  engine._syncFinish = function () {
    
    function getState() {
      let cb = Async.makeSpinningCallback();
      fxaMigrator._queueCurrentUserState().then(state => cb(null, state));
      return cb.wait();
    }
    
    Assert.equal(getState(), fxaMigrator.STATE_USER_FXA_VERIFIED,
                 "still waiting for verification");

    
    
    config.fxaccount.user.verified = true;
    fxAccounts.setSignedInUser(config.fxaccount.user);
    Services.obs.notifyObservers(null, ONVERIFIED_NOTIFICATION, null);

    
    
    
    let cb = Async.makeSpinningCallback();
    promiseOneObserver("fxa-migration:state-changed").then(({ data: state }) => cb(null, state));
    Assert.equal(cb.wait(), null, "no user action necessary while sync completes.");

    
    Assert.ok(!haveStartedSentinel, "haven't written a sentinel yet");

    
    Assert.ok(Service.scheduler.isBlocked, "sync is blocked.")

    wasWaiting = true;
    throw ex;
  };

  _("Starting sync");
  Service.sync();
  _("Finished sync");

  
  
  
  let promiseFinalSync = new Promise((resolve, reject) => {
    let oldSync = Service.sync;
    Service.sync = function() {
      Service.sync = oldSync;
      resolve();
    }
  });

  Assert.ok(wasWaiting, "everything was good while sync was running.")

  
  
  Assert.ok(Service.scheduler.isBlocked, "sync is blocked.");

  
  Assert.ok((yield promiseSentinelWritten), "wrote the sentinel");

  
  yield promiseFinalSync;

  
  let WeaveService = Cc["@mozilla.org/weave/service;1"]
         .getService(Components.interfaces.nsISupports)
         .wrappedJSObject;
  Assert.ok(WeaveService.fxAccountsEnabled, "FxA is enabled");
  Assert.ok(Service.identity instanceof BrowserIDManager,
            "sync is configured with the browserid_identity provider.");
  Assert.equal(Service.identity.username, config.username, "correct user configured")
  Assert.ok(!Service.scheduler.isBlocked, "sync is not blocked.")
  
  Assert.deepEqual((yield fxaMigrator._queueCurrentUserState()),
                   null,
                   "still no user action necessary");
  
  Assert.ok(!Service.engineManager.get("addons").enabled, "addons is still disabled");
  Assert.ok(Service.engineManager.get("passwords").enabled, "passwords is still enabled");

  
  yield promiseStopServer(server);
});



add_task(function* testTokenServerOldPrefName() {
  let value = "http://custom-token-server/";
  
  Services.prefs.setCharPref("services.sync.tokenServerURI", value);
  
  
  Assert.notEqual(Services.prefs.getCharPref("identity.sync.tokenserver.uri"), value);

  let prefs = fxaMigrator._getSentinelPrefs();
  Assert.equal(prefs["services.sync.tokenServerURI"], value);
  
  Services.prefs.clearUserPref("services.sync.tokenServerURI");
  Assert.ok(!Services.prefs.prefHasUserValue("services.sync.tokenServerURI"));
  fxaMigrator._applySentinelPrefs(prefs);
  
  Assert.equal(Services.prefs.getCharPref("identity.sync.tokenserver.uri"), value);
  
  Assert.ok(!Services.prefs.prefHasUserValue("services.sync.tokenServerURI"));
});

add_task(function* testTokenServerNewPrefName() {
  let value = "http://token-server/";
  
  Services.prefs.setCharPref("identity.sync.tokenserver.uri", value);

  let prefs = fxaMigrator._getSentinelPrefs();
  
  Assert.equal(prefs["services.sync.tokenServerURI"], value);
  
  Services.prefs.clearUserPref("services.sync.tokenServerURI");
  Assert.ok(!Services.prefs.prefHasUserValue("services.sync.tokenServerURI"));
  fxaMigrator._applySentinelPrefs(prefs);
  
  Assert.equal(Services.prefs.getCharPref("identity.sync.tokenserver.uri"), value);
  
  Assert.ok(!Services.prefs.prefHasUserValue("services.sync.tokenServerURI"));
});

function run_test() {
  initTestLogging();
  do_register_cleanup(() => {
    fxaMigrator.finalize();
    Svc.Prefs.resetBranch("");
  });
  run_next_test();
}
