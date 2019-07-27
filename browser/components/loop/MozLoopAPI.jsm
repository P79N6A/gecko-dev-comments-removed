



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
XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                        "resource://gre/modules/PluralForm.jsm");
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
  for (let prop of Object.getOwnPropertyNames(error)) {
    let value = error[prop];
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

  
  
  for (let prop of Object.getOwnPropertyNames(value)) {
    if (typeof value[prop] == "function") {
      delete value[prop];
    }
  }

  
  if (value.constructor.name == "Error") {
    return cloneErrorObject(value, targetWindow);
  }

  return Cu.cloneInto(value, targetWindow);
};








const injectObjectAPI = function(api, targetWindow) {
  let injectedAPI = {};
  
  
  Object.keys(api).forEach(func => {
    injectedAPI[func] = function(...params) {
      let lastParam = params.pop();

      
      
      if (lastParam && typeof lastParam === "function") {
        api[func](...params, function(...results) {
          lastParam(...[cloneValueInto(r, targetWindow) for (r of results)]);
        });
      } else {
        try {
          return cloneValueInto(api[func](...params, lastParam), targetWindow);
        } catch (ex) {
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

    








    getConversationWindowData: {
      enumerable: true,
      writable: true,
      value: function(conversationWindowId) {
        return Cu.cloneInto(MozLoopService.getConversationWindowData(conversationWindowId),
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
      value: function(bodyMessage, okButtonMessage, cancelButtonMessage, callback) {
        try {
          let buttonFlags =
            (Ci.nsIPrompt.BUTTON_POS_0 * Ci.nsIPrompt.BUTTON_TITLE_IS_STRING) +
            (Ci.nsIPrompt.BUTTON_POS_1 * Ci.nsIPrompt.BUTTON_TITLE_IS_STRING);

          let chosenButton = Services.prompt.confirmEx(null, "",
            bodyMessage, buttonFlags, okButtonMessage, cancelButtonMessage,
            null, null, {});

          callback(null, chosenButton == 0);
        } catch (ex) {
          callback(cloneValueInto(ex, targetWindow));
        }
      }
    },

    











    ensureRegistered: {
      enumerable: true,
      writable: true,
      value: function(sessionType, callback) {
        
        
        MozLoopService.promiseRegisteredWithServers(sessionType).then(() => {
          callback(null);
        }, err => {
          callback(cloneValueInto(err, targetWindow));
        }).catch(Cu.reportError);
      }
    },

    











    noteCallUrlExpiry: {
      enumerable: true,
      writable: true,
      value: function(expiryTimeSeconds) {
        MozLoopService.noteCallUrlExpiry(expiryTimeSeconds);
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
        
        MozLoopService.hawkRequest(sessionType, path, method, payloadObj).then((response) => {
          callback(null, response.body);
        }, hawkError => {
          
          
          
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
          let defaults = Services.prefs.getDefaultBranch(null);

          
          
          try {
            appVersionInfo = Cu.cloneInto({
              channel: defaults.getCharPref("app.update.channel"),
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

    





    telemetryAdd: {
      enumerable: true,
      writable: true,
      value: function(histogramId, value) {
        Services.telemetry.getHistogramById(histogramId).add(value);
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
    }
  };

  function onStatusChanged(aSubject, aTopic, aData) {
    let event = new targetWindow.CustomEvent("LoopStatusChanged");
    targetWindow.dispatchEvent(event);
  };

  function onDOMWindowDestroyed(aSubject, aTopic, aData) {
    if (targetWindow && aSubject != targetWindow)
      return;
    Services.obs.removeObserver(onDOMWindowDestroyed, "dom-window-destroyed");
    Services.obs.removeObserver(onStatusChanged, "loop-status-changed");
  };

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
