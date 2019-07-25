



































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
      os.addObserver(this, "sessionstore-windows-restored", true);
      break;
   


    case "sessionstore-windows-restored":
      Cu.import("resource://weave/service.js");
      Weave.Service.onStartup();
      break;
    }
  }
};

function AboutWeaveService() {}
AboutWeaveService.prototype = {
  classDescription: "about:weave",
  contractID: "@mozilla.org/network/protocol/about;1?what=weave",
  classID: Components.ID("{ecb6987d-9d71-475d-a44d-a5ff2099b08c}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule,
                                         Ci.nsISupportsWeakReference]),

  getURIFlags: function(aURI) {
    return (Ci.nsIAboutModule.ALLOW_SCRIPT |
            Ci.nsIAboutModule.URI_SAFE_FOR_UNTRUSTED_CONTENT);
  },

  newChannel: function(aURI) {
    let ios = Cc["@mozilla.org/network/io-service;1"]
      .getService(Ci.nsIIOService);
    let ch = ios.newChannel("chrome://weave/content/about/index.html", null, null);
    ch.originalURI = aURI;
    return ch;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([WeaveService, AboutWeaveService]);
}

