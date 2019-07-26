



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.IDService = function IDService() {
  this.wrappedJSObject = this;
};

this.IDService.prototype = {
  classID: Components.ID("{4e0a0e98-b1d3-4745-a1eb-f815199dd06b}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "app-startup":
        Services.obs.addObserver(this, "final-ui-startup", true);
        break;
      case "final-ui-startup":
        
        Cu.import("resource://gre/modules/DOMIdentity.jsm");
        DOMIdentity._init();
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([IDService]);
