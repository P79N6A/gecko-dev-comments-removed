











"use strict";

const EXPORTED_SYMBOLS = ["IdentityService"];

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/LogUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
                                  "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["minimal core"].concat(aMessageArgs));
}
function reportError(...aMessageArgs) {
  Logger.reportError.apply(Logger, ["core"].concat(aMessageArgs));
}

function IDService() {
  Services.obs.addObserver(this, "quit-application-granted", false);
  Services.obs.addObserver(this, "identity-auth-complete", false);

  
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
        this.shutdown();
        break;
    }
  },

  shutdown: function shutdown() {
    log("shutdown");
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
    log("watch: ", aRpCaller);

    
    this._rpFlows[aRpCaller.id] = aRpCaller;
    let options = {rpId: aRpCaller.id, origin: aRpCaller.origin};
    Services.obs.notifyObservers({wrappedJSObject: options},"identity-controller-watch", null);
  },

  









  request: function request(aRPId, aOptions) {
    log("request: rpId:", aRPId);
    let rp = this._rpFlows[aRPId];

    
    
    let options = {rpId: aRPId, origin: rp.origin};

    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-controller-request", null);
  },

  







  logout: function logout(aRpCallerId) {
    log("logout: RP caller id:", aRpCallerId);
    Services.obs.notifyObservers({wrappedJSObject: {rpId: aRpCallerId}},
                                 "identity-controller-logout",
                                 null);

  },

  
  

  


  doLogin: function doLogin(aRpCallerId, aAssertion) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp)
      return;

    rp.doLogin(aAssertion);
  },

  doLogout: function doLogout(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp)
      return;

    rp.doLogout();
  },

  doReady: function doReady(aRpCallerId) {
    let rp = this._rpFlows[aRpCallerId];
    if (!rp)
      return;

    rp.doReady();
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

let IdentityService = new IDService();
