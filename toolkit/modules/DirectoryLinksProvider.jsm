



"use strict";

this.EXPORTED_SYMBOLS = ["DirectoryLinksProvider"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const XMLHttpRequest =
  Components.Constructor("@mozilla.org/xmlextras/xmlhttprequest;1", "nsIXMLHttpRequest");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");


const DIRECTORY_LINKS_FILE = "directoryLinks.json";


const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";


const PREF_SELECTED_LOCALE = "general.useragent.locale";


const PREF_DIRECTORY_SOURCE = "browser.newtabpage.directorySource";


const DIRECTORY_FRECENCY = 1000;

const LINK_TYPES = Object.freeze([
  "sponsored",
  "affiliate",
  "organic",
]);






let DirectoryLinksProvider = {

  __linksURL: null,

  _observers: [],

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
      this._callObservers("onManyLinksChanged");
    }
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

  



  _fetchLinks: function DirectoryLinksProvider_fetchLinks(aCallback) {
    try {
      NetUtil.asyncFetch(this._linksURL, (aInputStream, aResult, aRequest) => {
        let output;
        if (Components.isSuccessCode(aResult)) {
          try {
            let json = NetUtil.readInputStreamToString(aInputStream,
                                                       aInputStream.available(),
                                                       {charset: "UTF-8"});
            let locale = this.locale;
            output = JSON.parse(json)[locale];
          }
          catch (e) {
            Cu.reportError(e);
          }
        }
        else {
          Cu.reportError(new Error("the fetch of " + this._linksURL + "was unsuccessful"));
        }
        aCallback(output || []);
      });
    }
    catch (e) {
      Cu.reportError(e);
      aCallback([]);
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
      let directoryLinksFilePath = OS.Path.join(OS.Constants.Path.localProfileDir, DIRECTORY_LINKS_FILE);
      OS.File.writeAtomic(directoryLinksFilePath, json, {tmpPath: directoryLinksFilePath + ".tmp"})
        .then(() => {
          deferred.resolve();
          self._callObservers("onManyLinksChanged");
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
      xmlHttp.send(JSON.stringify({ locale: this.locale }));
    } catch (e) {
      deferred.reject("Error fetching " + uri);
      Cu.reportError(e);
    }
    return deferred.promise;
  },

  



  getLinks: function DirectoryLinksProvider_getLinks(aCallback) {
    this._fetchLinks(rawLinks => {
      
      aCallback(rawLinks.map((link, position) => {
        link.frecency = DIRECTORY_FRECENCY;
        link.lastVisitDate = rawLinks.length - position;
        return link;
      }));
    });
  },

  init: function DirectoryLinksProvider_init() {
    this._addPrefsObserver();
  },

  


  reset: function DirectoryLinksProvider_reset() {
    delete this.__linksURL;
    this._removePrefsObserver();
    this._removeObservers();
  },

  addObserver: function DirectoryLinksProvider_addObserver(aObserver) {
    this._observers.push(aObserver);
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
    while (this._observers.length) {
      this._observers.pop();
    }
  }
};
