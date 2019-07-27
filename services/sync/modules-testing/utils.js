



"use strict";

this.EXPORTED_SYMBOLS = [
  "btoa", 
  "encryptPayload",
  "ensureLegacyIdentityManager",
  "setBasicCredentials",
  "makeIdentityConfig",
  "configureFxAccountIdentity",
  "configureIdentity",
  "SyncTestingInfrastructure",
  "waitForZeroTimer",
  "Promise", 
  "add_identity_test",
];

const {utils: Cu} = Components;

Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/browserid_identity.js");
Cu.import("resource://testing-common/services/common/logging.js");
Cu.import("resource://testing-common/services/sync/fakeservices.js");
Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/Promise.jsm");






this.waitForZeroTimer = function waitForZeroTimer(callback) {
  let ticks = 2;
  function wait() {
    if (ticks) {
      ticks -= 1;
      CommonUtils.nextTick(wait);
      return;
    }
    callback();
  }
  CommonUtils.namedTimer(wait, 150, {}, "timer");
}




this.ensureLegacyIdentityManager = function() {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  Status.__authManager = ns.Service.identity = new IdentityManager();
  ns.Service._clusterManager = ns.Service.identity.createClusterManager(ns.Service);
}

this.setBasicCredentials =
 function setBasicCredentials(username, password, syncKey) {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  let auth = ns.Service.identity;
  auth.username = username;
  auth.basicPassword = password;
  auth.syncKey = syncKey;
}



this.makeIdentityConfig = function(overrides) {
  
  let result = {
    
    username: "foo",
    
    fxaccount: {
      user: {
        assertion: 'assertion',
        email: 'email',
        kA: 'kA',
        kB: 'kB',
        sessionToken: 'sessionToken',
        uid: 'user_uid',
        verified: true,
      },
      token: {
        endpoint: Svc.Prefs.get("tokenServerURI"),
        duration: 300,
        id: "id",
        key: "key",
        
      }
    },
    sync: {
      
      password: "whatever",
      syncKey: "abcdeabcdeabcdeabcdeabcdea",
    }
  };

  
  if (overrides) {
    if (overrides.username) {
      result.username = overrides.username;
    }
    if (overrides.sync) {
      
      result.sync = overrides.sync;
    }
    if (overrides.fxaccount) {
      
      result.fxaccount = overrides.fxaccount;
    }
  }
  return result;
}



this.configureFxAccountIdentity = function(authService,
                                           config = makeIdentityConfig()) {
  let MockInternal = {};
  let fxa = new FxAccounts(MockInternal);

  
  
  config.fxaccount.user.email = config.username;
  fxa.internal.currentAccountState.signedInUser = {
    version: DATA_FORMAT_VERSION,
    accountData: config.fxaccount.user
  };
  fxa.internal.currentAccountState.getCertificate = function(data, keyPair, mustBeValidUntil) {
    this.cert = {
      validUntil: fxa.internal.now() + CERT_LIFETIME,
      cert: "certificate",
    };
    return Promise.resolve(this.cert.cert);
  };

  let mockTSC = { 
    getTokenFromBrowserIDAssertion: function(uri, assertion, cb) {
      config.fxaccount.token.uid = config.username;
      cb(null, config.fxaccount.token);
    },
  };
  authService._fxaService = fxa;
  authService._tokenServerClient = mockTSC;
  
  
  authService._signedInUser = fxa.internal.currentAccountState.signedInUser.accountData;
  authService._account = config.fxaccount.user.email;
}

this.configureIdentity = function(identityOverrides) {
  let config = makeIdentityConfig(identityOverrides);
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  if (ns.Service.identity instanceof BrowserIDManager) {
    
    configureFxAccountIdentity(ns.Service.identity, config);
    return ns.Service.identity.initializeWithCurrentIdentity().then(() => {
      
      return ns.Service.identity.whenReadyToAuthenticate.promise;
    });
  }
  
  setBasicCredentials(config.username, config.sync.password, config.sync.syncKey);
  let deferred = Promise.defer();
  deferred.resolve();
  return deferred.promise;
}

this.SyncTestingInfrastructure = function (server, username, password, syncKey) {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  ensureLegacyIdentityManager();
  let config = makeIdentityConfig();
  
  if (username)
    config.username = username;
  if (password)
    config.sync.password = password;
  if (syncKey)
    config.sync.syncKey = syncKey;
  let cb = Async.makeSpinningCallback();
  configureIdentity(config).then(cb, cb);
  cb.wait();

  let i = server.identity;
  let uri = i.primaryScheme + "://" + i.primaryHost + ":" +
            i.primaryPort + "/";

  ns.Service.serverURL = uri;
  ns.Service.clusterURL = uri;

  this.logStats = initTestLogging();
  this.fakeFilesystem = new FakeFilesystemService({});
  this.fakeGUIDService = new FakeGUIDService();
  this.fakeCryptoService = new FakeCryptoService();
}




this.encryptPayload = function encryptPayload(cleartext) {
  if (typeof cleartext == "object") {
    cleartext = JSON.stringify(cleartext);
  }

  return {
    ciphertext: cleartext, 
    IV: "irrelevant",
    hmac: fakeSHA256HMAC(cleartext, CryptoUtils.makeHMACKey("")),
  };
}











this.add_identity_test = function(test, testFunction) {
  function note(what) {
    let msg = "running test " + testFunction.name + " with " + what + " identity manager";
    test.do_print(msg);
  }
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);
  
  test.add_task(function() {
    note("sync");
    let oldIdentity = Status._authManager;
    ensureLegacyIdentityManager();
    yield testFunction();
    Status.__authManager = ns.Service.identity = oldIdentity;
  });
  
  test.add_task(function() {
    note("FxAccounts");
    let oldIdentity = Status._authManager;
    Status.__authManager = ns.Service.identity = new BrowserIDManager();
    yield testFunction();
    Status.__authManager = ns.Service.identity = oldIdentity;
  });
}
