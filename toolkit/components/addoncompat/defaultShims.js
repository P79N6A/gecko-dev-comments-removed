



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");







function DefaultInterpositionService() {
}

DefaultInterpositionService.prototype = {
  classID: Components.ID("{50bc93ce-602a-4bef-bf3a-61fc749c4caf}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAddonInterposition, Ci.nsISupportsWeakReference]),

  interpose: function(addon, target, iid, prop) {
    return null;
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DefaultInterpositionService]);
