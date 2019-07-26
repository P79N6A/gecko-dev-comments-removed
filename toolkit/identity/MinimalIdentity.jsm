














"use strict";

this.EXPORTED_SYMBOLS = ["IdentityService"];

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/LogUtils.jsm");
Cu.import("resource://gre/modules/identity/IdentityUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
                                  "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["minimal core"].concat(aMessageArgs));
}
function reportError(...aMessageArgs) {
  Logger.reportError.apply(Logger, ["core"].concat(aMessageArgs));
}

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
    reportError(err);
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

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-granted":
        Services.obs.removeObserver(this, "quit-application-granted");
        
        break;
    }
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

    let options = makeMessageObject(aRpCaller);
    log("sending identity-controller-watch:", options);
    Services.obs.notifyObservers({wrappedJSObject: options},"identity-controller-watch", null);
  },

  



  unwatch: function unwatch(aRpId, aTargetMM) {
    let rp = this._rpFlows[aRpId];
    let options = makeMessageObject({
      id: aRpId,
      origin: rp.origin,
      messageManager: aTargetMM
    });
    log("sending identity-controller-unwatch for id", options.id, options.origin);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-unwatch", null);
  },

  









  request: function request(aRPId, aOptions) {
    let rp = this._rpFlows[aRPId];

    
    
    let options = makeMessageObject(rp);
    objectCopy(aOptions, options);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-request", null);
  },

  







  logout: function logout(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];

    let options = makeMessageObject(rp);
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-logout", null);
  },

  childProcessShutdown: function childProcessShutdown(messageManager) {
    let options = makeMessageObject({messageManager: messageManager, id: null, origin: null});
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-child-process-shutdown", null);
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
      dump("WARNING: doLogin found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doLogin(aAssertion, aInternalParams);
  },

  doLogout: function doLogout(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      dump("WARNING: doLogout found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doLogout();
  },

  doReady: function doReady(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      dump("WARNING: doReady found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doReady();
  },

  doCancel: function doCancel(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp) {
      dump("WARNING: doCancel found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doCancel();
  },


  





  








  beginProvisioning: function beginProvisioning(aCaller) {
  },

  







  raiseProvisioningFailure: function raiseProvisioningFailure(aProvId, aReason) {
    reportError("Provisioning failure", aReason);
  },

  









  genKeyPair: function genKeyPair(aProvId) {
  },

  














  registerCertificate: function registerCertificate(aProvId, aCert) {
  },

  











  beginAuthentication: function beginAuthentication(aCaller) {
  },

  






  completeAuthentication: function completeAuthentication(aAuthId) {
  },

  






  cancelAuthentication: function cancelAuthentication(aAuthId) {
  },

  

  









  _discoverIdentityProvider: function _discoverIdentityProvider(aIdentity, aCallback) {
    
    
    var parsedEmail = this.parseEmail(aIdentity);
    if (parsedEmail === null) {
      return aCallback("Could not parse email: " + aIdentity);
    }
    log("_discoverIdentityProvider: identity:", aIdentity, "domain:", parsedEmail.domain);

    this._fetchWellKnownFile(parsedEmail.domain, function fetchedWellKnown(err, idpParams) {
      
      

      
      
      
      return aCallback(err, idpParams);
    });
  },

  












  _fetchWellKnownFile: function _fetchWellKnownFile(aDomain, aCallback, aScheme='https') {
    
    let url = aScheme + '://' + aDomain + "/.well-known/browserid";
    log("_fetchWellKnownFile:", url);

    
    let req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);

    
    
    req.open("GET", url, true);
    req.responseType = "json";
    req.mozBackgroundRequest = true;
    req.onload = function _fetchWellKnownFile_onload() {
      if (req.status < 200 || req.status >= 400) {
        log("_fetchWellKnownFile", url, ": server returned status:", req.status);
        return aCallback("Error");
      }
      try {
        let idpParams = req.response;

        
        if (! (idpParams.provisioning &&
            idpParams.authentication &&
            idpParams['public-key'])) {
          let errStr= "Invalid well-known file from: " + aDomain;
          log("_fetchWellKnownFile:", errStr);
          return aCallback(errStr);
        }

        let callbackObj = {
          domain: aDomain,
          idpParams: idpParams,
        };
        log("_fetchWellKnownFile result: ", callbackObj);
        
        return aCallback(null, callbackObj);

      } catch (err) {
        reportError("_fetchWellKnownFile", "Bad configuration from", aDomain, err);
        return aCallback(err.toString());
      }
    };
    req.onerror = function _fetchWellKnownFile_onerror() {
      log("_fetchWellKnownFile", "ERROR:", req.status, req.statusText);
      log("ERROR: _fetchWellKnownFile:", err);
      return aCallback("Error");
    };
    req.send(null);
  },

};

this.IdentityService = new IDService();
