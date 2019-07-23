



































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
    gDownloadLastDirPath = null;
  }
};

Components.classes["@mozilla.org/observer-service;1"]
          .getService(Components.interfaces.nsIObserverService)
          .addObserver(observer, "private-browsing", true);

let gDownloadLastDirPath = null;
let gDownloadLastDir = {
  get path() {
    if (gDownloadLastDirPath && !gDownloadLastDirPath.exists())
      gDownloadLastDirPath = null;

    return gDownloadLastDirPath;
  },
  set path(val) {
    gDownloadLastDirPath = val;
  }
};
