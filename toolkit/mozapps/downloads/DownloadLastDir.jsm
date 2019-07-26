

























const LAST_DIR_PREF = "browser.download.lastDir";
const SAVE_PER_SITE_PREF = LAST_DIR_PREF + ".savePerSite";
const nsIFile = Components.interfaces.nsIFile;

this.EXPORTED_SYMBOLS = [ "DownloadLastDir" ];

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

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
      case "last-pb-context-exited":
        gDownloadLastDirFile = null;
        break;
      case "browser:purge-session-history":
        gDownloadLastDirFile = null;
        if (Services.prefs.prefHasUserValue(LAST_DIR_PREF))
          Services.prefs.clearUserPref(LAST_DIR_PREF);
        
        
        let cps2 = Components.classes["@mozilla.org/content-pref/service;1"].
                     getService(Components.interfaces.nsIContentPrefService2);

        cps2.removeByName(LAST_DIR_PREF, {usePrivateBrowsing: false});
        cps2.removeByName(LAST_DIR_PREF, {usePrivateBrowsing: true});
        break;
    }
  }
};

let os = Components.classes["@mozilla.org/observer-service;1"]
                   .getService(Components.interfaces.nsIObserverService);
os.addObserver(observer, "last-pb-context-exited", true);
os.addObserver(observer, "browser:purge-session-history", true);

function readLastDirPref() {
  try {
    return Services.prefs.getComplexValue(LAST_DIR_PREF, nsIFile);
  }
  catch (e) {
    return null;
  }
}

function isContentPrefEnabled() {
  try {
    return Services.prefs.getBoolPref(SAVE_PER_SITE_PREF);
  }
  catch (e) {
    return true;
  }
}

let gDownloadLastDirFile = readLastDirPref();

this.DownloadLastDir = function DownloadLastDir(aWindow) {
  this.window = aWindow;
}

DownloadLastDir.prototype = {
  isPrivate: function DownloadLastDir_isPrivate() {
    return PrivateBrowsingUtils.isWindowPrivate(this.window);
  },
  
  get file() this._getLastFile(),
  set file(val) { this.setFile(null, val); },
  cleanupPrivateFile: function () {
    gDownloadLastDirFile = null;
  },
  
  
  getFile: function (aURI) {
    let Deprecated = Components.utils.import("resource://gre/modules/Deprecated.jsm", {}).Deprecated;
    Deprecated.warning("DownloadLastDir.getFile is deprecated. Please use getFileAsync instead.",
                       "https://developer.mozilla.org/en-US/docs/Mozilla/JavaScript_code_modules/DownloadLastDir.jsm",
                       Components.stack.caller);

    if (aURI && isContentPrefEnabled()) {
      let loadContext = this.window
                            .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                            .getInterface(Components.interfaces.nsIWebNavigation)
                            .QueryInterface(Components.interfaces.nsILoadContext);
      let lastDir = Services.contentPrefs.getPref(aURI, LAST_DIR_PREF, loadContext);
      if (lastDir) {
        var lastDirFile = Components.classes["@mozilla.org/file/local;1"]
                                    .createInstance(Components.interfaces.nsIFile);
        lastDirFile.initWithPath(lastDir);
        return lastDirFile;
      }
    }
    return this._getLastFile();
  },

  _getLastFile: function () {
    if (gDownloadLastDirFile && !gDownloadLastDirFile.exists())
      gDownloadLastDirFile = null;

    if (this.isPrivate()) {
      if (!gDownloadLastDirFile)
        gDownloadLastDirFile = readLastDirPref();
      return gDownloadLastDirFile;
    }
    return readLastDirPref();
  },

  getFileAsync: function(aURI, aCallback) {
    let plainPrefFile = this._getLastFile();
    if (!aURI || !isContentPrefEnabled()) {
      Services.tm.mainThread.dispatch(function() aCallback(plainPrefFile),
                                      Components.interfaces.nsIThread.DISPATCH_NORMAL);
      return;
    }

    let uri = aURI instanceof Components.interfaces.nsIURI ? aURI.spec : aURI;
    let cps2 = Components.classes["@mozilla.org/content-pref/service;1"]
                         .getService(Components.interfaces.nsIContentPrefService2);
    let loadContext = this.window
                          .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                          .getInterface(Components.interfaces.nsIWebNavigation)
                          .QueryInterface(Components.interfaces.nsILoadContext);
    let result = null;
    cps2.getByDomainAndName(uri, LAST_DIR_PREF, loadContext, {
      handleResult: function(aResult) result = aResult,
      handleCompletion: function(aReason) {
        let file = plainPrefFile;
        if (aReason == Components.interfaces.nsIContentPrefCallback2.COMPLETE_OK &&
           result instanceof Components.interfaces.nsIContentPref) {
          file = Components.classes["@mozilla.org/file/local;1"]
                           .createInstance(Components.interfaces.nsIFile);
          file.initWithPath(result.value);
        }
        aCallback(file);
      }
    });
  },

  setFile: function (aURI, aFile) {
    if (aURI && isContentPrefEnabled()) {
      let uri = aURI instanceof Components.interfaces.nsIURI ? aURI.spec : aURI;
      let cps2 = Components.classes["@mozilla.org/content-pref/service;1"]
                           .getService(Components.interfaces.nsIContentPrefService2);
      let loadContext = this.window
                            .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                            .getInterface(Components.interfaces.nsIWebNavigation)
                            .QueryInterface(Components.interfaces.nsILoadContext);
      if (aFile instanceof Components.interfaces.nsIFile)
        cps2.set(uri, LAST_DIR_PREF, aFile.path, loadContext);
      else
        cps2.removeByDomainAndName(uri, LAST_DIR_PREF, loadContext);
    }
    if (this.isPrivate()) {
      if (aFile instanceof Components.interfaces.nsIFile)
        gDownloadLastDirFile = aFile.clone();
      else
        gDownloadLastDirFile = null;
    } else {
      if (aFile instanceof Components.interfaces.nsIFile)
        Services.prefs.setComplexValue(LAST_DIR_PREF, nsIFile, aFile);
      else if (Services.prefs.prefHasUserValue(LAST_DIR_PREF))
        Services.prefs.clearUserPref(LAST_DIR_PREF);
    }
  }
};
