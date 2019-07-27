



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/loop/LoopCalls.jsm");
Cu.import("resource:///modules/loop/MozLoopService.jsm");
Cu.import("resource:///modules/loop/LoopRooms.jsm");
Cu.import("resource:///modules/loop/LoopContacts.jsm");
Cu.importGlobalProperties(["Blob"]);

XPCOMUtils.defineLazyModuleGetter(this, "LoopContacts",
                                        "resource:///modules/loop/LoopContacts.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoopStorage",
                                        "resource:///modules/loop/LoopStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "hookWindowCloseForPanelClose",
                                        "resource://gre/modules/MozSocialAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PageMetadata",
                                        "resource://gre/modules/PageMetadata.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                        "resource://gre/modules/PluralForm.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                        "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITour",
                                        "resource:///modules/UITour.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Social",
                                        "resource:///modules/Social.jsm");
XPCOMUtils.defineLazyGetter(this, "appInfo", function() {
  return Cc["@mozilla.org/xre/app-info;1"]
           .getService(Ci.nsIXULAppInfo)
           .QueryInterface(Ci.nsIXULRuntime);
});
XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                         "@mozilla.org/widget/clipboardhelper;1",
                                         "nsIClipboardHelper");
XPCOMUtils.defineLazyServiceGetter(this, "extProtocolSvc",
                                         "@mozilla.org/uriloader/external-protocol-service;1",
                                         "nsIExternalProtocolService");
this.EXPORTED_SYMBOLS = ["injectLoopAPI"];









const cloneErrorObject = function(error, targetWindow) {
  let obj = new targetWindow.Error();
  let props = Object.getOwnPropertyNames(error);
  
  
  if (!props.length) {
    props.push("message", "filename", "lineNumber", "columnNumber", "stack");
  }
  for (let prop of props) {
    let value = error[prop];
    
    if (typeof value == "undefined") {
      continue;
    }
    if (typeof value != "string" && typeof value != "number") {
      value = String(value);
    }

    Object.defineProperty(Cu.waiveXrays(obj), prop, {
      configurable: false,
      enumerable: true,
      value: value,
      writable: false
    });
  }
  return obj;
};










const cloneValueInto = function(value, targetWindow) {
  if (!value || typeof value != "object") {
    return value;
  }

  
  if (("error" in value) && (value.error instanceof Ci.nsIException)) {
    value = value.error;
  }

  
  
  for (let prop of Object.getOwnPropertyNames(value)) {
    if (typeof value[prop] == "function") {
      delete value[prop];
    }
  }

  
  if (value.constructor.name == "Error" || value instanceof Ci.nsIException) {
    return cloneErrorObject(value, targetWindow);
  }

  let clone;
  try {
    clone = Cu.cloneInto(value, targetWindow);
  } catch (ex) {
    MozLoopService.log.debug("Failed to clone value:", value);
    throw ex;
  }

  return clone;
};






const toHexString = function(charCode) {
  return ("0" + charCode.toString(16)).slice(-2);
};








const injectObjectAPI = function(api, targetWindow) {
  let injectedAPI = {};
  
  
  Object.keys(api).forEach(func => {
    injectedAPI[func] = function(...params) {
      let lastParam = params.pop();
      let callbackIsFunction = (typeof lastParam == "function");
      
      params = [cloneValueInto(p, api) for (p of params)];

      
      
      if (callbackIsFunction) {
        api[func](...params, function callback(...results) {
          
          
          if (callbackIsFunction && typeof lastParam != "function") {
            MozLoopService.log.debug(func + ": callback function was lost.");
            
            if (func == "on" && api.off) {
              api.off(results[0], callback);
              return;
            }
            
            if (results[0]) {
              MozLoopService.log.error(func + " error:", results[0]);
            }
            return;
          }
          lastParam(...[cloneValueInto(r, targetWindow) for (r of results)]);
        });
      } else {
        try {
          lastParam = cloneValueInto(lastParam, api);
          return cloneValueInto(api[func](...params, lastParam), targetWindow);
        } catch (ex) {
          MozLoopService.log.error(func + " error: ", ex, params, lastParam);
          return cloneValueInto(ex, targetWindow);
        }
      }
    };
  });

  let contentObj = Cu.cloneInto(injectedAPI, targetWindow, {cloneFunctions: true});
  
  
  
  try {
    Object.seal(Cu.waiveXrays(contentObj));
  } catch (ex) {}
  return contentObj;
};









function injectLoopAPI(targetWindow) {
  let ringer;
  let ringerStopper;
  let appVersionInfo;
  let contactsAPI;
  let roomsAPI;
  let callsAPI;
  let savedWindowListeners = new Map();
  let socialProviders;
  const kShareWidgetId = "social-share-button";
  let socialShareButtonListenersAdded = false;


  let api = {
    





    userProfile: {
      enumerable: true,
      get: function() {
        if (!MozLoopService.userProfile)
          return null;
        let userProfile = Cu.cloneInto({
          email: MozLoopService.userProfile.email,
          uid: MozLoopService.userProfile.uid
        }, targetWindow);
        return userProfile;
      }
    },

    


    doNotDisturb: {
      enumerable: true,
      get: function() {
        return MozLoopService.doNotDisturb;
      },
      set: function(aFlag) {
        MozLoopService.doNotDisturb = aFlag;
      }
    },

    errors: {
      enumerable: true,
      get: function() {
        let errors = {};
        for (let [type, error] of MozLoopService.errors) {
          
          
          if (error.error instanceof Ci.nsIException) {
            MozLoopService.log.debug("Warning: Some errors were omitted from MozLoopAPI.errors " +
                                     "due to issues copying nsIException across boundaries.",
                                     error.error);
            delete error.error;
          }

          
          if (error.hasOwnProperty("toString")) {
            delete error.toString;
          }
          errors[type] = Cu.waiveXrays(Cu.cloneInto(error, targetWindow, { cloneFunctions: true }));
        }
        return Cu.cloneInto(errors, targetWindow, { cloneFunctions: true });
      },
    },

    




    locale: {
      enumerable: true,
      get: function() {
        return MozLoopService.locale;
      }
    },

    










    addBrowserSharingListener: {
      enumerable: true,
      writable: true,
      value: function(listener) {
        let win = Services.wm.getMostRecentWindow("navigator:browser");
        let browser = win && win.gBrowser.selectedBrowser;
        if (!win || !browser) {
          
          
          let err = new Error("No tabs available to share.");
          MozLoopService.log.error(err);
          listener(cloneValueInto(err, targetWindow));
          return;
        }
        if (browser.getAttribute("remote") == "true") {
          
          
          let err = new Error("Tab sharing is not supported for e10s-enabled browsers");
          MozLoopService.log.error(err);
          listener(cloneValueInto(err, targetWindow));
          return;
        }

        win.LoopUI.addBrowserSharingListener(listener);

        savedWindowListeners.set(listener, Cu.getWeakReference(win));
      }
    },

    




    removeBrowserSharingListener: {
      enumerable: true,
      writable: true,
      value: function(listener) {
        if (!savedWindowListeners.has(listener)) {
          return;
        }

        let win = savedWindowListeners.get(listener).get();

        
        
        savedWindowListeners.delete(listener);

        if (!win) {
          return;
        }

        win.LoopUI.removeBrowserSharingListener(listener);
      }
    },

    








    getConversationWindowData: {
      enumerable: true,
      writable: true,
      value: function(conversationWindowId) {
        return cloneValueInto(MozLoopService.getConversationWindowData(conversationWindowId),
          targetWindow);
      }
    },

    




    contacts: {
      enumerable: true,
      get: function() {
        if (contactsAPI) {
          return contactsAPI;
        }

        
        let profile = MozLoopService.userProfile;
        if (profile) {
          LoopStorage.switchDatabase(profile.uid);
        }
        return contactsAPI = injectObjectAPI(LoopContacts, targetWindow);
      }
    },

    




    rooms: {
      enumerable: true,
      get: function() {
        if (roomsAPI) {
          return roomsAPI;
        }
        return roomsAPI = injectObjectAPI(LoopRooms, targetWindow);
      }
    },

    




    calls: {
      enumerable: true,
      get: function() {
        if (callsAPI) {
          return callsAPI;
        }

        return callsAPI = injectObjectAPI(LoopCalls, targetWindow);
      }
    },

    








    startImport: {
      enumerable: true,
      writable: true,
      value: function(options, callback) {
        LoopContacts.startImport(options, getChromeWindow(targetWindow), function(...results) {
          callback(...[cloneValueInto(r, targetWindow) for (r of results)]);
        });
      }
    },

    







    getStrings: {
      enumerable: true,
      writable: true,
      value: function(key) {
        return MozLoopService.getStrings(key);
      }
    },

    








    getPluralForm: {
      enumerable: true,
      writable: true,
      value: function(num, str) {
        return PluralForm.get(num, str);
      }
    },

    









    confirm: {
      enumerable: true,
      writable: true,
      value: function(options, callback) {
        let buttonFlags;
        if (options.okButton && options.cancelButton) {
          buttonFlags =
            (Ci.nsIPrompt.BUTTON_POS_0 * Ci.nsIPrompt.BUTTON_TITLE_IS_STRING) +
            (Ci.nsIPrompt.BUTTON_POS_1 * Ci.nsIPrompt.BUTTON_TITLE_IS_STRING);
        } else if (!options.okButton && !options.cancelButton) {
          buttonFlags = Services.prompt.STD_YES_NO_BUTTONS;
        } else {
          callback(cloneValueInto(new Error("confirm: missing button options"), targetWindow));
        }

        try {
          let chosenButton = Services.prompt.confirmEx(null, "",
            options.message, buttonFlags, options.okButton, options.cancelButton,
            null, null, {});

          callback(null, chosenButton == 0);
        } catch (ex) {
          callback(cloneValueInto(ex, targetWindow));
        }
      }
    },

    









    setLoopPref: {
      enumerable: true,
      writable: true,
      value: function(prefName, value, prefType) {
        MozLoopService.setLoopPref(prefName, value, prefType);
      }
    },

    












    getLoopPref: {
      enumerable: true,
      writable: true,
      value: function(prefName, prefType) {
        return MozLoopService.getLoopPref(prefName);
      }
    },

    


    startAlerting: {
      enumerable: true,
      writable: true,
      value: function() {
        let chromeWindow = getChromeWindow(targetWindow);
        chromeWindow.getAttention();
        ringer = new chromeWindow.Audio();
        ringer.src = Services.prefs.getCharPref("loop.ringtone");
        ringer.loop = true;
        ringer.load();
        ringer.play();
        targetWindow.document.addEventListener("visibilitychange",
          ringerStopper = function(event) {
            if (event.currentTarget.hidden) {
              api.stopAlerting.value();
            }
          });
      }
    },

    


    stopAlerting: {
      enumerable: true,
      writable: true,
      value: function() {
        if (ringerStopper) {
          targetWindow.document.removeEventListener("visibilitychange",
                                                    ringerStopper);
          ringerStopper = null;
        }
        if (ringer) {
          ringer.pause();
          ringer = null;
        }
      }
    },

    





















    hawkRequest: {
      enumerable: true,
      writable: true,
      value: function(sessionType, path, method, payloadObj, callback) {
        
        let callbackIsFunction = (typeof callback == "function");
        MozLoopService.hawkRequest(sessionType, path, method, payloadObj).then((response) => {
          callback(null, response.body);
        }, hawkError => {
          
          
          if (callbackIsFunction && typeof callback != "function") {
            MozLoopService.log.error("hawkRequest: callback function was lost.", hawkError);
            return;
          }
          
          
          
          callback(Cu.cloneInto({
            error: (hawkError.error && typeof hawkError.error == "string")
                   ? hawkError.error : "Unexpected exception",
            message: hawkError.message,
            code: hawkError.code,
            errno: hawkError.errno,
          }, targetWindow));
        }).catch(Cu.reportError);
      }
    },

    LOOP_SESSION_TYPE: {
      enumerable: true,
      get: function() {
        return Cu.cloneInto(LOOP_SESSION_TYPE, targetWindow);
      }
    },

    TWO_WAY_MEDIA_CONN_LENGTH: {
      enumerable: true,
      get: function() {
        return Cu.cloneInto(TWO_WAY_MEDIA_CONN_LENGTH, targetWindow);
      }
    },

    SHARING_STATE_CHANGE: {
      enumerable: true,
      get: function() {
        return Cu.cloneInto(SHARING_STATE_CHANGE, targetWindow);
      }
    },

    fxAEnabled: {
      enumerable: true,
      get: function() {
        return MozLoopService.fxAEnabled;
      },
    },

    logInToFxA: {
      enumerable: true,
      writable: true,
      value: function() {
        return MozLoopService.logInToFxA();
      }
    },

    logOutFromFxA: {
      enumerable: true,
      writable: true,
      value: function() {
        return MozLoopService.logOutFromFxA();
      }
    },

    openFxASettings: {
      enumerable: true,
      writable: true,
      value: function() {
        return MozLoopService.openFxASettings();
      },
    },

    





    openGettingStartedTour: {
      enumerable: true,
      writable: true,
      value: function(aSrc) {
        return MozLoopService.openGettingStartedTour(aSrc);
      },
    },

    




    copyString: {
      enumerable: true,
      writable: true,
      value: function(str) {
        clipboardHelper.copyString(str);
      }
    },

    







    appVersionInfo: {
      enumerable: true,
      get: function() {
        if (!appVersionInfo) {
          
          
          try {
            appVersionInfo = Cu.cloneInto({
              channel: UpdateChannel.get(),
              version: appInfo.version,
              OS: appInfo.OS
            }, targetWindow);
          } catch (ex) {
            
            if (typeof targetWindow !== 'undefined' && "console" in targetWindow) {
              MozLoopService.log.error("Failed to construct appVersionInfo; if this isn't " +
                                       "an xpcshell unit test, something is wrong", ex);
            }
          }
        }
        return appVersionInfo;
      }
    },

    






    composeEmail: {
      enumerable: true,
      writable: true,
      value: function(subject, body, recipient) {
        recipient = recipient || "";
        let mailtoURL = "mailto:" + encodeURIComponent(recipient) +
                        "?subject=" + encodeURIComponent(subject) +
                        "&body=" + encodeURIComponent(body);
        extProtocolSvc.loadURI(CommonUtils.makeURI(mailtoURL));
      }
    },

    





    telemetryAddKeyedValue: {
      enumerable: true,
      writable: true,
      value: function(histogramId, value) {
        Services.telemetry.getKeyedHistogramById(histogramId).add(value);
      }
    },

    


    generateUUID: {
      enumerable: true,
      writable: true,
      value: function() {
        return MozLoopService.generateUUID();
      }
    },

    getAudioBlob: {
      enumerable: true,
      writable: true,
      value: function(name, callback) {
        let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                        .createInstance(Ci.nsIXMLHttpRequest);
        let url = `chrome://browser/content/loop/shared/sounds/${name}.ogg`;

        request.open("GET", url, true);
        request.responseType = "arraybuffer";
        request.onload = () => {
          if (request.status < 200 || request.status >= 300) {
            let error = new Error(request.status + " " + request.statusText);
            callback(cloneValueInto(error, targetWindow));
            return;
          }

          let blob = new Blob([request.response], {type: "audio/ogg"});
          callback(null, cloneValueInto(blob, targetWindow));
        };

        request.send();
      }
    },

    










    getUserAvatar: {
      enumerable: true,
      writable: true,
      value: function(emailAddress, size = 40) {
        const kEmptyGif = "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7";
        if (!emailAddress || !MozLoopService.getLoopPref("contacts.gravatars.show")) {
          return kEmptyGif;
        }

        
        let hasher = Cc["@mozilla.org/security/hash;1"]
                       .createInstance(Ci.nsICryptoHash);
        hasher.init(Ci.nsICryptoHash.MD5);
        let stringStream = Cc["@mozilla.org/io/string-input-stream;1"]
                             .createInstance(Ci.nsIStringInputStream);
        stringStream.data = emailAddress.trim().toLowerCase();
        hasher.updateFromStream(stringStream, -1);
        let hash = hasher.finish(false);
        
        let md5Email = [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");

        
        return "https://www.gravatar.com/avatar/" + md5Email + ".jpg?default=blank&s=" + size;
      }
    },

    





    getSelectedTabMetadata: {
      value: function(callback) {
        let win = Services.wm.getMostRecentWindow("navigator:browser");
        win.messageManager.addMessageListener("PageMetadata:PageDataResult", function onPageDataResult(msg) {
          win.messageManager.removeMessageListener("PageMetadata:PageDataResult", onPageDataResult);
          let pageData = msg.json;
          callback(cloneValueInto(pageData, targetWindow));
        });
        win.gBrowser.selectedBrowser.messageManager.sendAsyncMessage("PageMetadata:GetPageData");
      }
    },

    






    addConversationContext: {
      enumerable: true,
      writable: true,
      value: function(windowId, sessionId, callid) {
        MozLoopService.addConversationContext(windowId, {
          sessionId: sessionId,
          callId: callid
        });
      }
    },

    







    notifyUITour: {
      enumerable: true,
      writable: true,
      value: function(subject, params) {
        UITour.notify(subject, params);
      }
    },

    







    setScreenShareState: {
      enumerable: true,
      writable: true,
      value: function(windowId, active) {
        MozLoopService.setScreenShareState(windowId, active);
      }
    },

    






    isSocialShareButtonAvailable: {
      enumerable: true,
      writable: true,
      value: function() {
        let win = Services.wm.getMostRecentWindow("navigator:browser");
        if (!win || !win.CustomizableUI) {
          return false;
        }

        let widget = win.CustomizableUI.getWidget(kShareWidgetId);
        if (widget) {
          if (!socialShareButtonListenersAdded) {
            let eventName = "social:" + kShareWidgetId;
            Services.obs.addObserver(onShareWidgetChanged, eventName + "-added", false);
            Services.obs.addObserver(onShareWidgetChanged, eventName + "-removed", false);
            socialShareButtonListenersAdded = true;
          }
          return !!widget.areaType;
        }

        return false;
      }
    },

    



    addSocialShareButton: {
      enumerable: true,
      writable: true,
      value: function() {
        
        if (api.isSocialShareButtonAvailable.value()) {
          return;
        }

        let win = Services.wm.getMostRecentWindow("navigator:browser");
        if (!win || !win.CustomizableUI) {
          return;
        }
        win.CustomizableUI.addWidgetToArea(kShareWidgetId, win.CustomizableUI.AREA_NAVBAR);
      }
    },

    



    addSocialShareProvider: {
      enumerable: true,
      writable: true,
      value: function() {
        
        if (!api.isSocialShareButtonAvailable.value()) {
          return;
        }

        let win = Services.wm.getMostRecentWindow("navigator:browser");
        if (!win || !win.SocialShare) {
          return;
        }
        win.SocialShare.showDirectory();
      }
    },

    





    getSocialShareProviders: {
      enumerable: true,
      writable: true,
      value: function() {
        if (socialProviders) {
          return socialProviders;
        }
        return updateSocialProvidersCache();
      }
    },

    











    socialShareRoom: {
      enumerable: true,
      writable: true,
      value: function(providerOrigin, roomURL, title, body = null) {
        let win = Services.wm.getMostRecentWindow("navigator:browser");
        if (!win || !win.SocialShare) {
          return;
        }

        let graphData = {
          url: roomURL,
          title: title
        };
        if (body) {
          graphData.body = body;
        }
        win.SocialShare.sharePage(providerOrigin, graphData);
      }
    }
  };

  





  function sendEvent(name = "LoopStatusChanged") {
    if (typeof targetWindow.CustomEvent != "function") {
      MozLoopService.log.debug("Could not send event to content document, " +
        "because it's being destroyed or we're in a unit test where " +
        "`targetWindow` is mocked.");
      return;
    }

    let event = new targetWindow.CustomEvent(name);
    targetWindow.dispatchEvent(event);
  }

  function onStatusChanged(aSubject, aTopic, aData) {
    sendEvent();
  }

  function onDOMWindowDestroyed(aSubject, aTopic, aData) {
    if (targetWindow && aSubject != targetWindow)
      return;
    Services.obs.removeObserver(onDOMWindowDestroyed, "dom-window-destroyed");
    Services.obs.removeObserver(onStatusChanged, "loop-status-changed");
    
    if (socialProviders)
      Services.obs.removeObserver(updateSocialProvidersCache, "social:providers-changed");
    if (socialShareButtonListenersAdded) {
      let eventName = "social:" + kShareWidgetId;
      Services.obs.removeObserver(onShareWidgetChanged, eventName + "-added");
      Services.obs.removeObserver(onShareWidgetChanged, eventName + "-removed");
    }
  }

  function onShareWidgetChanged(aSubject, aTopic, aData) {
    sendEvent("LoopShareWidgetChanged");
  }

  







  function updateSocialProvidersCache() {
    let providers = [];

    for (let provider of Social.providers) {
      if (!provider.shareURL) {
        continue;
      }

      
      providers.push({
        iconURL: provider.iconURL,
        name: provider.name,
        origin: provider.origin
      });
    }

    let providersWasSet = !!socialProviders;
    
    socialProviders = cloneValueInto(providers.sort((a, b) =>
      a.name.toLowerCase().localeCompare(b.name.toLowerCase())), targetWindow);

    
    
    if (!providersWasSet) {
      Services.obs.addObserver(updateSocialProvidersCache, "social:providers-changed", false);
    } else {
      
      sendEvent("LoopSocialProvidersChanged");
    }

    return socialProviders;
  }

  let contentObj = Cu.createObjectIn(targetWindow);
  Object.defineProperties(contentObj, api);
  Object.seal(contentObj);
  Cu.makeObjectPropsNormal(contentObj);
  Services.obs.addObserver(onStatusChanged, "loop-status-changed", false);
  Services.obs.addObserver(onDOMWindowDestroyed, "dom-window-destroyed", false);

  if ("navigator" in targetWindow) {
    targetWindow.navigator.wrappedJSObject.__defineGetter__("mozLoop", function () {
      
      
      
      
      delete targetWindow.navigator.wrappedJSObject.mozLoop;
      return targetWindow.navigator.wrappedJSObject.mozLoop = contentObj;
    });

    
    hookWindowCloseForPanelClose(targetWindow);
  } else {
    
    return targetWindow.mozLoop = contentObj;
  }

}

function getChromeWindow(contentWin) {
  return contentWin.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShellTreeItem)
                   .rootTreeItem
                   .QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindow);
}
