



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserIDManager"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/util.js");







this.BrowserIDManager = function BrowserIDManager(fxaService, tokenServerClient) {
  this._fxaService = fxaService;
  this._tokenServerClient = tokenServerClient;
  this._log = Log.repository.getLogger("Sync.Identity");
  this._log.Level = Log.Level[Svc.Prefs.get("log.logger.identity")];

};

this.BrowserIDManager.prototype = {
  __proto__: IdentityManager.prototype,

  _fxaService: null,
  _tokenServerClient: null,
  
  _token: null,

  _clearUserState: function() {
    this.account = null;
    this._token = null;
  },

  


  _normalizeAccountValue: function(value) {
    return value.toLowerCase();
  },

  


  _now: function() {
    return Date.now();
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
    
    if (this._normalizeAccountValue(signedInUser.email) !== this.account) {
      return false;
    }
    return true;
  },

  




  _getSignedInUser: function() {
    let userBlob;
    let cb = Async.makeSpinningCallback();

    this._fxaService.getSignedInUser().then(function (result) {
        cb(null, result);
    },
    function (err) {
        cb(err);
    });

    try {
      userBlob = cb.wait();
    } catch (err) {
      this._log.info("FxAccounts.getSignedInUser() failed with: " + err);
      return null;
    }
    return userBlob;
  },

 _fetchTokenForUser: function(user) {
    let token;
    let cb = Async.makeSpinningCallback();
    let tokenServerURI = Svc.Prefs.get("services.sync.tokenServerURI");

    try {
      this._tokenServerClient.getTokenFromBrowserIDAssertion(
        tokenServerURI, user.assertion, cb);
      token = cb.wait();
    } catch (err) {
      this._log.info("TokenServerClient.getTokenFromBrowserIDAssertion() failed with: " + err.api_endpoint);
      return null;
    }

    token.expiration = this._now() + (token.duration * 1000);
    return token;
  },

  getResourceAuthenticator: function() {
    return this._getAuthenticationHeader.bind(this);
  },

  



  _getAuthenticationHeader: function(httpObject, method) {
    if (!this.hasValidToken()) {
      this._clearUserState();
      let user = this._getSignedInUser();
      if (!user) {
        return null;
      }
      this._token = this._fetchTokenForUser(user);
      if (!this._token) {
        return null;
      }
      this.account = this._normalizeAccountValue(user.email);
    }
    let credentials = {algorithm: "sha256",
                       id: this.username,
                       key: this._token,
                      };
    method = method || httpObject.method;
    let headerValue = CryptoUtils.computeHAWK(httpObject.uri, method,
                                              {credentials: credentials});
    return {headers: {authorization: headerValue.field}};
  },

  getRequestAuthenticator: function() {
    return this._addAuthenticationHeader.bind(this);
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
