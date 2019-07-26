














"use strict";

this.EXPORTED_SYMBOLS = ["IdentityService"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/IdentityUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "logger", function() {
  Cu.import('resource://gre/modules/identity/LogUtils.jsm');
  return getLogger("Identity", "toolkit.identity.debug");
});

function makeMessageObject(aRpCaller) {
  let options = {};

  options.id = aRpCaller.id;
  options.origin = aRpCaller.origin;

  
  options.loggedInUser = aRpCaller.loggedInUser;

  
  options._internal = aRpCaller._internal;

  Object.keys(aRpCaller).forEach(function(option) {
    
    
    if (!Object.hasOwnProperty(this, option)
        && option[0] !== '_'
        && typeof aRpCaller[option] !== 'function') {
      options[option] = aRpCaller[option];
    }
  });

  
  if ((typeof options.id === 'undefined') ||
      (typeof options.origin === 'undefined')) {
    let err = "id and origin required in relying-party message: " + JSON.stringify(options);
    logger.error(err);
    throw new Error(err);
  }

  return options;
}

function IDService() {
  Services.obs.addObserver(this, "quit-application-granted", false);

  
  this.RP = this;
  this.IDP = this;

  
  this._rpFlows = {};
  this._authFlows = {};
  this._provFlows = {};
}

IDService.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

  observe: function IDService_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-granted":
        Services.obs.removeObserver(this, "quit-application-granted");
        break;
    }
  },

  


















  watch: function IDService_watch(aRpCaller) {
    
    this._rpFlows[aRpCaller.id] = aRpCaller;

    let options = makeMessageObject(aRpCaller);
    logger.log("sending identity-controller-watch:", options);
    Services.obs.notifyObservers({wrappedJSObject: options},"identity-controller-watch", null);
  },

  



  unwatch: function IDService_unwatch(aRpId, aTargetMM) {
    let rp = this._rpFlows[aRpId];
    let options = makeMessageObject({
      id: aRpId,
      origin: rp.origin,
      messageManager: aTargetMM
    });
    logger.log("sending identity-controller-unwatch for id", options.id, options.origin);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-unwatch", null);
  },

  









  request: function IDService_request(aRPId, aOptions) {
    let rp = this._rpFlows[aRPId];

    
    
    let options = makeMessageObject(rp);
    objectCopy(aOptions, options);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-request", null);
  },

  







  logout: function IDService_logout(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];

    let options = makeMessageObject(rp);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-logout", null);
  },

  childProcessShutdown: function IDService_childProcessShutdown(messageManager) {
    let options = makeMessageObject({messageManager: messageManager, id: null, origin: null});
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-child-process-shutdown", null);
    Object.keys(this._rpFlows).forEach(function(key) {
      if (this._rpFlows[key]._mm === messageManager) {
        logger.log("child process shutdown for rp", key, "- deleting flow");
        delete this._rpFlows[key];
      }
    }, this);
  },

  





  doLogin: function IDService_doLogin(aRpCallerId, aAssertion, aInternalParams) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      dump("WARNING: doLogin found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doLogin(aAssertion, aInternalParams);
  },

  doLogout: function IDService_doLogout(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      dump("WARNING: doLogout found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doLogout();
  },

  doReady: function IDService_doReady(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      dump("WARNING: doReady found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doReady();
  },

  doCancel: function IDService_doCancel(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      dump("WARNING: doCancel found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doCancel();
  }
};

this.IdentityService = new IDService();
