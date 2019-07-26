




































































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


const kOpenIdentityDialog = "id-dialog-open";
const kDoneIdentityDialog = "id-dialog-done";
const kCloseIdentityDialog = "id-dialog-close-iframe";


const kIdentityDelegateWatch = "identity-delegate-watch";
const kIdentityDelegateRequest = "identity-delegate-request";
const kIdentityDelegateLogout = "identity-delegate-logout";
const kIdentityDelegateFinished = "identity-delegate-finished";
const kIdentityDelegateReady = "identity-delegate-ready";

const kIdentityControllerDoMethod = "identity-controller-doMethod";

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["SignInToWebsiteController"].concat(aMessageArgs));
}







let ContentInterface = {
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

function Pipe() {
  this._watchers = [];
}

Pipe.prototype = {
  init: function pipe_init() {
    Services.obs.addObserver(this, "identity-child-process-shutdown", false);
    Services.obs.addObserver(this, "identity-controller-unwatch", false);
  },

  uninit: function pipe_uninit() {
    Services.obs.removeObserver(this, "identity-child-process-shutdown");
    Services.obs.removeObserver(this, "identity-controller-unwatch");
  },

  observe: function Pipe_observe(aSubject, aTopic, aData) {
    let options = {};
    if (aSubject) {
      options = aSubject.wrappedJSObject;
    }
    switch (aTopic) {
      case "identity-child-process-shutdown":
        log("pipe removing watchers by message manager");
        this._removeWatchers(null, options.messageManager);
        break;

      case "identity-controller-unwatch":
        log("unwatching", options.id);
        this._removeWatchers(options.id, options.messageManager);
        break;
    }
  },

  _addWatcher: function Pipe__addWatcher(aId, aMm) {
    log("Adding watcher with id", aId);
    for (let i = 0; i < this._watchers.length; ++i) {
      let watcher = this._watchers[i];
      if (this._watcher.id === aId) {
        watcher.count++;
        return;
      }
    }
    this._watchers.push({id: aId, count: 1, mm: aMm});
  },

  _removeWatchers: function Pipe__removeWatcher(aId, aMm) {
    let checkId = aId !== null;
    let index = -1;
    for (let i = 0; i < this._watchers.length; ++i) {
      let watcher = this._watchers[i];
      if (watcher.mm === aMm &&
          (!checkId || (checkId && watcher.id === aId))) {
        index = i;
        break;
      }
    }

    if (index !== -1) {
      if (checkId) {
        if (--(this._watchers[index].count) === 0) {
          this._watchers.splice(index, 1);
        }
      } else {
        this._watchers.splice(index, 1);
      }
    }

    if (this._watchers.length === 0) {
      log("No more watchers; clean up persona host iframe");
      let detail = {
        type: kCloseIdentityDialog
      };
      log('telling content to close the dialog');
      
      ContentInterface.sendChromeEvent(detail);
    }
  },

  communicate: function(aRpOptions, aContentOptions, aMessageCallback) {
    let rpID = aRpOptions.id;
    let rpMM = aRpOptions.mm;
    if (rpMM) {
      this._addWatcher(rpID, rpMM);
    }

    log("RP options:", aRpOptions, "\n  content options:", aContentOptions);

    
    
    
    let content = ContentInterface.getContent();
    let mm = null;
    let uuid = getRandomId();
    let self = this;

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
        type: kDoneIdentityDialog,
        showUI: aContentOptions.showUI || false,
        id: kDoneIdentityDialog + "-" + uuid,
        requestId: aRpOptions.id
      };
      log('received delegate finished; telling content to close the dialog');
      ContentInterface.sendChromeEvent(detail);
      self._removeWatchers(rpID, rpMM);
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

            mm.sendAsyncMessage(aContentOptions.message, aRpOptions);
          }
          break;

        case kDoneIdentityDialog + '-' + uuid:
          
          
          
          content.removeEventListener("mozContentEvent", getAssertion);
          break;

        default:
          log("ERROR - Unexpected message: id=" + msg.id + ", type=" + msg.type + ", errorMsg=" + msg.errorMsg);
          break;
      }

    });

    
    
    
    
    let detail = {
      type: kOpenIdentityDialog,
      showUI: aContentOptions.showUI || false,
      id: kOpenIdentityDialog + "-" + uuid,
      requestId: aRpOptions.id
    };

    ContentInterface.sendChromeEvent(detail);
  }

};










this.SignInToWebsiteController = {

  



  init: function SignInToWebsiteController_init(aOptions) {
    aOptions = aOptions || {};
    this.pipe = aOptions.pipe || new Pipe();
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
    switch (aTopic) {
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

      switch (message.method) {
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
    
    let contentOptions = {
      message: kIdentityDelegateWatch,
      showUI: false
    };
    this.pipe.communicate(aRpOptions, contentOptions,
        this._makeDoMethodCallback(aRpOptions.id));
  },

  


  doRequest: function SignInToWebsiteController_doRequest(aRpOptions) {
    log("doRequest", aRpOptions);
    let contentOptions = {
      message: kIdentityDelegateRequest,
      showUI: true
    };
    this.pipe.communicate(aRpOptions, contentOptions,
        this._makeDoMethodCallback(aRpOptions.id));
  },

  


  doLogout: function SignInToWebsiteController_doLogout(aRpOptions) {
    log("doLogout", aRpOptions);
    let contentOptions = {
      message: kIdentityDelegateLogout,
      showUI: false
    };
    this.pipe.communicate(aRpOptions, contentOptions,
        this._makeDoMethodCallback(aRpOptions.id));
  }

};
