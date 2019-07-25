



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function AitcService() {
  this.wrappedJSObject = this;
}
AitcService.prototype = {
  classID: Components.ID("{a3d387ca-fd26-44ca-93be-adb5fda5a78d}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "app-startup":
        let os = Cc["@mozilla.org/observer-service;1"]
                   .getService(Ci.nsIObserverService);
        os.addObserver(this, "final-ui-startup", true);
        break;
      case "final-ui-startup":
        
        Cu.import("resource://services-common/preferences.js");
        if (Preferences.get("services.sync.engine.apps", false)) {
          return;
        }
        if (!Preferences.get("services.aitc.enabled", true)) {
          return;
        }

        Cu.import("resource://services-common/utils.js");
        CommonUtils.namedTimer(function() {
          
        }, 2000, this, "timer");
        break;
    }
  }
};

const components = [AitcService];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
