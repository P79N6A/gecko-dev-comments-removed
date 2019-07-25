



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function WeaveService() {
  this.wrappedJSObject = this;
}
WeaveService.prototype = {
  classID: Components.ID("{74b89fb0-f200-4ae8-a3ec-dd164117f6de}"),

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
          Cu.import("resource://services-sync/main.js");
          if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED)
            Weave.Service;
        }
      }, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
      break;
    }
  }
};

function AboutWeaveLog() {}
AboutWeaveLog.prototype = {
  classID: Components.ID("{d28f8a0b-95da-48f4-b712-caf37097be41}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule,
                                         Ci.nsISupportsWeakReference]),

  getURIFlags: function(aURI) {
    return 0;
  },

  newChannel: function(aURI) {
    let dir = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties);
    let file = dir.get("ProfD", Ci.nsILocalFile);
    file.append("weave");
    file.append("logs");
    file.append("verbose-log.txt");
    let ios = Cc["@mozilla.org/network/io-service;1"].
      getService(Ci.nsIIOService);
    let ch = ios.newChannel(ios.newFileURI(file).spec, null, null);
    ch.originalURI = aURI;
    return ch;
  }
};

function AboutWeaveLog1() {}
AboutWeaveLog1.prototype = {
  classID: Components.ID("{a08ee179-df50-48e0-9c87-79e4dd5caeb1}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule,
                                         Ci.nsISupportsWeakReference]),

  getURIFlags: function(aURI) {
    return 0;
  },

  newChannel: function(aURI) {
    let dir = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties);
    let file = dir.get("ProfD", Ci.nsILocalFile);
    file.append("weave");
    file.append("logs");
    file.append("verbose-log.txt.1");
    let ios = Cc["@mozilla.org/network/io-service;1"].
      getService(Ci.nsIIOService);
    let ch = ios.newChannel(ios.newFileURI(file).spec, null, null);
    ch.originalURI = aURI;
    return ch;
  }
};

let components = [WeaveService, AboutWeaveLog, AboutWeaveLog1];


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(components);
}


if (typeof XPCOMUtils.generateNSGetFactory == "function")
    const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
