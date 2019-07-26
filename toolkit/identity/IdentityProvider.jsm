





"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/Sandbox.jsm");

this.EXPORTED_SYMBOLS = ["IdentityProvider"];
const FALLBACK_PROVIDER = "browserid.org";

XPCOMUtils.defineLazyModuleGetter(this,
                                  "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

XPCOMUtils.defineLazyGetter(this, "logger", function() {
  Cu.import('resource://gre/modules/identity/LogUtils.jsm');
  return getLogger("Identity IDP", "toolkit.identity.debug");
});

function IdentityProviderService() {
  XPCOMUtils.defineLazyModuleGetter(this,
                                    "_store",
                                    "resource://gre/modules/identity/IdentityStore.jsm",
                                    "IdentityStore");

  this.reset();
}

IdentityProviderService.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),
  _sandboxConfigured: false,

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-granted":
        Services.obs.removeObserver(this, "quit-application-granted");
        this.shutdown();
        break;
    }
  },

  reset: function IDP_reset() {
    
    
    
    
    
    
    this._provisionFlows = {};

    
    
    
    
    this._authenticationFlows = {};
  },

  getProvisionFlow: function getProvisionFlow(aProvId, aErrBack) {
    let provFlow = this._provisionFlows[aProvId];
    if (provFlow) {
      return provFlow;
    }

    let err = "No provisioning flow found with id " + aProvId;
    logger.warning("ERROR:", err);
    if (typeof aErrBack === 'function') {
      aErrBack(err);
    }
  },

  shutdown: function RP_shutdown() {
    this.reset();

    if (this._sandboxConfigured) {
      
      Cu.import("resource://gre/modules/DOMIdentity.jsm");
      DOMIdentity._configureMessages(Services.appShell.hiddenDOMWindow, false);
      this._sandboxConfigured = false;
    }

    Services.obs.removeObserver(this, "quit-application-granted");
  },

  get securityLevel() {
    return 1;
  },

  get certDuration() {
    switch(this.securityLevel) {
      default:
        return 3600;
    }
  },

  












  _provisionIdentity: function _provisionIdentity(aIdentity, aIDPParams, aProvId, aCallback) {
    let provPath = aIDPParams.idpParams.provisioning;
    let url = Services.io.newURI("https://" + aIDPParams.domain, null, null).resolve(provPath);
    logger.log("_provisionIdentity: identity:", aIdentity, "url:", url);

    
    
    

    if (aProvId) {
      
      logger.log("_provisionIdentity: re-using sandbox in provisioning flow with id:", aProvId);
      this._provisionFlows[aProvId].provisioningSandbox.reload();

    } else {
      this._createProvisioningSandbox(url, function createdSandbox(aSandbox) {
        
        

        let provId = aSandbox.id;
        this._provisionFlows[provId] = {
          identity: aIdentity,
          idpParams: aIDPParams,
          securityLevel: this.securityLevel,
          provisioningSandbox: aSandbox,
          callback: function doCallback(aErr) {
            aCallback(aErr, provId);
          },
        };

        logger.log("_provisionIdentity: Created sandbox and provisioning flow with id:", provId);
        

      }.bind(this));
    }
  },

  
  








  beginProvisioning: function beginProvisioning(aCaller) {
    logger.log("beginProvisioning:", aCaller.id);

    
    let provFlow = this.getProvisionFlow(aCaller.id, aCaller.doError);

    
    provFlow.caller = aCaller;

    let identity = provFlow.identity;
    let frame = provFlow.provisioningFrame;

    
    let duration = this.certDuration;

    
    
    provFlow.didBeginProvisioning = true;

    
    
    return aCaller.doBeginProvisioningCallback(identity, duration);
  },

  







  raiseProvisioningFailure: function raiseProvisioningFailure(aProvId, aReason) {
    logger.warning("Provisioning failure", aReason);

    
    let provFlow = this.getProvisionFlow(aProvId);

    

    
    
    
    
    

    
    provFlow.callback(aReason);
  },

  









  genKeyPair: function genKeyPair(aProvId) {
    
    let provFlow = this.getProvisionFlow(aProvId);

    if (!provFlow.didBeginProvisioning) {
      let errStr = "ERROR: genKeyPair called before beginProvisioning";
      logger.warning(errStr);
      provFlow.callback(errStr);
      return;
    }

    
    jwcrypto.generateKeyPair(jwcrypto.ALGORITHMS.DS160, function gkpCb(err, kp) {
      logger.log("in gkp callback");
      if (err) {
        logger.error("ERROR: genKeyPair:", err);
        provFlow.callback(err);
        return;
      }

      provFlow.kp = kp;

      
      
      logger.log("genKeyPair: generated keypair for provisioning flow with id:", aProvId);
      provFlow.caller.doGenKeyPairCallback(provFlow.kp.serializedPublicKey);
    }.bind(this));
  },

  














  registerCertificate: function registerCertificate(aProvId, aCert) {
    logger.log("registerCertificate:", aProvId, aCert);

    
    let provFlow = this.getProvisionFlow(aProvId);

    if (!provFlow.caller) {
      logger.warning("registerCertificate", "No provision flow or caller");
      return;
    }
    if (!provFlow.kp)  {
      let errStr = "Cannot register a certificate without a keypair";
      logger.warning("registerCertificate", errStr);
      provFlow.callback(errStr);
      return;
    }

    
    this._store.addIdentity(provFlow.identity, provFlow.kp, aCert);

    
    provFlow.callback(null);

    
    this._cleanUpProvisionFlow(aProvId);
  },

  









  _doAuthentication: function _doAuthentication(aProvId, aIDPParams) {
    logger.log("_doAuthentication: provId:", aProvId, "idpParams:", aIDPParams);
    
    

    
    let authPath = aIDPParams.idpParams.authentication;
    let authURI = Services.io.newURI("https://" + aIDPParams.domain, null, null).resolve(authPath);

    
    
    
    
    this._beginAuthenticationFlow(aProvId, authURI);

    
    
    
    
  },

  











  beginAuthentication: function beginAuthentication(aCaller) {
    logger.log("beginAuthentication: caller id:", aCaller.id);

    
    
    
    let authFlow = this._authenticationFlows[aCaller.id];
    if (!authFlow) {
      return aCaller.doError("beginAuthentication: no flow for caller id", aCaller.id);
    }

    authFlow.caller = aCaller;

    let identity = this._provisionFlows[authFlow.provId].identity;

    
    logger.log("beginAuthentication: authFlow:", aCaller.id, "identity:", identity);
    return authFlow.caller.doBeginAuthenticationCallback(identity);
  },

  






  completeAuthentication: function completeAuthentication(aAuthId) {
    logger.log("completeAuthentication:", aAuthId);

    
    let authFlow = this._authenticationFlows[aAuthId];
    if (!authFlow) {
      logger.warning("completeAuthentication", "No auth flow with id", aAuthId);
      return;
    }
    let provId = authFlow.provId;

    
    delete authFlow['caller'];
    delete this._authenticationFlows[aAuthId];

    let provFlow = this.getProvisionFlow(provId);
    provFlow.didAuthentication = true;
    let subject = {
      rpId: provFlow.rpId,
      identity: provFlow.identity,
    };
    Services.obs.notifyObservers({ wrappedJSObject: subject }, "identity-auth-complete", aAuthId);
  },

  






  cancelAuthentication: function cancelAuthentication(aAuthId) {
    logger.log("cancelAuthentication:", aAuthId);

    
    let authFlow = this._authenticationFlows[aAuthId];
    if (!authFlow) {
      logger.warning("cancelAuthentication", "No auth flow with id:", aAuthId);
      return;
    }
    let provId = authFlow.provId;

    
    delete authFlow['caller'];
    delete this._authenticationFlows[aAuthId];

    let provFlow = this.getProvisionFlow(provId);
    provFlow.didAuthentication = true;
    Services.obs.notifyObservers(null, "identity-auth-complete", aAuthId);

    
    let errStr = "Authentication canceled by IDP";
    logger.log("ERROR: cancelAuthentication:", errStr);
    provFlow.callback(errStr);
  },

  


  setAuthenticationFlow: function(aAuthId, aProvId) {
    
    
    
    logger.log("setAuthenticationFlow: authId:", aAuthId, "provId:", aProvId);
    this._authenticationFlows[aAuthId] = { provId: aProvId };
    this._provisionFlows[aProvId].authId = aAuthId;
  },

  



  _createProvisioningSandbox: function _createProvisioningSandbox(aURL, aCallback) {
    logger.log("_createProvisioningSandbox:", aURL);

    if (!this._sandboxConfigured) {
      
      Cu.import("resource://gre/modules/DOMIdentity.jsm");
      DOMIdentity._configureMessages(Services.appShell.hiddenDOMWindow, true);
      this._sandboxConfigured = true;
    }

    new Sandbox(aURL, aCallback);
  },

  


  _beginAuthenticationFlow: function _beginAuthenticationFlow(aProvId, aURL) {
    logger.log("_beginAuthenticationFlow:", aProvId, aURL);
    let propBag = {provId: aProvId};

    Services.obs.notifyObservers({wrappedJSObject:propBag}, "identity-auth", aURL);
  },

  



  _cleanUpProvisionFlow: function _cleanUpProvisionFlow(aProvId) {
    logger.log('_cleanUpProvisionFlow:', aProvId);
    let prov = this._provisionFlows[aProvId];

    
    if (prov.provisioningSandbox) {
      let sandbox = this._provisionFlows[aProvId]['provisioningSandbox'];
      if (sandbox.free) {
        logger.log('_cleanUpProvisionFlow: freeing sandbox');
        sandbox.free();
      }
      delete this._provisionFlows[aProvId]['provisioningSandbox'];
    }

    
    if (this._authenticationFlows[prov.authId]) {
      delete this._authenticationFlows[prov.authId];
    }

    
    delete this._provisionFlows[aProvId];
  }

};

this.IdentityProvider = new IdentityProviderService();
