



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
  log.level = Log.Level[Svc.Prefs.get("log.logger.identity")] || Log.Level.Error;
  return log;
});


let fxAccountsCommon = {};
Cu.import("resource://gre/modules/FxAccountsCommon.js", fxAccountsCommon);

const OBSERVER_TOPICS = [
  fxAccountsCommon.ONLOGIN_NOTIFICATION,
  fxAccountsCommon.ONLOGOUT_NOTIFICATION,
];

const PREF_SYNC_SHOW_CUSTOMIZATION = "services.sync-setup.ui.showCustomizationDialog";

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
  this._tokenServerClient.observerPrefix = "weave:service";
  
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

    
    if (this._syncKeyBundle) {
      return Promise.resolve();
    }

    
    
    
    if (Weave.Status.login == LOGIN_FAILED_LOGIN_REJECTED) {
      return Promise.reject(new Error("User needs to re-authenticate"));
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

  offerSyncOptions: function () {
    
    
    const url = "chrome://browser/content/sync/customize.xul";
    const features = "centerscreen,chrome,modal,dialog,resizable=no";
    let win = Services.wm.getMostRecentWindow("navigator:browser");

    let data = {accepted: false};
    win.openDialog(url, "_blank", features, data);

    return data;
  },

  initializeWithCurrentIdentity: function(isInitialSync=false) {
    
    
    
    this._log.trace("initializeWithCurrentIdentity");

    
    this.whenReadyToAuthenticate = Promise.defer();
    this.whenReadyToAuthenticate.promise.then(null, (err) => {
      this._log.error("Could not authenticate", err);
    });

    
    
    
    
    this.resetCredentials();
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
          let data = this.offerSyncOptions();
          if (data.accepted) {
            Services.prefs.clearUserPref(PREF_SYNC_SHOW_CUSTOMIZATION);

            
            Weave.Service.engineManager.declineDisabled();
          } else {
            
            return this._fxaService.signOut();
          }
        }
      }).then(() => {
        return this._fetchTokenForUser();
      }).then(token => {
        this._token = token;
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
        
        this._log.error("Background fetch for key bundle failed", err);
      });
      
    }).then(null, err => {
      this._log.error("Processing logged in account", err);
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

  





  prefetchMigrationSentinel: function(service) {
    
  },

  


  _getSyncCredentialsHosts: function() {
    return Utils.getSyncCredentialsHostsFxA();
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

    return STATUS_OK;
  },

  
  
  _canFetchKeys: function() {
    let userData = this._signedInUser;
    
    
    return userData && (userData.keyFetchToken || (userData.kA && userData.kB));
  },

  





  unlockAndVerifyAuthState: function() {
    if (this._canFetchKeys()) {
      log.debug("unlockAndVerifyAuthState already has (or can fetch) sync keys");
      return Promise.resolve(STATUS_OK);
    }
    
    if (!Utils.ensureMPUnlocked()) {
      
      log.debug("unlockAndVerifyAuthState: user declined to unlock master-password");
      return Promise.resolve(MASTER_PASSWORD_LOCKED);
    }
    
    
    return this._fxaService.getSignedInUser().then(
      accountData => {
        this._updateSignedInUser(accountData);
        
        
        
        let result = this._canFetchKeys() ? STATUS_OK : LOGIN_FAILED_LOGIN_REJECTED;
        log.debug("unlockAndVerifyAuthState re-fetched credentials and is returning", result);
        return result;
      }
    );
  },

  



  hasValidToken: function() {
    
    
    let ignoreCachedAuthCredentials = false;
    try {
      ignoreCachedAuthCredentials = Svc.Prefs.get("debug.ignoreCachedAuthCredentials");
    } catch(e) {
      
    }
    if (ignoreCachedAuthCredentials) {
      return false;
    }
    if (!this._token) {
      return false;
    }
    if (this._token.expiration < this._now()) {
      return false;
    }
    return true;
  },

  
  
  
  _fetchTokenForUser: function() {
    let tokenServerURI = Svc.Prefs.get("tokenServerURI");
    if (tokenServerURI.endsWith("/")) { 
      tokenServerURI = tokenServerURI.slice(0, -1);
    }
    let log = this._log;
    let client = this._tokenServerClient;
    let fxa = this._fxaService;
    let userData = this._signedInUser;

    
    
    
    if (!this._canFetchKeys()) {
      log.info("Unable to fetch keys (master-password locked?), so aborting token fetch");
      return Promise.resolve(null);
    }

    let maybeFetchKeys = () => {
      
      
      if (userData.kA && userData.kB) {
        return;
      }
      log.info("Fetching new keys");
      return this._fxaService.getKeys().then(
        newUserData => {
          userData = newUserData;
          this._updateSignedInUser(userData); 
        }
      );
    }

    let getToken = (tokenServerURI, assertion) => {
      log.debug("Getting a token");
      let deferred = Promise.defer();
      let cb = function (err, token) {
        if (err) {
          return deferred.reject(err);
        }
        log.debug("Successfully got a sync token");
        return deferred.resolve(token);
      };

      let kBbytes = CommonUtils.hexToBytes(userData.kB);
      let headers = {"X-Client-State": this._computeXClientState(kBbytes)};
      client.getTokenFromBrowserIDAssertion(tokenServerURI, assertion, cb, headers);
      return deferred.promise;
    }

    let getAssertion = () => {
      log.info("Getting an assertion from", tokenServerURI);
      let audience = Services.io.newURI(tokenServerURI, null, null).prePath;
      return fxa.getAssertion(audience);
    };

    
    
    return fxa.whenVerified(this._signedInUser)
      .then(() => maybeFetchKeys())
      .then(() => getAssertion())
      .then(assertion => getToken(tokenServerURI, assertion))
      .then(token => {
        
        
        
        token.expiration = this._now() + (token.duration * 1000) * 0.80;
        if (!this._syncKeyBundle) {
          
          this._syncKeyBundle = deriveKeyBundle(Utils.hexToBytes(userData.kB));
        }
        return token;
      })
      .then(null, err => {
        
        
        
        if (err.response && err.response.status === 401) {
          err = new AuthenticationError(err);
        
        } else if (err.code && err.code === 401) {
          err = new AuthenticationError(err);
        }

        
        
        
        if (err instanceof AuthenticationError) {
          this._log.error("Authentication error in _fetchTokenForUser", err);
          
          this._authFailureReason = LOGIN_FAILED_LOGIN_REJECTED;
        } else {
          this._log.error("Non-authentication error in _fetchTokenForUser", err);
          
          
          this._authFailureReason = LOGIN_FAILED_NETWORK_ERROR;
        }
        
        
        
        
        this._shouldHaveSyncKeyBundle = true;
        Weave.Status.login = this._authFailureReason;
        Services.obs.notifyObservers(null, "weave:ui:login:error", null);
        throw err;
      });
  },

  
  
  _ensureValidToken: function() {
    if (this.hasValidToken()) {
      this._log.debug("_ensureValidToken already has one");
      return Promise.resolve();
    }
    
    
    this._token = null;
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
      this._log.error("Failed to fetch a token for authentication", ex);
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
      
      
      
      
      
      
      
      
      if (!this.identity._token) {
        throw new Error("Can't get a cluster URL as we can't fetch keys.");
      }
      let endpoint = this.identity._token.endpoint;
      
      
      
      if (!endpoint.endsWith("/")) {
        endpoint += "/";
      }
      log.debug("_findCluster returning " + endpoint);
      return endpoint;
    }.bind(this);

    
    let promiseClusterURL = function() {
      return this.identity.whenReadyToAuthenticate.promise.then(
        () => {
          
          
          
          
          if (this.service.clusterURL) {
            log.debug("_findCluster found existing clusterURL, so discarding the current token");
            this.identity._token = null;
          }
          return this.identity._ensureValidToken();
        }
      ).then(endPointFromIdentityToken
      );
    }.bind(this);

    let cb = Async.makeSpinningCallback();
    promiseClusterURL().then(function (clusterURL) {
      cb(null, clusterURL);
    }).then(
      null, err => {
      log.info("Failed to fetch the cluster URL", err);
      
      
      
      
      
      
      
      
      
      
      
      
      if (err instanceof AuthenticationError) {
        
        cb(null, null);
      } else {
        
        cb(err);
      }
    });
    return cb.wait();
  },

  getUserBaseURL: function() {
    
    
    
    
    
    return this.service.clusterURL;
  }
}
