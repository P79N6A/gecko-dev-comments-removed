



"use strict";

this.EXPORTED_SYMBOLS = [
  "btoa", 
  "encryptPayload",
  "setBasicCredentials",
  "makeIdentityConfig",
  "configureFxAccountIdentity",
  "SyncTestingInfrastructure",
  "waitForZeroTimer",
  "Promise", 
];

const {utils: Cu} = Components;

Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services-common/logging.js");
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
        isVerified: true,
      },
      token: {
        endpoint: Svc.Prefs.get("tokenServerURI"),
        duration: 300,
        id: "id",
        key: "key",
        
      }
    }
    
  };
  
  if (overrides) {
    if (overrides.username) {
      result.username = overrides.username;
    }
    
    if (overrides.fxaccount) {
      
      result.fxaccount = overrides.fxaccount;
    }
    return result;
}



this.configureFxAccountIdentity = function(authService,
                                           config = makeIdentityConfig()) {
  let MockInternal = {
    signedInUser: {
      version: DATA_FORMAT_VERSION,
      accountData: config.fxaccount.user
    },
    getCertificate: function(data, keyPair, mustBeValidUntil) {
      this.cert = {
        validUntil: Date.now() + CERT_LIFETIME,
        cert: "certificate",
      };
      return Promise.resolve(this.cert.cert);
    },
  };
  let fxa = new FxAccounts(MockInternal);

  let mockTSC = { 
    getTokenFromBrowserIDAssertion: function(uri, assertion, cb) {
      config.fxaccount.token.uid = config.username;
      cb(null, config.fxaccount.token);
    },
  };
  authService._fxaService = fxa;
  authService._tokenServerClient = mockTSC;
  
  
  authService._account = config.fxaccount.user.email;
}

this.SyncTestingInfrastructure = function (server, username, password, syncKey) {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  let auth = ns.Service.identity;
  auth.account = username || "foo";
  auth.basicPassword = password || "password";
  auth.syncKey = syncKey || "abcdeabcdeabcdeabcdeabcdea";

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

