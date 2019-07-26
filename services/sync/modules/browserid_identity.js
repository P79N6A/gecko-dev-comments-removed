



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserIDManager"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://services-sync/stages/cluster.js");


XPCOMUtils.defineLazyModuleGetter(this, "BulkKeyBundle",
                                  "resource://services-sync/keys.js");

XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts",
                                  "resource://gre/modules/FxAccounts.jsm");

XPCOMUtils.defineLazyGetter(this, 'fxAccountsCommon', function() {
  let ob = {};
  Cu.import("resource://gre/modules/FxAccountsCommon.js", ob);
  return ob;
});

function deriveKeyBundle(kB) {
  let out = CryptoUtils.hkdf(kB, undefined,
                             "identity.mozilla.com/picl/v1/oldsync", 2*32);
  let bundle = new BulkKeyBundle();
  
  bundle.keyPair = [out.slice(0, 32), out.slice(32, 64)];
  return bundle;
}


this.BrowserIDManager = function BrowserIDManager() {
  this._fxaService = fxAccounts;
  this._tokenServerClient = new TokenServerClient();
  
  this.whenReadyToAuthenticate = null;
  this._log = Log.repository.getLogger("Sync.BrowserIDManager");
  this._log.addAppender(new Log.DumpAppender());
  this._log.Level = Log.Level[Svc.Prefs.get("log.logger.identity")] || Log.Level.Error;
};

this.BrowserIDManager.prototype = {
  __proto__: IdentityManager.prototype,

  _fxaService: null,
  _tokenServerClient: null,
  
  _token: null,
  _account: null,

  
  
  _shouldHaveSyncKeyBundle: false,

  get readyToAuthenticate() {
    
    
    return this._shouldHaveSyncKeyBundle;
  },

  get needsCustomization() {
    try {
      return Services.prefs.getBoolPref("services.sync.needsCustomization");
    } catch (e) {
      return false;
    }
  },

  initialize: function() {
    Services.obs.addObserver(this, fxAccountsCommon.ONVERIFIED_NOTIFICATION, false);
    Services.obs.addObserver(this, fxAccountsCommon.ONLOGOUT_NOTIFICATION, false);
    return this.initializeWithCurrentIdentity();
  },

  initializeWithCurrentIdentity: function() {
    this._log.trace("initializeWithCurrentIdentity");
    Components.utils.import("resource://services-sync/main.js");

    
    this.whenReadyToAuthenticate = Promise.defer();
    this._shouldHaveSyncKeyBundle = false;
    this.username = ""; 

    return fxAccounts.getSignedInUser().then(accountData => {
      if (!accountData) {
        this._log.info("initializeWithCurrentIdentity has no user logged in");
        this._account = null;
        return;
      }

      if (this.needsCustomization) {
        
        
        const url = "chrome://browser/content/sync/customize.xul";
        const features = "centerscreen,chrome,modal,dialog,resizable=no";
        let win = Services.wm.getMostRecentWindow("navigator:browser");

        let data = {accepted: false};
        win.openDialog(url, "_blank", features, data);

        if (data.accepted) {
          Services.prefs.clearUserPref("services.sync.needsCustomization");
        } else {
          
          return fxAccounts.signOut();
        }
      }

      this._account = accountData.email;
      
      this._log.info("Starting background fetch for key bundle.");
      this._fetchSyncKeyBundle().then(() => {
        this._shouldHaveSyncKeyBundle = true; 
        this.whenReadyToAuthenticate.resolve();
        this._log.info("Background fetch for key bundle done");
      }).then(null, err => {
        this._shouldHaveSyncKeyBundle = true; 
        this.whenReadyToAuthenticate.reject(err);
        
        this._log.error("Background fetch for key bundle failed: " + err);
        throw err;
      });
      
    }).then(null, err => {
      dump("err in processing logged in account "+err.message);
    });
  },

  observe: function (subject, topic, data) {
    switch (topic) {
    case fxAccountsCommon.ONVERIFIED_NOTIFICATION:
    case fxAccountsCommon.ONLOGIN_NOTIFICATION:
      
      
      
      
      
      this.initializeWithCurrentIdentity();
      break;

    case fxAccountsCommon.ONLOGOUT_NOTIFICATION:
      Components.utils.import("resource://services-sync/main.js");
      
      
      this.username = "";
      this._account = null;
      Weave.Service.logout();
      break;
    }
  },

  


  _now: function() {
    return Date.now();
  },

  get account() {
    return this._account;
  },

  











  set account(value) {
    throw "account setter should be not used in BrowserIDManager";
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
  },

  


  resetSyncKey: function() {
    this._syncKey = null;
    this._syncKeyBundle = null;
    this._syncKeyUpdated = true;
    this._shouldHaveSyncKeyBundle = false;
  },

  





  get currentAuthState() {
    
    
    
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
    let signedInUser = this._getSignedInUser();
    if (!signedInUser) {
      return false;
    }
    
    if (signedInUser.email !== this.account) {
      return false;
    }
    return true;
  },

  




  _getSignedInUser: function() {
    let userData;
    let cb = Async.makeSpinningCallback();

    this._fxaService.getSignedInUser().then(function (result) {
        cb(null, result);
    },
    function (err) {
        cb(err);
    });

    try {
      userData = cb.wait();
    } catch (err) {
      this._log.error("FxAccounts.getSignedInUser() failed with: " + err);
      return null;
    }
    return userData;
  },

  _fetchSyncKeyBundle: function() {
    
    return this._refreshTokenForLoggedInUser(
    ).then(token => {
      this._token = token;
      return this._fxaService.getKeys();
    }).then(userData => {
      
      
      if (!userData || userData.email !== this.account) {
        throw new Error("The currently logged-in user has changed.");
      }
      
      this.username = this._token.uid.toString();
      
      let kB = Utils.hexToBytes(userData.kB);
      this._syncKeyBundle = deriveKeyBundle(kB);
    });
  },

  
  
  _refreshTokenForLoggedInUser: function() {
    return this._fxaService.getSignedInUser().then(function (userData) {
      if (!userData || userData.email !== this.account) {
        
        
        
        this._log.error("Currently logged in FxA user differs from what was locally noted. TODO: do proper error handling.");
        return null;
      }
      return this._fetchTokenForUser(userData);
    }.bind(this));
  },

  _refreshTokenForLoggedInUserSync: function() {
    let cb = Async.makeSpinningCallback();

    this._refreshTokenForLoggedInUser().then(function (token) {
      cb(null, token);
    },
    function (err) {
      cb(err);
    });

    try {
      return cb.wait();
    } catch (err) {
      this._log.info("refreshTokenForLoggedInUserSync: " + err.message);
      return null;
    }
  },

  
  _fetchTokenForUser: function(userData) {
    let tokenServerURI = Svc.Prefs.get("tokenServerURI");
    let log = this._log;
    let client = this._tokenServerClient;
    log.info("Fetching Sync token from: " + tokenServerURI);

    function getToken(tokenServerURI, assertion) {
      let deferred = Promise.defer();
      let cb = function (err, token) {
        if (err) {
          log.info("TokenServerClient.getTokenFromBrowserIDAssertion() failed with: " + err.message);
          return deferred.reject(err);
        } else {
          return deferred.resolve(token);
        }
      };
      client.getTokenFromBrowserIDAssertion(tokenServerURI, assertion, cb);
      return deferred.promise;
    }

    let audience = Services.io.newURI(tokenServerURI, null, null).prePath;
    
    
    return this._fxaService.whenVerified(userData)
      .then(() => this._fxaService.getAssertion(audience))
      .then(assertion => getToken(tokenServerURI, assertion))
      .then(token => {
        token.expiration = this._now() + (token.duration * 1000);
        return token;
      });
  },

  getResourceAuthenticator: function () {
    return this._getAuthenticationHeader.bind(this);
  },

  


  getRESTRequestAuthenticator: function() {
    return this._addAuthenticationHeader.bind(this);
  },

  



  _getAuthenticationHeader: function(httpObject, method) {
    if (!this.hasValidToken()) {
      
      this._token = this._refreshTokenForLoggedInUserSync();
      if (!this._token) {
        return null;
      }
    }
    let credentials = {algorithm: "sha256",
                       id: this._token.id,
                       key: this._token.key,
                      };
    method = method || httpObject.method;
    let headerValue = CryptoUtils.computeHAWK(httpObject.uri, method,
                                              {credentials: credentials});
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
    let promiseClusterURL = function() {
      return fxAccounts.getSignedInUser().then(userData => {
        return this.identity._fetchTokenForUser(userData).then(token => {
          
          
          
          let clusterURI = Services.io.newURI(token.endpoint, null, null);
          clusterURI.path = "/";
          return clusterURI.spec;
        });
      });
    }.bind(this);

    let cb = Async.makeSpinningCallback();
    promiseClusterURL().then(function (clusterURL) {
        cb(null, clusterURL);
    },
    function (err) {
        cb(err);
    });
    return cb.wait();
  },
}
