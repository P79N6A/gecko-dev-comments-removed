



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function IDService() {
  this.wrappedJSObject = this;
}
IDService.prototype = {
  classID: Components.ID("{baa581e5-8e72-406c-8c9f-dcd4b23a6f82}"),

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
