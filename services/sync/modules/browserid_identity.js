



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserIDManager"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/Promise.jsm");


for (let symbol of ["BulkKeyBundle"]) {
  XPCOMUtils.defineLazyModuleGetter(this, symbol,
                                    "resource://services-sync/keys.js",
                                    symbol);
}

function deriveKeyBundle(kB) {
  let out = CryptoUtils.hkdf(kB, undefined,
                             "identity.mozilla.com/picl/v1/oldsync", 2*32);
  let bundle = new BulkKeyBundle();
  
  bundle.keyPair = [out.slice(0, 32), out.slice(32, 64)];
  return bundle;
}








this.BrowserIDManager = function BrowserIDManager(fxaService, tokenServerClient) {
  this._fxaService = fxaService;
  this._tokenServerClient = tokenServerClient;
  this._log = Log.repository.getLogger("Sync.BrowserIDManager");
  this._log.Level = Log.Level[Svc.Prefs.get("log.logger.identity")];

};

this.BrowserIDManager.prototype = {
  __proto__: IdentityManager.prototype,

  _fxaService: null,
  _tokenServerClient: null,
  
  _token: null,
  _account: null,

  


  _now: function() {
    return Date.now();
  },

  clusterURL: null,

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
  },

  





  get currentAuthState() {
    
    
    
    if (!this.username) {
      return LOGIN_FAILED_NO_USERNAME;
    }

    
    
    
    if (!this.syncKeyBundle) {
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

  
  
  
  initWithLoggedInUser: function() {
    
    return this._fxaService.getSignedInUser()
      .then(userData => {
        if (!userData) {
          this._log.warn("initWithLoggedInUser found no logged in user");
          throw new Error("initWithLoggedInUser found no logged in user");
        }
        
        this._account = userData.email;
        
        return this._refreshTokenForLoggedInUser();
      })
      .then(token => {
        this._token = token;
        
        
        
        
        this.username = this._token.uid.toString();

        return this._fxaService.getKeys();
      })
      .then(userData => {
        
        let kB = Utils.hexToBytes(userData.kB);
        this._syncKeyBundle = deriveKeyBundle(kB);

        
        
        
        let clusterURI = Services.io.newURI(this._token.endpoint, null, null);
        clusterURI.path = "/";
        this.clusterURL = clusterURI.spec;
        this._log.info("initWithLoggedUser has username " + this.username + ", endpoint is " + this.clusterURL);
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
  }
};
