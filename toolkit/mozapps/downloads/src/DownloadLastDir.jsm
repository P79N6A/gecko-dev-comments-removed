



































const LAST_DIR_PREF = "browser.download.lastDir";
const PBSVC_CID = "@mozilla.org/privatebrowsing;1";
const nsILocalFile = Components.interfaces.nsILocalFile;

var EXPORTED_SYMBOLS = [ "gDownloadLastDir" ];

let pbSvc = null;
if (PBSVC_CID in Components.classes) {
  pbSvc = Components.classes[PBSVC_CID]
                    .getService(Components.interfaces.nsIPrivateBrowsingService);
}
let prefSvc = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch);

let observer = {
  QueryInterface: function (aIID) {
    if (aIID.equals(Components.interfaces.nsIObserver) ||
        aIID.equals(Components.interfaces.nsISupports) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "private-browsing":
        if (aData == "enter")
          gDownloadLastDirFile = readLastDirPref();
        else if (aData == "exit")
          gDownloadLastDirFile = null;
        break;
      case "browser:purge-session-history":
        gDownloadLastDirFile = null;
        if (prefSvc.prefHasUserValue(LAST_DIR_PREF))
          prefSvc.clearUserPref(LAST_DIR_PREF);
        break;
    }
  }
};

let os = Components.classes["@mozilla.org/observer-service;1"]
                   .getService(Components.interfaces.nsIObserverService);
os.addObserver(observer, "private-browsing", true);
os.addObserver(observer, "browser:purge-session-history", true);

function readLastDirPref() {
  try {
    return prefSvc.getComplexValue(LAST_DIR_PREF, nsILocalFile);
  }
  catch (e) {
    return null;
  }
}

let gDownloadLastDirFile = readLastDirPref();
let gDownloadLastDir = {
  get file() {
    if (gDownloadLastDirFile && !gDownloadLastDirFile.exists())
      gDownloadLastDirFile = null;

    if (pbSvc && pbSvc.privateBrowsingEnabled)
      return gDownloadLastDirFile;
    else
      return readLastDirPref();
  },
  set file(val) {
    if (pbSvc && pbSvc.privateBrowsingEnabled) {
      if (val instanceof Components.interfaces.nsIFile)
        gDownloadLastDirFile = val.clone();
      else
        gDownloadLastDirFile = null;
    } else {
      if (val instanceof Components.interfaces.nsIFile)
        prefSvc.setComplexValue(LAST_DIR_PREF, nsILocalFile, val);
      else if (prefSvc.prefHasUserValue(LAST_DIR_PREF))
        prefSvc.clearUserPref(LAST_DIR_PREF);
    }
  }
};
