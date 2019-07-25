



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function WeaveService() {}
WeaveService.prototype = {
  classDescription: "Weave Service",
  contractID: "@mozilla.org/weave/service;1",
  classID: Components.ID("{74b89fb0-f200-4ae8-a3ec-dd164117f6de}"),
  _xpcom_categories: [{ category: "app-startup", service: true }],

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function BSS__observe(subject, topic, data) {
    switch (topic) {
    case "app-startup":
      let os = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
      os.addObserver(this, "final-ui-startup", true);
      break;

    case "final-ui-startup":
      
      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this.timer.initWithCallback({
        notify: function() {
          Cu.import("resource://weave/service.js");
        }
      }, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
      break;
    }
  }
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([WeaveService]);
}

