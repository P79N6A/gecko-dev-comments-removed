



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function WeaveService() {}
WeaveService.prototype = {
  classDescription: "Weave Service",
  contractID: "@mozilla.org/weave/service;1",
  classID: Components.ID("{74b89fb0-f200-4ae8-a3ec-dd164117f6de}"),
  _xpcom_categories: [{ category: "app-startup", service: true }],

  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIObserver,
                                         Components.interfaces.nsISupportsWeakReference]),

  observe: function BSS__observe(subject, topic, data) {
    switch (topic) {
    case "app-startup":
      let os = Components.classes["@mozilla.org/observer-service;1"].
        getService(Components.interfaces.nsIObserverService);
      os.addObserver(this, "profile-after-change", true);
      break;
   


    case "sessionstore-windows-restored":
      Components.utils.import("resource://weave/service.js");
      Weave.Service.onStartup();
      break;
    }
  }
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([WeaveService]);
}

