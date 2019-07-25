





"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/LogUtils.jsm");
Cu.import("resource://gre/modules/identity/IdentityStore.jsm");

const EXPORTED_SYMBOLS = ["RelyingParty"];

XPCOMUtils.defineLazyModuleGetter(this,
                                  "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["RP"].concat(aMessageArgs));
}
function reportError(...aMessageArgs) {
  Logger.reportError.apply(Logger, ["RP"].concat(aMessageArgs));
}

function IdentityRelyingParty() {
  
  
  
  this._store = IdentityStore;

  this.reset();
}

IdentityRelyingParty.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-granted":
        Services.obs.removeObserver(this, "quit-application-granted");
        this.shutdown();
        break;

    }
  },

  reset: function RP_reset() {
    
    
    this._rpFlows = {};
  },

  shutdown: function RP_shutdown() {
    this.reset();
    Services.obs.removeObserver(this, "quit-application-granted");
  },

  


















  watch: function watch(aRpCaller) {
    this._rpFlows[aRpCaller.id] = aRpCaller;
    let origin = aRpCaller.origin;
    let state = this._store.getLoginState(origin) || { isLoggedIn: false, email: null };

    log("watch: rpId:", aRpCaller.id,
        "origin:", origin,
        "loggedInEmail:", aRpCaller.loggedInEmail,
        "loggedIn:", state.isLoggedIn,
        "email:", state.email);

    
    
    
    
    
    
    if (state.isLoggedIn) {
      if (state.email && aRpCaller.loggedInEmail === state.email) {
        this._notifyLoginStateChanged(aRpCaller.id, state.email);
        return aRpCaller.doReady();

      } else if (aRpCaller.loggedInEmail === null) {
        
        let options = {loggedInEmail: state.email, origin: origin};
        return this._doLogin(aRpCaller, options);

      } else {
        
        

        let options = {loggedInEmail: state.email, origin: origin};
        return this._doLogin(aRpCaller, options);
      }

    
    
    
    

    } else {
      if (aRpCaller.loggedInEmail) {
        return this._doLogout(aRpCaller, {origin: origin});

      } else {
        return aRpCaller.doReady();
      }
    }
  },

  





  _doLogin: function _doLogin(aRpCaller, aOptions, aAssertion) {
    log("_doLogin: rpId:", aRpCaller.id, "origin:", aOptions.origin);

    let loginWithAssertion = function loginWithAssertion(assertion) {
      this._store.setLoginState(aOptions.origin, true, aOptions.loggedInEmail);
      this._notifyLoginStateChanged(aRpCaller.id, aOptions.loggedInEmail);
      aRpCaller.doLogin(assertion);
      aRpCaller.doReady();
    }.bind(this);

    if (aAssertion) {
      loginWithAssertion(aAssertion);
    } else {
      this._getAssertion(aOptions, function gotAssertion(err, assertion) {
        if (err) {
          reportError("_doLogin:", "Failed to get assertion on login attempt:", err);
          this._doLogout(aRpCaller);
        } else {
          loginWithAssertion(assertion);
        }
      }.bind(this));
    }
  },

  



  _doLogout: function _doLogout(aRpCaller, aOptions) {
    log("_doLogout: rpId:", aRpCaller.id, "origin:", aOptions.origin);

    let state = this._store.getLoginState(aOptions.origin) || {};

    state.isLoggedIn = false;
    this._notifyLoginStateChanged(aRpCaller.id, null);

    aRpCaller.doLogout();
    aRpCaller.doReady();
  },

  











  _notifyLoginStateChanged: function _notifyLoginStateChanged(aRpCallerId, aIdentity) {
    log("_notifyLoginStateChanged: rpId:", aRpCallerId, "identity:", aIdentity);

    let options = {rpId: aRpCallerId};
    Services.obs.notifyObservers({wrappedJSObject: options},
                                 "identity-login-state-changed",
                                 aIdentity);
  },

  









  request: function request(aRPId, aOptions) {
    log("request: rpId:", aRPId);
    let rp = this._rpFlows[aRPId];

    
    
    let options = {rpId: aRPId, origin: rp.origin};

    
    let baseURI = Services.io.newURI(rp.origin, null, null);
    for (let optionName of ["privacyPolicy", "termsOfService"]) {
      if (aOptions[optionName]) {
        options[optionName] = baseURI.resolve(aOptions[optionName]);
      }
    }

    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-request", null);
  },

  







  logout: function logout(aRpCallerId) {
    log("logout: RP caller id:", aRpCallerId);
    let rp = this._rpFlows[aRpCallerId];
    if (rp && rp.origin) {
      let origin = rp.origin;
      log("logout: origin:", origin);
      this._doLogout(rp, {origin: origin});
    } else {
      log("logout: no RP found with id:", aRpCallerId);
    }
    
    
  },

  getDefaultEmailForOrigin: function getDefaultEmailForOrigin(aOrigin) {
    let identities = this.getIdentitiesForSite(aOrigin);
    let result = identities.lastUsed || null;
    log("getDefaultEmailForOrigin:", aOrigin, "->", result);
    return result;
  },

  


  getIdentitiesForSite: function getIdentitiesForSite(aOrigin) {
    let rv = { result: [] };
    for (let id in this._store.getIdentities()) {
      rv.result.push(id);
    }
    let loginState = this._store.getLoginState(aOrigin);
    if (loginState && loginState.email)
      rv.lastUsed = loginState.email;
    return rv;
  },

  

















  _getAssertion: function _getAssertion(aOptions, aCallback) {
    let audience = aOptions.origin;
    let email = aOptions.loggedInEmail || this.getDefaultEmailForOrigin(audience);
    log("_getAssertion: audience:", audience, "email:", email);
    if (!audience) {
      throw "audience required for _getAssertion";
    }

    
    if (!this._store.fetchIdentity(email)) {
      this._store.addIdentity(email, null, null);
    }

    let cert = this._store.fetchIdentity(email)['cert'];
    if (cert) {
      this._generateAssertion(audience, email, function generatedAssertion(err, assertion) {
        if (err) {
          log("ERROR: _getAssertion:", err);
        }
        log("_getAssertion: generated assertion:", assertion);
        return aCallback(err, assertion);
      });
    }
  },

  














  _generateAssertion: function _generateAssertion(aAudience, aIdentity, aCallback) {
    log("_generateAssertion: audience:", aAudience, "identity:", aIdentity);

    let id = this._store.fetchIdentity(aIdentity);
    if (! (id && id.cert)) {
      let errStr = "Cannot generate an assertion without a certificate";
      log("ERROR: _generateAssertion:", errStr);
      aCallback(errStr);
      return;
    }

    let kp = id.keyPair;

    if (!kp) {
      let errStr = "Cannot generate an assertion without a keypair";
      log("ERROR: _generateAssertion:", errStr);
      aCallback(errStr);
      return;
    }

    jwcrypto.generateAssertion(id.cert, kp, aAudience, aCallback);
  },

  


  _cleanUpProvisionFlow: function RP_cleanUpProvisionFlow(aRPId, aProvId) {
    let rp = this._rpFlows[aRPId];
    if (rp) {
      delete rp['provId'];
    } else {
      log("Error: Couldn't delete provision flow ", aProvId, " for RP ", aRPId);
    }
  },

};

let RelyingParty = new IdentityRelyingParty();
