





"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const EXPORTED_SYMBOLS = ["IdentityStore"];



function IDServiceStore() {
  this.reset();
}



IDServiceStore.prototype = {
  addIdentity: function addIdentity(aEmail, aKeyPair, aCert) {
    this._identities[aEmail] = {keyPair: aKeyPair, cert: aCert};
  },
  fetchIdentity: function fetchIdentity(aEmail) {
    return aEmail in this._identities ? this._identities[aEmail] : null;
  },
  removeIdentity: function removeIdentity(aEmail) {
    let data = this._identities[aEmail];
    delete this._identities[aEmail];
    return data;
  },
  getIdentities: function getIdentities() {
    
    return this._identities;
  },
  clearCert: function clearCert(aEmail) {
    
    this._identities[aEmail].cert = null;
    this._identities[aEmail].keyPair = null;
  },

  












  setLoginState: function setLoginState(aOrigin, aState, aEmail) {
    if (aState && !aEmail) {
      throw "isLoggedIn cannot be set to true without an email";
    }
    return this._loginStates[aOrigin] = {isLoggedIn: aState, email: aEmail};
  },
  getLoginState: function getLoginState(aOrigin) {
    return aOrigin in this._loginStates ? this._loginStates[aOrigin] : null;
  },
  clearLoginState: function clearLoginState(aOrigin) {
    delete this._loginStates[aOrigin];
  },

  reset: function Store_reset() {
    
    this._identities = {};

    
    
    
    this._loginStates = {};
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-granted":
        Services.obs.removeObserver(this, "quit-application-granted");
        this.reset();
        break;
    }
  },
};

let IdentityStore = new IDServiceStore();
