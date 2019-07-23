



































var EXPORTED_SYMBOLS = [ "gDownloadLastDir" ];

let observer = {
  QueryInterface: function (aIID) {
    if (aIID.equals(Components.interfaces.nsIObserver) ||
        aIID.equals(Components.interfaces.nsISupports) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },
  observe: function (aSubject, aTopic, aData) {
    gDownloadLastDirFile = null;
  }
};

Components.classes["@mozilla.org/observer-service;1"]
          .getService(Components.interfaces.nsIObserverService)
          .addObserver(observer, "private-browsing", true);

let gDownloadLastDirFile = null;
let gDownloadLastDir = {
  get file() {
    if (gDownloadLastDirFile && !gDownloadLastDirFile.exists())
      gDownloadLastDirFile = null;

    return gDownloadLastDirFile;
  },
  set file(val) {
    gDownloadLastDirFile = val;
  }
};
