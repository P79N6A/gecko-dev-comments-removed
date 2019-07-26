




































































"use strict";

this.EXPORTED_SYMBOLS = ["SignInToWebsiteController"];

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/identity/IdentityUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "IdentityService",
                                  "resource://gre/modules/identity/MinimalIdentity.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");



const kIdentityShimFile = "chrome://browser/content/identity.js";


const kOpenIdentityDialog = "open-id-dialog";
const kCloseIdentityDialog = "close-id-dialog";


const kReceivedIdentityAssertion = "received-id-assertion";
const kIdentityDelegateWatch = "identity-delegate-watch";
const kIdentityDelegateRequest = "identity-delegate-request";
const kIdentityDelegateLogout = "identity-delegate-logout";
const kIdentityDelegateFinished = "identity-delegate-finished";
const kIdentityDelegateReady = "identity-delegate-ready";

const kIdentityControllerDoMethod = "identity-controller-doMethod";

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["SignInToWebsiteController"].concat(aMessageArgs));
}







let GaiaInterface = {
  _getBrowser: function SignInToWebsiteController__getBrowser() {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },

  getContent: function SignInToWebsiteController_getContent() {
    return this._getBrowser().getContentWindow();
  },

  sendChromeEvent: function SignInToWebsiteController_sendChromeEvent(detail) {
    this._getBrowser().shell.sendChromeEvent(detail);
  }
};





let Pipe = {

  


















  communicate: function(aRpOptions, aGaiaOptions, aMessageCallback) {
    log("open gaia dialog with options:", aGaiaOptions);

    
    
    
    let content = GaiaInterface.getContent();
    let mm = null;
    let uuid = getRandomId();

    if (!content) {
      log("ERROR: what the what? no content window?");
      
      return;
    }

    function removeMessageListeners() {
      if (mm) {
        mm.removeMessageListener(kIdentityDelegateFinished, identityDelegateFinished);
        mm.removeMessageListener(kIdentityControllerDoMethod, aMessageCallback);
      }
    }

    function identityDelegateFinished() {
      removeMessageListeners();

      let detail = {
        type: kReceivedIdentityAssertion,
        showUI: aGaiaOptions.showUI || false,
        id: kReceivedIdentityAssertion + "-" + uuid
      };
      log('telling gaia to close the dialog');
      
      GaiaInterface.sendChromeEvent(detail);
    }

    content.addEventListener("mozContentEvent", function getAssertion(evt) {
      let msg = evt.detail;
      if (!msg.id.match(uuid)) {
        return;
      }

      switch (msg.id) {
        case kOpenIdentityDialog + '-' + uuid:
          if (msg.type === 'cancel') {
            
            content.removeEventListener("mozContentEvent", getAssertion);
            removeMessageListeners();
            aMessageCallback({json: {method: "cancel"}});
          } else {
            
            
            
            
            let frame = evt.detail.frame;
            let frameLoader = frame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
            mm = frameLoader.messageManager;
            try {
              mm.loadFrameScript(kIdentityShimFile, true);
              log("Loaded shim " + kIdentityShimFile + "\n");
            } catch (e) {
              log("Error loading ", kIdentityShimFile, " as a frame script: ", e);
            }

            
            
            
            
            
            mm.addMessageListener(kIdentityControllerDoMethod, aMessageCallback);
            mm.addMessageListener(kIdentityDelegateFinished, identityDelegateFinished);

            mm.sendAsyncMessage(aGaiaOptions.message, aRpOptions);
          }
          break;

        case kReceivedIdentityAssertion + '-' + uuid:
          
          
          
          content.removeEventListener("mozContentEvent", getAssertion);
          break;

        default:
          log("ERROR - Unexpected message: id=" + msg.id + ", type=" + msg.type + ", errorMsg=" + msg.errorMsg);
          break;
      }

    });

    
    
    
    
    let detail = {
      type: kOpenIdentityDialog,
      showUI: aGaiaOptions.showUI || false,
      id: kOpenIdentityDialog + "-" + uuid
    };

    GaiaInterface.sendChromeEvent(detail);
  }

};










this.SignInToWebsiteController = {

  



  init: function SignInToWebsiteController_init(aOptions) {
    aOptions = aOptions || {};
    this.pipe = aOptions.pipe || Pipe;
    Services.obs.addObserver(this, "identity-controller-watch", false);
    Services.obs.addObserver(this, "identity-controller-request", false);
    Services.obs.addObserver(this, "identity-controller-logout", false);
  },

  uninit: function SignInToWebsiteController_uninit() {
    Services.obs.removeObserver(this, "identity-controller-watch");
    Services.obs.removeObserver(this, "identity-controller-request");
    Services.obs.removeObserver(this, "identity-controller-logout");
  },

  observe: function SignInToWebsiteController_observe(aSubject, aTopic, aData) {
    log("observe: received", aTopic, "with", aData, "for", aSubject);
    let options = null;
    if (aSubject) {
      options = aSubject.wrappedJSObject;
    }
    switch(aTopic) {
      case "identity-controller-watch":
        this.doWatch(options);
        break;
      case "identity-controller-request":
        this.doRequest(options);
        break;
      case "identity-controller-logout":
        this.doLogout(options);
        break;
      default:
        Logger.reportError("SignInToWebsiteController", "Unknown observer notification:", aTopic);
        break;
    }
  },

  



  _makeDoMethodCallback: function SignInToWebsiteController__makeDoMethodCallback(aRpId) {
    return function SignInToWebsiteController_methodCallback(aOptions) {
      let message = aOptions.json;
      if (typeof message === 'string') {
        message = JSON.parse(message);
      }

      switch(message.method) {
        case "ready":
          IdentityService.doReady(aRpId);
          break;

        case "login":
           if (message._internalParams) {
             IdentityService.doLogin(aRpId, message.assertion, message._internalParams);
           } else {
             IdentityService.doLogin(aRpId, message.assertion);
           }
          break;

        case "logout":
          IdentityService.doLogout(aRpId);
          break;

        case "cancel":
          IdentityService.doCancel(aRpId);
          break;

        default:
          log("WARNING: wonky method call:", message.method);
          break;
      }
    };
  },

  doWatch: function SignInToWebsiteController_doWatch(aRpOptions) {
    
    var gaiaOptions = {
      message: kIdentityDelegateWatch,
      showUI: false
    };
    this.pipe.communicate(aRpOptions, gaiaOptions, this._makeDoMethodCallback(aRpOptions.id));
  },

  


  doRequest: function SignInToWebsiteController_doRequest(aRpOptions) {
    log("doRequest", aRpOptions);
    
    var gaiaOptions = {
      message: kIdentityDelegateRequest,
      showUI: true
    };
    this.pipe.communicate(aRpOptions, gaiaOptions, this._makeDoMethodCallback(aRpOptions.id));
  },

  


  doLogout: function SignInToWebsiteController_doLogout(aRpOptions) {
    log("doLogout", aRpOptions);
    var gaiaOptions = {
      message: kIdentityDelegateLogout,
      showUI: false
    };
    this.pipe.communicate(aRpOptions, gaiaOptions, this._makeDoMethodCallback(aRpOptions.id));
  }

};
