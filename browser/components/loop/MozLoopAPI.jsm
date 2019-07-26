



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/loop/MozLoopService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "hookWindowCloseForPanelClose",
  "resource://gre/modules/MozSocialAPI.jsm");

this.EXPORTED_SYMBOLS = ["injectLoopAPI"];









function injectLoopAPI(targetWindow) {
  let api = {
    


    doNotDisturb: {
      enumerable: true,
      configurable: true,
      get: function() {
        return MozLoopService.doNotDisturb;
      },
      set: function(aFlag) {
        MozLoopService.doNotDisturb = aFlag;
      }
    },

    




    serverUrl: {
      enumerable: true,
      configurable: true,
      get: function() {
        return Services.prefs.getCharPref("loop.server");
      }
    },

    




    locale: {
      enumerable: true,
      configurable: true,
      get: function() {
        return MozLoopService.locale;
      }
    },

    







    getStrings: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function(key) {
        return MozLoopService.getStrings(key);
      }
    },

    










    ensureRegistered: {
      enumerable: true,
      configurable: true,
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
      configurable: true,
      writable: true,
      value: function(expiryTimeSeconds) {
        MozLoopService.noteCallUrlExpiry(expiryTimeSeconds);
      }
    },

    












    getLoopCharPref: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function(prefName) {
        return MozLoopService.getLoopCharPref(prefName);
      }
    }
  };

  let contentObj = Cu.createObjectIn(targetWindow);
  Object.defineProperties(contentObj, api);
  Cu.makeObjectPropsNormal(contentObj);

  targetWindow.navigator.wrappedJSObject.__defineGetter__("mozLoop", function() {
    
    
    
    
    delete targetWindow.navigator.wrappedJSObject.mozLoop;
    return targetWindow.navigator.wrappedJSObject.mozLoop = contentObj;
  });

  
  hookWindowCloseForPanelClose(targetWindow);
}
