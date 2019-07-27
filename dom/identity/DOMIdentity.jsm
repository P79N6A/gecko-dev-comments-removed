



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PREF_FXA_ENABLED = "identity.fxaccounts.enabled";
const FXA_PERMISSION = "firefox-accounts";


this.EXPORTED_SYMBOLS = ["DOMIdentity"];

XPCOMUtils.defineLazyModuleGetter(this, "objectCopy",
                                  "resource://gre/modules/identity/IdentityUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "IdentityService",
#ifdef MOZ_B2G_VERSION
                                  "resource://gre/modules/identity/MinimalIdentity.jsm");
#else
                                  "resource://gre/modules/identity/Identity.jsm");
#endif

XPCOMUtils.defineLazyModuleGetter(this, "FirefoxAccounts",
                                  "resource://gre/modules/identity/FirefoxAccounts.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "makeMessageObject",
                                  "resource://gre/modules/identity/IdentityUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
                                  "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "permissionManager",
                                   "@mozilla.org/permissionmanager;1",
                                   "nsIPermissionManager");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["DOMIdentity"].concat(aMessageArgs));
}

function IDDOMMessage(aOptions) {
  objectCopy(aOptions, this);
}

function IDPProvisioningContext(aID, aOrigin, aTargetMM) {
  this._id = aID;
  this._origin = aOrigin;
  this._mm = aTargetMM;
}

IDPProvisioningContext.prototype = {
  get id() this._id,
  get origin() this._origin,

  doBeginProvisioningCallback: function IDPPC_doBeginProvCB(aID, aCertDuration) {
    let message = new IDDOMMessage({id: this.id});
    message.identity = aID;
    message.certDuration = aCertDuration;
    this._mm.sendAsyncMessage("Identity:IDP:CallBeginProvisioningCallback",
                              message);
  },

  doGenKeyPairCallback: function IDPPC_doGenKeyPairCallback(aPublicKey) {
    log("doGenKeyPairCallback");
    let message = new IDDOMMessage({id: this.id});
    message.publicKey = aPublicKey;
    this._mm.sendAsyncMessage("Identity:IDP:CallGenKeyPairCallback", message);
  },

  doError: function(msg) {
    log("Provisioning ERROR: " + msg);
  }
};

function IDPAuthenticationContext(aID, aOrigin, aTargetMM) {
  this._id = aID;
  this._origin = aOrigin;
  this._mm = aTargetMM;
}

IDPAuthenticationContext.prototype = {
  get id() this._id,
  get origin() this._origin,

  doBeginAuthenticationCallback: function IDPAC_doBeginAuthCB(aIdentity) {
    let message = new IDDOMMessage({id: this.id});
    message.identity = aIdentity;
    this._mm.sendAsyncMessage("Identity:IDP:CallBeginAuthenticationCallback",
                              message);
  },

  doError: function IDPAC_doError(msg) {
    log("Authentication ERROR: " + msg);
  }
};

function RPWatchContext(aOptions, aTargetMM) {
  objectCopy(aOptions, this);

  
  if (! (this.id && this.origin)) {
    throw new Error("id and origin are required for RP watch context");
  }

  
  this.loggedInUser = aOptions.loggedInUser;

  
  this._internal = aOptions._internal;

  this._mm = aTargetMM;
}

RPWatchContext.prototype = {
  doLogin: function RPWatchContext_onlogin(aAssertion, aMaybeInternalParams) {
    log("doLogin: " + this.id);
    let message = new IDDOMMessage({id: this.id, assertion: aAssertion});
    if (aMaybeInternalParams) {
      message._internalParams = aMaybeInternalParams;
    }
    this._mm.sendAsyncMessage("Identity:RP:Watch:OnLogin", message);
  },

  doLogout: function RPWatchContext_onlogout() {
    log("doLogout: " + this.id);
    let message = new IDDOMMessage({id: this.id});
    this._mm.sendAsyncMessage("Identity:RP:Watch:OnLogout", message);
  },

  doReady: function RPWatchContext_onready() {
    log("doReady: " + this.id);
    let message = new IDDOMMessage({id: this.id});
    this._mm.sendAsyncMessage("Identity:RP:Watch:OnReady", message);
  },

  doCancel: function RPWatchContext_oncancel() {
    log("doCancel: " + this.id);
    let message = new IDDOMMessage({id: this.id});
    this._mm.sendAsyncMessage("Identity:RP:Watch:OnCancel", message);
  },

  doError: function RPWatchContext_onerror(aMessage) {
    log("doError: " + this.id + ": " + JSON.stringify(aMessage));
    let message = new IDDOMMessage({id: this.id, message: aMessage});
    this._mm.sendAsyncMessage("Identity:RP:Watch:OnError", message);
  }
};

this.DOMIdentity = {
  












  _serviceContexts: new Map(),
  _mmContexts: new Map(),

  


  _mockIdentityService: null,
  get IdentityService() {
    if (this._mockIdentityService) {
      log("Using a mocked identity service");
      return this._mockIdentityService;
    }
    return IdentityService;
  },

  


  newContext: function(message, targetMM) {
    let context = new RPWatchContext(message, targetMM);
    this._serviceContexts.set(message.id, context);
    this._mmContexts.set(targetMM, message.id);
    return context;
  },

  








  getService: function(message) {
    if (!this._serviceContexts.has(message.id)) {
      throw new Error("getService called before newContext for " + message.id);
    }

    let context = this._serviceContexts.get(message.id);
    if (context.wantIssuer == "firefox-accounts") {
      if (Services.prefs.getPrefType(PREF_FXA_ENABLED) === Ci.nsIPrefBranch.PREF_BOOL
          && Services.prefs.getBoolPref(PREF_FXA_ENABLED)) {
        return FirefoxAccounts;
      }
      log("WARNING: Firefox Accounts is not enabled; Defaulting to BrowserID");
    }
    return this.IdentityService;
  },

  


  getContextForMM: function(targetMM) {
    return this._serviceContexts.get(this._mmContexts.get(targetMM));
  },

  hasContextForMM: function(targetMM) {
    return this._mmContexts.has(targetMM);
  },

  



  deleteContextForMM: function(targetMM) {
    this._serviceContexts.delete(this._mmContexts.get(targetMM));
    this._mmContexts.delete(targetMM);
  },

  hasPermission: function(aMessage) {
    
    
    if (aMessage.json && aMessage.json.wantIssuer == "firefox-accounts") {
      if (!aMessage.principal) {
        return false;
      }
      let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
                     .getService(Ci.nsIScriptSecurityManager);
      let uri = Services.io.newURI(aMessage.principal.origin, null, null);
      let principal = secMan.getAppCodebasePrincipal(uri,
        aMessage.principal.appId, aMessage.principal.isInBrowserElement);

      let permission =
        permissionManager.testPermissionFromPrincipal(principal,
                                                      FXA_PERMISSION);
      return permission != Ci.nsIPermissionManager.UNKNOWN_ACTION &&
             permission != Ci.nsIPermissionManager.DENY_ACTION;
    }
    return true;
  },

  
  receiveMessage: function DOMIdentity_receiveMessage(aMessage) {
    let msg = aMessage.json;

    
    
    let targetMM = aMessage.target;

    if (!this.hasPermission(aMessage)) {
      throw new Error("PERMISSION_DENIED");
    }

    switch (aMessage.name) {
      
      case "Identity:RP:Watch":
        this._watch(msg, targetMM);
        break;
      case "Identity:RP:Unwatch":
        this._unwatch(msg, targetMM);
        break;
      case "Identity:RP:Request":
        this._request(msg, targetMM);
        break;
      case "Identity:RP:Logout":
        this._logout(msg, targetMM);
        break;
      
      case "Identity:IDP:BeginProvisioning":
        this._beginProvisioning(msg, targetMM);
        break;
      case "Identity:IDP:GenKeyPair":
        this._genKeyPair(msg);
        break;
      case "Identity:IDP:RegisterCertificate":
        this._registerCertificate(msg);
        break;
      case "Identity:IDP:ProvisioningFailure":
        this._provisioningFailure(msg);
        break;
      case "Identity:IDP:BeginAuthentication":
        this._beginAuthentication(msg, targetMM);
        break;
      case "Identity:IDP:CompleteAuthentication":
        this._completeAuthentication(msg);
        break;
      case "Identity:IDP:AuthenticationFailure":
        this._authenticationFailure(msg);
        break;
      case "child-process-shutdown":
        
        
        
        this._childProcessShutdown(targetMM);
        break;
    }
  },

  
  observe: function DOMIdentity_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown":
        this._unsubscribeListeners();
        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.ww.unregisterNotification(this);
        break;
    }
  },

  messages: ["Identity:RP:Watch", "Identity:RP:Request", "Identity:RP:Logout",
             "Identity:IDP:BeginProvisioning", "Identity:IDP:ProvisioningFailure",
             "Identity:IDP:RegisterCertificate", "Identity:IDP:GenKeyPair",
             "Identity:IDP:BeginAuthentication",
             "Identity:IDP:CompleteAuthentication",
             "Identity:IDP:AuthenticationFailure",
             "Identity:RP:Unwatch",
             "child-process-shutdown"],

  
  _init: function DOMIdentity__init() {
    Services.ww.registerNotification(this);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    this._subscribeListeners();
  },

  _subscribeListeners: function DOMIdentity__subscribeListeners() {
    if (!ppmm) return;
    for (let message of this.messages) {
      ppmm.addMessageListener(message, this);
    }
  },

  _unsubscribeListeners: function DOMIdentity__unsubscribeListeners() {
    for (let message of this.messages) {
      ppmm.removeMessageListener(message, this);
    }
    ppmm = null;
  },

  _watch: function DOMIdentity__watch(message, targetMM) {
    log("DOMIdentity__watch: " + message.id);
    let context = this.newContext(message, targetMM);
    this.getService(message).RP.watch(context);
  },

  _unwatch: function DOMIdentity_unwatch(message, targetMM) {
    log("DOMIDentity__unwatch: " + message.id);
    
    
    
    
    try {
      this.getService(message).RP.unwatch(message.id, targetMM);
    } catch(ex) {
      log("ERROR: can't unwatch " + message.id + ": " + ex);
    }
  },

  _request: function DOMIdentity__request(message) {
    this.getService(message).RP.request(message.id, message);
  },

  _logout: function DOMIdentity__logout(message) {
    log("logout " + message + "\n");
    this.getService(message).RP.logout(message.id, message.origin, message);
  },

  _childProcessShutdown: function DOMIdentity__childProcessShutdown(targetMM) {
    if (!this.hasContextForMM(targetMM)) {
      return;
    }

    this.getContextForMM(targetMM).RP.childProcessShutdown(targetMM);
    this.deleteContextForMM(targetMM);

    let options = makeMessageObject({messageManager: targetMM, id: null, origin: null});
    Services.obs.notifyObservers({wrappedJSObject: options}, "identity-child-process-shutdown", null);
  },

  _beginProvisioning: function DOMIdentity__beginProvisioning(message, targetMM) {
    let context = new IDPProvisioningContext(message.id, message.origin,
                                             targetMM);
    this.getService(message).IDP.beginProvisioning(context);
  },

  _genKeyPair: function DOMIdentity__genKeyPair(message) {
    this.getService(message).IDP.genKeyPair(message.id);
  },

  _registerCertificate: function DOMIdentity__registerCertificate(message) {
    this.getService(message).IDP.registerCertificate(message.id, message.cert);
  },

  _provisioningFailure: function DOMIdentity__provisioningFailure(message) {
    this.getService(message).IDP.raiseProvisioningFailure(message.id, message.reason);
  },

  _beginAuthentication: function DOMIdentity__beginAuthentication(message, targetMM) {
    let context = new IDPAuthenticationContext(message.id, message.origin,
                                               targetMM);
    this.getService(message).IDP.beginAuthentication(context);
  },

  _completeAuthentication: function DOMIdentity__completeAuthentication(message) {
    this.getService(message).IDP.completeAuthentication(message.id);
  },

  _authenticationFailure: function DOMIdentity__authenticationFailure(message) {
    this.getService(message).IDP.cancelAuthentication(message.id);
  }
};


