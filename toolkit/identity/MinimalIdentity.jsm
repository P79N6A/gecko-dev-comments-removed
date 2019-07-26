














"use strict";

this.EXPORTED_SYMBOLS = ["IdentityService"];

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/LogUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "objectCopy",
                                  "resource://gre/modules/identity/IdentityUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "makeMessageObject",
                                  "resource://gre/modules/identity/IdentityUtils.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["minimal core"].concat(aMessageArgs));
}
function reportError(...aMessageArgs) {
  Logger.reportError.apply(Logger, ["core"].concat(aMessageArgs));
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

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-granted":
        this.shutdown();
        break;
    }
  },

  shutdown: function() {
    Services.obs.removeObserver(this, "quit-application-granted");
  },

  


  parseEmail: function parseEmail(email) {
    var match = email.match(/^([^@]+)@([^@^/]+.[a-z]+)$/);
    if (match) {
      return {
        username: match[1],
        domain: match[2]
      };
    }
    return null;
  },

  


















  watch: function watch(aRpCaller) {
    
    this._rpFlows[aRpCaller.id] = aRpCaller;

    log("flows:", Object.keys(this._rpFlows).join(', '));

    let options = makeMessageObject(aRpCaller);
    log("sending identity-controller-watch:", options);
    Services.obs.notifyObservers({wrappedJSObject: options},"identity-controller-watch", null);
  },

  



  unwatch: function unwatch(aRpId, aTargetMM) {
    let rp = this._rpFlows[aRpId];
    if (!rp) {
      return;
    }

    let options = makeMessageObject({
      id: aRpId,
      origin: rp.origin,
      messageManager: aTargetMM
    });
    log("sending identity-controller-unwatch for id", options.id, options.origin);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-unwatch", null);

    
    delete this._rpFlows[aRpId];
  },

  









  request: function request(aRPId, aOptions) {
    let rp = this._rpFlows[aRPId];
    if (!rp) {
      reportError("request() called before watch()");
      return;
    }

    
    
    let options = makeMessageObject(rp);
    objectCopy(aOptions, options);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-request", null);
  },

  







  logout: function logout(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      reportError("logout() called before watch()");
      return;
    }

    let options = makeMessageObject(rp);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-logout", null);
  },

  childProcessShutdown: function childProcessShutdown(messageManager) {
    Object.keys(this._rpFlows).forEach(function(key) {
      if (this._rpFlows[key]._mm === messageManager) {
        log("child process shutdown for rp", key, "- deleting flow");
        delete this._rpFlows[key];
      }
    }, this);
  },

  





  doLogin: function doLogin(aRpCallerId, aAssertion, aInternalParams) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      log("WARNING: doLogin found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doLogin(aAssertion, aInternalParams);
  },

  doLogout: function doLogout(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      log("WARNING: doLogout found no rp to go with callerId " + aRpCallerId);
      return;
    }

    
    let origin = rp.origin;
    Object.keys(this._rpFlows).forEach(function(key) {
      let rp = this._rpFlows[key];
      if (rp.origin === origin) {
        rp.doLogout();
      }
    }.bind(this));
  },

  doReady: function doReady(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      log("WARNING: doReady found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doReady();
  },

  doCancel: function doCancel(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      log("WARNING: doCancel found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doCancel();
  }
};

this.IdentityService = new IDService();
