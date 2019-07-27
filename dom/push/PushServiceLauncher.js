



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function PushServiceLauncher() {
};

PushServiceLauncher.prototype = {
  classID: Components.ID("{4b8caa3b-3c58-4f3c-a7f5-7bd9cb24c11d}"),

  contractID: "@mozilla.org/push/ServiceLauncher;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "app-startup":
        Services.obs.addObserver(this, "final-ui-startup", true);
        break;
      case "final-ui-startup":
        Services.obs.removeObserver(this, "final-ui-startup");

        let isParent = Cc["@mozilla.org/xre/runtime;1"]
                       .getService(Ci.nsIXULRuntime)
                       .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

        if (isParent) {
          Cu.import("resource://gre/modules/PushService.jsm");
          PushService.init();
        }
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PushServiceLauncher]);
