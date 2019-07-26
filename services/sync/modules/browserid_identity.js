



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserIDManager"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://services-sync/stages/cluster.js");
Cu.import("resource://gre/modules/FxAccounts.jsm");


XPCOMUtils.defineLazyModuleGetter(this, "Weave",
                                  "resource://services-sync/main.js");

XPCOMUtils.defineLazyModuleGetter(this, "BulkKeyBundle",
                                  "resource://services-sync/keys.js");

XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts",
                                  "resource://gre/modules/FxAccounts.jsm");

XPCOMUtils.defineLazyGetter(this, 'log', function() {
  let log = Log.repository.getLogger("Sync.BrowserIDManager");
  log.addAppender(new Log.DumpAppender());
  log.level = Log.Level[Svc.Prefs.get("log.logger.identity")] || Log.Level.Error;
  return log;
});


let fxAccountsCommon = {};
Cu.import("resource://gre/modules/FxAccountsCommon.js", fxAccountsCommon);

const OBSERVER_TOPICS = [
  fxAccountsCommon.ONLOGIN_NOTIFICATION,
  fxAccountsCommon.ONLOGOUT_NOTIFICATION,
];

const PREF_SYNC_SHOW_CUSTOMIZATION = "services.sync.ui.showCustomizationDialog";

function deriveKeyBundle(kB) {
  let out = CryptoUtils.hkdf(kB, undefined,
                             "identity.mozilla.com/picl/v1/oldsync", 2*32);
  let bundle = new BulkKeyBundle();
  
  bundle.keyPair = [out.slice(0, 32), out.slice(32, 64)];
  return bundle;
}








function AuthenticationError(details) {
  this.details = details;
}

AuthenticationError.prototype = {
  toString: function() {
    return "AuthenticationError(" + this.details + ")";
  }
}

this.BrowserIDManager = function BrowserIDManager() {
  
  
  this._fxaService = fxAccounts;
  this._tokenServerClient = new TokenServerClient();
  
  this.whenReadyToAuthenticate = null;
  this._log = log;
};

this.BrowserIDManager.prototype = {
  __proto__: IdentityManager.prototype,

  _fxaService: null,
  _tokenServerClient: null,
  
  _token: null,
  _signedInUser: null, 

  
  
  
  _authFailureReason: null,

  
  
  _shouldHaveSyncKeyBundle: false,

  get readyToAuthenticate() {
    
    
    return this._shouldHaveSyncKeyBundle;
  },

  get needsCustomization() {
    try {
      return Services.prefs.getBoolPref(PREF_SYNC_SHOW_CUSTOMIZATION);
    } catch (e) {
      return false;
    }
  },

  initialize: function() {
    for (let topic of OBSERVER_TOPICS) {
      Services.obs.addObserver(this, topic, false);
    }
    return this.initializeWithCurrentIdentity();
  },

  



  ensureLoggedIn: function() {
    if (!this._shouldHaveSyncKeyBundle) {
      
      return this.whenReadyToAuthenticate.promise;
    }

    
    if (Weave.Status.login == LOGIN_SUCCEEDED) {
      return Promise.resolve();
    }

    
    
    
    if (Weave.Status.login == LOGIN_FAILED_LOGIN_REJECTED) {
      return Promise.reject();
    }

    
    
    this.initializeWithCurrentIdentity();
    return this.whenReadyToAuthenticate.promise;
  },

  finalize: function() {
    
    for (let topic of OBSERVER_TOPICS) {
      Services.obs.removeObserver(this, topic);
    }
    this.resetCredentials();
    this._signedInUser = null;
    return Promise.resolve();
  },

  initializeWithCurrentIdentity: function(isInitialSync=false) {
    
    
    
    this._log.trace("initializeWithCurrentIdentity");

    
    this.whenReadyToAuthenticate = Promise.defer();
    this._shouldHaveSyncKeyBundle = false;
    this._authFailureReason = null;

    return this._fxaService.getSignedInUser().then(accountData => {
      if (!accountData) {
        this._log.info("initializeWithCurrentIdentity has no user logged in");
        this.account = null;
        
        this._shouldHaveSyncKeyBundle = true;
        this.whenReadyToAuthenticate.reject("no user is logged in");
        return;
      }

      this.account = accountData.email;
      this._updateSignedInUser(accountData);
      
      
      
      this._log.info("Waiting for user to be verified.");
      this._fxaService.whenVerified(accountData).then(accountData => {
        this._updateSignedInUser(accountData);
        this._log.info("Starting fetch for key bundle.");
        if (this.needsCustomization) {
          
          
          const url = "chrome://browser/content/sync/customize.xul";
          const features = "centerscreen,chrome,modal,dialog,resizable=no";
          let win = Services.wm.getMostRecentWindow("navigator:browser");

          let data = {accepted: false};
          win.openDialog(url, "_blank", features, data);

          if (data.accepted) {
            Services.prefs.clearUserPref(PREF_SYNC_SHOW_CUSTOMIZATION);
          } else {
            
            return this._fxaService.signOut();
          }
        }
      }).then(() => {
        return this._fetchSyncKeyBundle();
      }).then(() => {
        this._shouldHaveSyncKeyBundle = true; 
        this.whenReadyToAuthenticate.resolve();
        this._log.info("Background fetch for key bundle done");
        Weave.Status.login = LOGIN_SUCCEEDED;
        if (isInitialSync) {
          this._log.info("Doing initial sync actions");
          Svc.Prefs.set("firstSync", "resetClient");
          Services.obs.notifyObservers(null, "weave:service:setup-complete", null);
          Weave.Utils.nextTick(Weave.Service.sync, Weave.Service);
        }
      }).then(null, err => {
        this._shouldHaveSyncKeyBundle = true; 
        this.whenReadyToAuthenticate.reject(err);
        
        this._log.error("Background fetch for key bundle failed: " + err);
      });
      
    }).then(null, err => {
      this._log.error("Processing logged in account: " + err);
    });
  },

  _updateSignedInUser: function(userData) {
    
    
    
    
    if (this._signedInUser && this._signedInUser.email != userData.email) {
      throw new Error("Attempting to update to a different user.")
    }
    this._signedInUser = userData;
  },

  logout: function() {
    
    
    
    
    
    this._token = null;
  },

  observe: function (subject, topic, data) {
    this._log.debug("observed " + topic);
    switch (topic) {
    case fxAccountsCommon.ONLOGIN_NOTIFICATION:
      
      
      
      
      
      
      
      
      this.initializeWithCurrentIdentity(true);
      break;

    case fxAccountsCommon.ONLOGOUT_NOTIFICATION:
      Weave.Service.startOver();
      
      
      break;
    }
  },

  


  _sha256: function(message) {
    let hasher = Cc["@mozilla.org/security/hash;1"]
                    .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA256);
    return CryptoUtils.digestBytes(message, hasher);
  },

  




  _computeXClientState: function(kBbytes) {
    return CommonUtils.bytesAsHex(this._sha256(kBbytes).slice(0, 16), false);
  },

  


  _now: function() {
    return this._fxaService.now()
  },

  get _localtimeOffsetMsec() {
    return this._fxaService.localtimeOffsetMsec;
  },

  usernameFromAccount: function(val) {
    
    return val;
  },

  




  get basicPassword() {
    this._log.error("basicPassword getter should be not used in BrowserIDManager");
    return null;
  },

  




  set basicPassword(value) {
    throw "basicPassword setter should be not used in BrowserIDManager";
  },

  








  get syncKey() {
    if (this.syncKeyBundle) {
      
      
      
      
      
      
      return "99999999999999999999999999";
    }
    else {
      return null;
    }
  },

  set syncKey(value) {
    throw "syncKey setter should be not used in BrowserIDManager";
  },

  get syncKeyBundle() {
    return this._syncKeyBundle;
  },

  


  resetCredentials: function() {
    this.resetSyncKey();
    this._token = null;
  },

  


  resetSyncKey: function() {
    this._syncKey = null;
    this._syncKeyBundle = null;
    this._syncKeyUpdated = true;
    this._shouldHaveSyncKeyBundle = false;
  },

  





  get currentAuthState() {
    if (this._authFailureReason) {
      this._log.info("currentAuthState returning " + this._authFailureReason +
                     " due to previous failure");
      return this._authFailureReason;
    }
    
    
    
    if (!this.username) {
      return LOGIN_FAILED_NO_USERNAME;
    }

    
    
    
    if (this._shouldHaveSyncKeyBundle && !this.syncKeyBundle) {
      return LOGIN_FAILED_NO_PASSPHRASE;
    }

    return STATUS_OK;
  },

  



  hasValidToken: function() {
    if (!this._token) {
      return false;
    }
    if (this._token.expiration < this._now()) {
      return false;
    }
    return true;
  },

  _fetchSyncKeyBundle: function() {
    
    return this._fxaService.getKeys().then(userData => {
      this._updateSignedInUser(userData); 
      return this._fetchTokenForUser().then(token => {
        this._token = token;
        
        let kB = Utils.hexToBytes(userData.kB);
        this._syncKeyBundle = deriveKeyBundle(kB);
        return;
      });
    });
  },

  
  _fetchTokenForUser: function() {
    let tokenServerURI = Svc.Prefs.get("tokenServerURI");
    let log = this._log;
    let client = this._tokenServerClient;
    let fxa = this._fxaService;
    let userData = this._signedInUser;

    
    let kBbytes = CommonUtils.hexToBytes(userData.kB);
    let headers = {"X-Client-State": this._computeXClientState(kBbytes)};
    log.info("Fetching assertion and token from: " + tokenServerURI);

    function getToken(tokenServerURI, assertion) {
      log.debug("Getting a token");
      let deferred = Promise.defer();
      let cb = function (err, token) {
        if (err) {
          log.info("TokenServerClient.getTokenFromBrowserIDAssertion() failed with: " + err);
          if (err.response && err.response.status === 401) {
            err = new AuthenticationError(err);
          }
          return deferred.reject(err);
        } else {
          log.debug("Successfully got a sync token");
          return deferred.resolve(token);
        }
      };

      client.getTokenFromBrowserIDAssertion(tokenServerURI, assertion, cb, headers);
      return deferred.promise;
    }

    function getAssertion() {
      log.debug("Getting an assertion");
      let audience = Services.io.newURI(tokenServerURI, null, null).prePath;
      return fxa.getAssertion(audience).then(null, err => {
        log.error("fxa.getAssertion() failed with: " + err.code + " - " + err.message);
        if (err.code === 401) {
          throw new AuthenticationError("Unable to get assertion for user");
        } else {
          throw err;
        }
      });
    };

    
    
    return fxa.whenVerified(this._signedInUser)
      .then(() => getAssertion())
      .then(assertion => getToken(tokenServerURI, assertion))
      .then(token => {
        
        
        
        token.expiration = this._now() + (token.duration * 1000) * 0.80;
        return token;
      })
      .then(null, err => {
        
        
        
        if (err instanceof AuthenticationError) {
          this._log.error("Authentication error in _fetchTokenForUser: " + err);
          
          this._authFailureReason = LOGIN_FAILED_LOGIN_REJECTED;
        } else {
          this._log.error("Non-authentication error in _fetchTokenForUser: " + err.message);
          
          this._authFailureReason = LOGIN_FAILED_NETWORK_ERROR;
        }
        
        
        
        this._shouldHaveSyncKeyBundle = true;
        this._syncKeyBundle = null;
        Weave.Status.login = this._authFailureReason;
        Services.obs.notifyObservers(null, "weave:service:login:error", null);
        throw err;
      });
  },

  
  
  _ensureValidToken: function() {
    if (this.hasValidToken()) {
      return Promise.resolve();
    }
    return this._fetchTokenForUser().then(
      token => {
        this._token = token;
      }
    );
  },

  getResourceAuthenticator: function () {
    return this._getAuthenticationHeader.bind(this);
  },

  


  getRESTRequestAuthenticator: function() {
    return this._addAuthenticationHeader.bind(this);
  },

  



  _getAuthenticationHeader: function(httpObject, method) {
    let cb = Async.makeSpinningCallback();
    this._ensureValidToken().then(cb, cb);
    try {
      cb.wait();
    } catch (ex) {
      this._log.error("Failed to fetch a token for authentication: " + ex);
      return null;
    }
    if (!this._token) {
      return null;
    }
    let credentials = {algorithm: "sha256",
                       id: this._token.id,
                       key: this._token.key,
                      };
    method = method || httpObject.method;

    
    
    let options = {
      now: this._now(),
      localtimeOffsetMsec: this._localtimeOffsetMsec,
      credentials: credentials,
    };

    let headerValue = CryptoUtils.computeHAWK(httpObject.uri, method, options);
    return {headers: {authorization: headerValue.field}};
  },

  _addAuthenticationHeader: function(request, method) {
    let header = this._getAuthenticationHeader(request, method);
    if (!header) {
      return null;
    }
    request.setHeader("authorization", header.headers.authorization);
    return request;
  },

  createClusterManager: function(service) {
    return new BrowserIDClusterManager(service);
  }

};




function BrowserIDClusterManager(service) {
  ClusterManager.call(this, service);
}

BrowserIDClusterManager.prototype = {
  __proto__: ClusterManager.prototype,

  _findCluster: function() {
    let endPointFromIdentityToken = function() {
      let endpoint = this.identity._token.endpoint;
      
      
      
      if (!endpoint.endsWith("/")) {
        endpoint += "/";
      }
      return endpoint;
    }.bind(this);

    
    let promiseClusterURL = function() {
      return this.identity.whenReadyToAuthenticate.promise.then(
        () => this.identity._ensureValidToken()
      ).then(
        () => endPointFromIdentityToken()
      );
    }.bind(this);

    let cb = Async.makeSpinningCallback();
    promiseClusterURL().then(function (clusterURL) {
      cb(null, clusterURL);
    }).then(null, cb);
    return cb.wait();
  },

  getUserBaseURL: function() {
    
    
    
    
    
    return this.service.clusterURL;
  }
}
