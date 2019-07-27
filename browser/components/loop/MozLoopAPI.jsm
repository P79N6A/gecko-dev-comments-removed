



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/loop/MozLoopService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "hookWindowCloseForPanelClose",
                                        "resource://gre/modules/MozSocialAPI.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                         "@mozilla.org/widget/clipboardhelper;1",
                                         "nsIClipboardHelper");
this.EXPORTED_SYMBOLS = ["injectLoopAPI"];









function injectLoopAPI(targetWindow) {
  let ringer;
  let ringerStopper;

  let api = {
    


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

    







    getStrings: {
      enumerable: true,
      writable: true,
      value: function(key) {
        return MozLoopService.getStrings(key);
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
        
        return MozLoopService.hawkRequest(path, method, payloadObj).then((response) => {
          callback(null, response.body);
        }, (error) => {
          callback(Cu.cloneInto(error, targetWindow));
        });
      }
    },

    




    copyString: {
      enumerable: true,
      writable: true,
      value: function(str) {
        clipboardHelper.copyString(str);
      }
    }
  };

  let contentObj = Cu.createObjectIn(targetWindow);
  Object.defineProperties(contentObj, api);
  Object.seal(contentObj);
  Cu.makeObjectPropsNormal(contentObj);

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
