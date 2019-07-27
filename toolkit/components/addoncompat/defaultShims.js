



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");







function DefaultInterpositionService() {
}

DefaultInterpositionService.prototype = {
  classID: Components.ID("{50bc93ce-602a-4bef-bf3a-61fc749c4caf}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAddonInterposition, Ci.nsISupportsWeakReference]),

  getWhitelist: function() {
    return [];
  },

  interposeProperty: function(addon, target, iid, prop) {
    return null;
  },

  interposeCall: function(addonId, originalFunc, originalThis, args) {
    args.splice(0, 0, addonId);
    return originalFunc.apply(originalThis, args);
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DefaultInterpositionService]);
