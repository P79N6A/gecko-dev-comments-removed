



































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

function AboutWeaveTabs() {}
AboutWeaveTabs.prototype = {
  classDescription: "about:weave-tabs",
  contractID: "@mozilla.org/network/protocol/about;1?what=weave-tabs",
  classID: Components.ID("{ecb6987d-9d71-475d-a44d-a5ff2099b08c}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule,
                                         Ci.nsISupportsWeakReference]),

  getURIFlags: function(aURI) {
    return (Ci.nsIAboutModule.ALLOW_SCRIPT);
  },

  newChannel: function(aURI) {
    let ios = Cc["@mozilla.org/network/io-service;1"]
      .getService(Ci.nsIIOService);
    let ch = ios.newChannel("chrome://weave/content/fx-tabs.xul", null, null);
    ch.originalURI = aURI;
    return ch;
  }
};
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([WeaveService, AboutWeaveTabs]);
}

