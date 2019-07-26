



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
    




    loopServer: {
      enumerable: true,
      configurable: true,
      writable: false,
      value: function() {
        return Services.prefs.getCharPref("loop.server");
      }
    },

    




    getLocale: {
      enumerable: true,
      configurable: true,
      writable: false,
      value: function() {
        return MozLoopService.locale;
      }
    },

    







    getStrings: {
      enumerable: true,
      configurable: true,
      writable: false,
      value: function(key) {
        return MozLoopService.getStrings(key);
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
