



"use strict";

this.EXPORTED_SYMBOLS = ["DirectoryLinksProvider"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const XMLHttpRequest =
  Components.Constructor("@mozilla.org/xmlextras/xmlhttprequest;1", "nsIXMLHttpRequest");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyGetter(this, "gTextDecoder", () => {
  return new TextDecoder();
});


const DIRECTORY_LINKS_FILE = "directoryLinks.json";


const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";


const PREF_SELECTED_LOCALE = "general.useragent.locale";


const PREF_DIRECTORY_SOURCE = "browser.newtabpage.directory.source";


const DIRECTORY_FRECENCY = 1000;

const LINK_TYPES = Object.freeze([
  "sponsored",
  "affiliate",
  "organic",
]);






let DirectoryLinksProvider = {

  __linksURL: null,

  _observers: new Set(),

  
  _downloadDeferred: null,

  
  _downloadIntervalMS: 86400000,

  get _observedPrefs() Object.freeze({
    linksURL: PREF_DIRECTORY_SOURCE,
    matchOSLocale: PREF_MATCH_OS_LOCALE,
    prefSelectedLocale: PREF_SELECTED_LOCALE,
  }),

  get _linksURL() {
    if (!this.__linksURL) {
      try {
        this.__linksURL = Services.prefs.getCharPref(this._observedPrefs["linksURL"]);
      }
      catch (e) {
        Cu.reportError("Error fetching directory links url from prefs: " + e);
      }
    }
    return this.__linksURL;
  },

  



  get locale() {
    let matchOS;
    try {
      matchOS = Services.prefs.getBoolPref(PREF_MATCH_OS_LOCALE);
    }
    catch (e) {}

    if (matchOS) {
      return Services.locale.getLocaleComponentForUserAgent();
    }

    try {
      let locale = Services.prefs.getComplexValue(PREF_SELECTED_LOCALE,
                                                  Ci.nsIPrefLocalizedString);
      if (locale) {
        return locale.data;
      }
    }
    catch (e) {}

    try {
      return Services.prefs.getCharPref(PREF_SELECTED_LOCALE);
    }
    catch (e) {}

    return "en-US";
  },

  get linkTypes() LINK_TYPES,

  observe: function DirectoryLinksProvider_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed") {
      if (aData == this._observedPrefs["linksURL"]) {
        delete this.__linksURL;
      }
    }
    
    this._fetchAndCacheLinksIfNecessary(true);
  },

  _addPrefsObserver: function DirectoryLinksProvider_addObserver() {
    for (let pref in this._observedPrefs) {
      let prefName = this._observedPrefs[pref];
      Services.prefs.addObserver(prefName, this, false);
    }
  },

  _removePrefsObserver: function DirectoryLinksProvider_removeObserver() {
    for (let pref in this._observedPrefs) {
      let prefName = this._observedPrefs[pref];
      Services.prefs.removeObserver(prefName, this);
    }
  },

  _fetchAndCacheLinks: function DirectoryLinksProvider_fetchAndCacheLinks(uri) {
    let deferred = Promise.defer();
    let xmlHttp = new XMLHttpRequest();
    xmlHttp.overrideMimeType("application/json");

    let self = this;
    xmlHttp.onload = function(aResponse) {
      let json = this.responseText;
      if (this.status && this.status != 200) {
        json = "{}";
      }
      OS.File.writeAtomic(self._directoryFilePath, json, {tmpPath: self._directoryFilePath + ".tmp"})
        .then(() => {
          deferred.resolve();
        },
        () => {
          deferred.reject("Error writing uri data in profD.");
        });
    };

    xmlHttp.onerror = function(e) {
      deferred.reject("Fetching " + uri + " results in error code: " + e.target.status);
    };

    try {
      xmlHttp.open('POST', uri);
      xmlHttp.send(JSON.stringify({
        locale: this.locale,
        directoryCount: this._directoryCount,
      }));
    } catch (e) {
      deferred.reject("Error fetching " + uri);
      Cu.reportError(e);
    }
    return deferred.promise;
  },

  



  _fetchAndCacheLinksIfNecessary: function DirectoryLinksProvider_fetchAndCacheLinksIfNecessary(forceDownload=false) {
    if (this._downloadDeferred) {
      
      return this._downloadDeferred.promise;
    }

    if (forceDownload || this._needsDownload) {
      this._downloadDeferred = Promise.defer();
      this._fetchAndCacheLinks(this._linksURL).then(() => {
        
        this._lastDownloadMS = Date.now();
        this._downloadDeferred.resolve();
        this._downloadDeferred = null;
        this._callObservers("onManyLinksChanged")
      },
      error => {
        this._downloadDeferred.resolve();
        this._downloadDeferred = null;
        this._callObservers("onDownloadFail");
      });
      return this._downloadDeferred.promise;
    }

    
    return Promise.resolve();
  },

  


  get _needsDownload () {
    
    if ((Date.now() - this._lastDownloadMS) > this._downloadIntervalMS) {
      return true;
    }
    return false;
  },

  



  _readDirectoryLinksFile: function DirectoryLinksProvider_readDirectoryLinksFile() {
    return OS.File.read(this._directoryFilePath).then(binaryData => {
      let output;
      try {
        let locale = this.locale;
        let json = gTextDecoder.decode(binaryData);
        output = JSON.parse(json)[locale];
      }
      catch (e) {
        Cu.reportError(e);
      }
      return output || [];
    },
    error => {
      Cu.reportError(error);
      return [];
    });
  },

  






  reportShownCount: function DirectoryLinksProvider_reportShownCount(directoryCount) {
    
    this._directoryCount = Cu.cloneInto(directoryCount, {});
    if (directoryCount.sponsored > 0
        || directoryCount.affiliate > 0
        || directoryCount.organic > 0) {
      return this._fetchAndCacheLinksIfNecessary();
    }
    return Promise.resolve();
  },

  



  getLinks: function DirectoryLinksProvider_getLinks(aCallback) {
    this._readDirectoryLinksFile().then(rawLinks => {
      
      aCallback(rawLinks.map((link, position) => {
        link.directoryIndex = position;
        link.frecency = DIRECTORY_FRECENCY;
        link.lastVisitDate = rawLinks.length - position;
        return link;
      }));
    });
  },

  init: function DirectoryLinksProvider_init() {
    this._addPrefsObserver();
    
    this._directoryFilePath = OS.Path.join(OS.Constants.Path.localProfileDir, DIRECTORY_LINKS_FILE);
    this._lastDownloadMS = 0;
    return Task.spawn(function() {
      
      let doesFileExists = yield OS.File.exists(this._directoryFilePath);
      if (doesFileExists) {
        let fileInfo = yield OS.File.stat(this._directoryFilePath);
        this._lastDownloadMS = Date.parse(fileInfo.lastModificationDate);
      }
      
      yield this._fetchAndCacheLinksIfNecessary();
    }.bind(this));
  },

  


  reset: function DirectoryLinksProvider_reset() {
    delete this.__linksURL;
    delete this._directoryCount;
    this._removePrefsObserver();
    this._removeObservers();
  },

  addObserver: function DirectoryLinksProvider_addObserver(aObserver) {
    this._observers.add(aObserver);
  },

  removeObserver: function DirectoryLinksProvider_removeObserver(aObserver) {
    this._observers.delete(aObserver);
  },

  _callObservers: function DirectoryLinksProvider__callObservers(aMethodName, aArg) {
    for (let obs of this._observers) {
      if (typeof(obs[aMethodName]) == "function") {
        try {
          obs[aMethodName](this, aArg);
        } catch (err) {
          Cu.reportError(err);
        }
      }
    }
  },

  _removeObservers: function() {
    this._observers.clear();
  }
};
