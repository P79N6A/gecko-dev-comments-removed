







"use strict";

const EXPORTED_SYMBOLS = ["SignInToWebsiteController"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "IdentityService",
                                  "resource://gre/modules/identity/MinimalIdentity.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");




const kIdentityShimFile = "chrome://browser/content/identity.js";


const kOpenIdentityDialog = "open-id-dialog";
const kCloseIdentityDialog = "close-id-dialog";


XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["SignInToWebsiteController"].concat(aMessageArgs));
}

let SignInToWebsiteController = {

  init: function SignInToWebsiteController_init() {
    log("SignInToWebsiteController: init");
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

  getRandomId: function getRandomId() {
    return uuidgen.generateUUID().toString();
  },

  doWatch: function SignInToWebsiteController_doWatch(aOptions) {
    
    
    IdentityService.doReady(aOptions.rpId);
  },

  


  doRequest: function SignInToWebsiteController_doRequest(aOptions) {
    log("request options", aOptions);
    this._openDialogAndSendMessage(aOptions.rpId, "identity-delegate-request", aOptions);
  },

  


  doLogout: function SignInToWebsiteController_doLogout(aOptions) {
    log("logout options", aOptions);
    IdentityService.doLogout(aOptions.rpId);
  },

  
  
  _openDialogAndSendMessage: function SignInToWebsiteController_openDialogAndSendMessage(aRpId, aMessage, aOptions) {
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content) {
      
      return;
    }

    
    let id = kOpenIdentityDialog + "-" + this.getRandomId();
    let detail = {
      type: kOpenIdentityDialog,
      id: id
    };

    log("id before is ", id);
    
    content.addEventListener("mozContentEvent", function getAssertion(evt) {
      log("id after is ", id);
      let msg = evt.detail;
      if (msg.id != id) {
        log("mozContentEvent. evt.detail.id != ", id, msg);
        content.removeEventListener("mozContentEvent", getAssertion);
        return;
      }

      
      
      let frame = evt.detail.frame;
      let frameLoader = frame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
      let mm = frameLoader.messageManager;
      try {
        log("about to load frame script");
        mm.loadFrameScript(kIdentityShimFile, true);
      } catch (e) {
        log("Error loading ", kIdentityShimFile, " as a frame script: ", e);
      }

      mm.addMessageListener("identity-delegate-return-assertion", function(message) {
        log("back with assertion", message.json);
        if (message.json.assertion)
          IdentityService.doLogin(aRpId, message.json.assertion);

        IdentityService.doReady(aRpId);
      });

      
      log("sending message" , aMessage, aOptions);
      mm.sendAsyncMessage(aMessage, aOptions);

      log("done load frame script");

      content.removeEventListener("mozContentEvent", getAssertion);
    });

    browser.shell.sendChromeEvent(detail);
    log("sent", detail, "to gaia");
  }

};
