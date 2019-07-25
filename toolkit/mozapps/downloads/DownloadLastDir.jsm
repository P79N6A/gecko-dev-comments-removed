





















































const LAST_DIR_PREF = "browser.download.lastDir";
const PBSVC_CID = "@mozilla.org/privatebrowsing;1";
const nsILocalFile = Components.interfaces.nsILocalFile;

var EXPORTED_SYMBOLS = [ "gDownloadLastDir" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Dict.jsm");

let pbSvc = null;
if (PBSVC_CID in Components.classes) {
  pbSvc = Components.classes[PBSVC_CID]
                    .getService(Components.interfaces.nsIPrivateBrowsingService);
}

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
        else if (aData == "exit") {
          gDownloadLastDirFile = null;
          gDownloadLastDirStore = new Dict();
        }
        break;
      case "browser:purge-session-history":
        gDownloadLastDirFile = null;
        if (Services.prefs.prefHasUserValue(LAST_DIR_PREF))
          Services.prefs.clearUserPref(LAST_DIR_PREF);
        gDownloadLastDirStore = new Dict();
        Services.contentPrefs.removePrefsByName(LAST_DIR_PREF);
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
    return Services.prefs.getComplexValue(LAST_DIR_PREF, nsILocalFile);
  }
  catch (e) {
    return null;
  }
}

let gDownloadLastDirFile = readLastDirPref();
let gDownloadLastDirStore = new Dict();
let gDownloadLastDir = {
  
  get file() { return this.getFile(); },
  set file(val) { this.setFile(null, val); },
  getFile: function (aURI) {
    if (aURI) {
      let lastDir;
      if (pbSvc && pbSvc.privateBrowsingEnabled) {
        let group = Services.contentPrefs.grouper.group(aURI);
        lastDir = gDownloadLastDirStore.get(group, null);
      }
      if (!lastDir) {
        lastDir = Services.contentPrefs.getPref(aURI, LAST_DIR_PREF);
      }
      if (lastDir) {
        var lastDirFile = Components.classes["@mozilla.org/file/local;1"]
                                    .createInstance(Components.interfaces.nsILocalFile);
        lastDirFile.initWithPath(lastDir);
        return lastDirFile;
      }
    }
    if (gDownloadLastDirFile && !gDownloadLastDirFile.exists())
      gDownloadLastDirFile = null;

    if (pbSvc && pbSvc.privateBrowsingEnabled)
      return gDownloadLastDirFile;
    else
      return readLastDirPref();
  },
  setFile: function (aURI, aFile) {
    if (aURI) {
      if (pbSvc && pbSvc.privateBrowsingEnabled) {
        let group = Services.contentPrefs.grouper.group(aURI);
        gDownloadLastDirStore.set(group, aFile.path);
      } else {
        Services.contentPrefs.setPref(aURI, LAST_DIR_PREF, aFile.path);
      }
    }
    if (pbSvc && pbSvc.privateBrowsingEnabled) {
      if (aFile instanceof Components.interfaces.nsIFile)
        gDownloadLastDirFile = aFile.clone();
      else
        gDownloadLastDirFile = null;
    } else {
      if (aFile instanceof Components.interfaces.nsIFile)
        Services.prefs.setComplexValue(LAST_DIR_PREF, nsILocalFile, aFile);
      else if (Services.prefs.prefHasUserValue(LAST_DIR_PREF))
        Services.prefs.clearUserPref(LAST_DIR_PREF);
    }
  }
};
