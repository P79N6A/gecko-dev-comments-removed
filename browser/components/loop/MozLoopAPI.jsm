



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/loop/MozLoopService.jsm");
Cu.import("resource:///modules/loop/LoopContacts.jsm");

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
this.EXPORTED_SYMBOLS = ["injectLoopAPI"];









const cloneErrorObject = function(error, targetWindow) {
  let obj = new targetWindow.Error();
  for (let prop of Object.getOwnPropertyNames(error)) {
    obj[prop] = String(error[prop]);
  }
  return obj;
};








const injectObjectAPI = function(api, targetWindow) {
  let injectedAPI = {};
  
  
  Object.keys(api).forEach(func => {
    injectedAPI[func] = function(...params) {
      let callback = params.pop();
      api[func](...params, function(...results) {
        results = results.map(result => {
          if (result && typeof result == "object") {
            
            if (result.constructor.name == "Error") {
              return cloneErrorObject(result.message)
            }
            return Cu.cloneInto(result, targetWindow);
          }
          return result;
        });
        callback(...results);
      });
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

    




    locale: {
      enumerable: true,
      get: function() {
        return MozLoopService.locale;
      }
    },

    








    getCallData: {
      enumerable: true,
      writable: true,
      value: function(loopCallId) {
        return Cu.cloneInto(MozLoopService.getCallData(loopCallId), targetWindow);
      }
    },

    




    contacts: {
      enumerable: true,
      get: function() {
        if (contactsAPI) {
          return contactsAPI;
        }
        return contactsAPI = injectObjectAPI(LoopContacts, targetWindow);
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

    










    ensureRegistered: {
      enumerable: true,
      writable: true,
      value: function(callback) {
        
        
        return MozLoopService.register().then(() => {
          callback(null);
        }, err => {
          callback(err);
        });
      }
    },

    











    noteCallUrlExpiry: {
      enumerable: true,
      writable: true,
      value: function(expiryTimeSeconds) {
        MozLoopService.noteCallUrlExpiry(expiryTimeSeconds);
      }
    },

    








    setLoopCharPref: {
      enumerable: true,
      writable: true,
      value: function(prefName, value) {
        MozLoopService.setLoopCharPref(prefName, value);
      }
    },

    












    getLoopCharPref: {
      enumerable: true,
      writable: true,
      value: function(prefName) {
        return MozLoopService.getLoopCharPref(prefName);
      }
    },

    












    getLoopBoolPref: {
      enumerable: true,
      writable: true,
      value: function(prefName) {
        return MozLoopService.getLoopBoolPref(prefName);
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
      value: function(path, method, payloadObj, callback) {
        
        
        return MozLoopService.hawkRequest(LOOP_SESSION_TYPE.GUEST, path, method, payloadObj).then((response) => {
          callback(null, response.body);
        }, (error) => {
          callback(Cu.cloneInto(error, targetWindow));
        });
      }
    },

    LOOP_SESSION_TYPE: {
      enumerable: true,
      writable: false,
      value: function() {
        return LOOP_SESSION_TYPE;
      },
    },

    logInToFxA: {
      enumerable: true,
      writable: true,
      value: function() {
        return MozLoopService.logInToFxA();
      }
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

          appVersionInfo = Cu.cloneInto({
            channel: defaults.getCharPref("app.update.channel"),
            version: appInfo.version,
            OS: appInfo.OS
          }, targetWindow);
        }
        return appVersionInfo;
      }
    },
  };

  function onStatusChanged(aSubject, aTopic, aData) {
    let event = new targetWindow.CustomEvent("LoopStatusChanged");
    targetWindow.dispatchEvent(event)
  };

  function onDOMWindowDestroyed(aSubject, aTopic, aData) {
    if (aSubject != targetWindow)
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

  targetWindow.navigator.wrappedJSObject.__defineGetter__("mozLoop", function() {
    
    
    
    
    delete targetWindow.navigator.wrappedJSObject.mozLoop;
    return targetWindow.navigator.wrappedJSObject.mozLoop = contentObj;
  });

  
  hookWindowCloseForPanelClose(targetWindow);
}

function getChromeWindow(contentWin) {
  return contentWin.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShellTreeItem)
                   .rootTreeItem
                   .QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindow);
}
