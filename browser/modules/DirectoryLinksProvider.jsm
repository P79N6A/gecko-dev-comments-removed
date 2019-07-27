



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
Cu.import("resource://gre/modules/Timer.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
  "resource://gre/modules/NewTabUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyGetter(this, "gTextDecoder", () => {
  return new TextDecoder();
});


const DIRECTORY_LINKS_FILE = "directoryLinks.json";
const DIRECTORY_LINKS_TYPE = "application/json";


const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";


const PREF_SELECTED_LOCALE = "general.useragent.locale";


const PREF_DIRECTORY_SOURCE = "browser.newtabpage.directory.source";


const PREF_DIRECTORY_PING = "browser.newtabpage.directory.ping";


const PREF_NEWTAB_ENHANCED = "browser.newtabpage.enhanced";


const ALLOWED_LINK_SCHEMES = new Set(["http", "https"]);


const ALLOWED_IMAGE_SCHEMES = new Set(["https", "data"]);


const DIRECTORY_FRECENCY = 1000;


const SUGGESTED_FRECENCY = Infinity;


const DEFAULT_FREQUENCY_CAP = 5;


const PING_SCORE_DIVISOR = 10000;


const PING_ACTIONS = ["block", "click", "pin", "sponsored", "sponsored_link", "unpin", "view"];






let DirectoryLinksProvider = {

  __linksURL: null,

  _observers: new Set(),

  
  _downloadDeferred: null,

  
  _downloadIntervalMS: 86400000,

  


  _enhancedLinks: new Map(),

  


  _frequencyCaps: new Map(),

  


  _suggestedLinks: new Map(),

  


  _topSitesWithSuggestedLinks: new Set(),

  get _observedPrefs() Object.freeze({
    enhanced: PREF_NEWTAB_ENHANCED,
    linksURL: PREF_DIRECTORY_SOURCE,
    matchOSLocale: PREF_MATCH_OS_LOCALE,
    prefSelectedLocale: PREF_SELECTED_LOCALE,
  }),

  get _linksURL() {
    if (!this.__linksURL) {
      try {
        this.__linksURL = Services.prefs.getCharPref(this._observedPrefs["linksURL"]);

        
        if (this.locale == "en-US" && !Services.prefs.prefHasUserValue(this._observedPrefs["linksURL"])) {
          this.__linksURL = "data:text/plain;base64,ewogICAgImRpcmVjdG9yeSI6IFsKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDQ5OCwKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy9kMTFiYTBiMzA5NWJiMTlkODA5MmNkMjliZTljYmI5ZTE5NzY3MWVhLjI4MDg4LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzEzMzJhNjhiYWRmMTFlM2Y3ZjY5YmY3MzY0ZTc5YzBhN2UyNzUzYmMuNTMxNi5wbmciLAogICAgICAgICAgICAidGl0bGUiOiAiTW96aWxsYSBDb21tdW5pdHkiLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHA6Ly9jb250cmlidXRlLm1vemlsbGEub3JnLyIKICAgICAgICB9LAogICAgICAgIHsKICAgICAgICAgICAgImJnQ29sb3IiOiAiIiwKICAgICAgICAgICAgImRpcmVjdG9yeUlkIjogNTAwLAogICAgICAgICAgICAiZW5oYW5jZWRJbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzL2NjNjM3NzRiN2E5YWFlMDJmZTM2YmM1Y2FmOTBjMWUyNWU2NmEyYmMuMTM3OTEucG5nIiwKICAgICAgICAgICAgImltYWdlVVJJIjogImh0dHBzOi8vZHRleDRrdmJwcG92dC5jbG91ZGZyb250Lm5ldC9pbWFnZXMvZTgyMmNkNDYyOGM1MTYyMzEzZjQ5ZjVkNDU1NmY4YWFmZGYzODc1MC4xMTUxMy5wbmciLAogICAgICAgICAgICAidGl0bGUiOiAiTW96aWxsYSBNYW5pZmVzdG8iLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHBzOi8vd3d3Lm1vemlsbGEub3JnL2Fib3V0L21hbmlmZXN0by8iCiAgICAgICAgfSwKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDUwMiwKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy80MGU1NjMwNDA1ZDUwMzFjYTczMzkzYmQ3YmMwMDY0MTU2ZjJjYzgyLjEwOTg0LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzQ5MGQ0MmQxZjlhNzZjMDc3Mzk2MjZkMWI4YTU2OTE2OWFlYzhmYmUuMTEwMzkucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIkN1c3RvbWl6ZSBGaXJlZm94IiwKICAgICAgICAgICAgInR5cGUiOiAiYWZmaWxpYXRlIiwKICAgICAgICAgICAgInVybCI6ICJodHRwOi8vZmFzdGVzdGZpcmVmb3guY29tL2ZpcmVmb3gvZGVza3RvcC9jdXN0b21pemUvIgogICAgICAgIH0sCiAgICAgICAgewogICAgICAgICAgICAiYmdDb2xvciI6ICIiLAogICAgICAgICAgICAiZGlyZWN0b3J5SWQiOiA1MDQsCiAgICAgICAgICAgICJlbmhhbmNlZEltYWdlVVJJIjogImh0dHBzOi8vZHRleDRrdmJwcG92dC5jbG91ZGZyb250Lm5ldC9pbWFnZXMvODc3ZjFjNTYxZTczNWY3YjlmNDE5ZmY5YWM3OWViOGM3NDgxMTE5ZC4xNjc0NC5wbmciLAogICAgICAgICAgICAiaW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy8yNWM5ZmJiMDczMDhiODRkMTYwZmMxYjc5NTkzNjRhMmMxOGY5M2I5LjY0MDQucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIkZpcmVmb3ggTWFya2V0cGxhY2UiLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHBzOi8vbWFya2V0cGxhY2UuZmlyZWZveC5jb20vIgogICAgICAgIH0sCiAgICAgICAgewogICAgICAgICAgICAiYmdDb2xvciI6ICIjM2ZiNThlIiwKICAgICAgICAgICAgImRpcmVjdG9yeUlkIjogNTA1LAogICAgICAgICAgICAiZW5oYW5jZWRJbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzcyMDEyMWU3NDYyZDhjNzg2M2I0ZGQ4ZmE3YjVjMTA4OWI1ZjVmYjIuMzM4NjIucG5nIiwKICAgICAgICAgICAgImltYWdlVVJJIjogImh0dHBzOi8vZHRleDRrdmJwcG92dC5jbG91ZGZyb250Lm5ldC9pbWFnZXMvMGU2MDMxNjc1YTljNDkxZGQwYzY1ZTljNjdjZmJmNTRhNTg4MGYxNy4yMjk1LnN2ZyIsCiAgICAgICAgICAgICJ0aXRsZSI6ICJNb3ppbGxhIFdlYm1ha2VyIiwKICAgICAgICAgICAgInR5cGUiOiAiYWZmaWxpYXRlIiwKICAgICAgICAgICAgInVybCI6ICJodHRwczovL3dlYm1ha2VyLm9yZy8%2FdXRtX3NvdXJjZT1kaXJlY3RvcnktdGlsZXMmdXRtX21lZGl1bT1maXJlZm94LWJyb3dzZXIiCiAgICAgICAgfSwKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDUwNiwKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy9kOTcxY2JhZmEwMzA5YTIwMWU1MThhY2RhYzRmMWVlNGRhYmM3ZWFhLjE1MTA5LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzL2I0YWRjNThkZDNjMDJkYTM1NTEwNDk3N2I5MTAyNTUwNjBjZmQ2ZDguMTAzNTAucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIkZpcmVmb3ggU3luYyIsCiAgICAgICAgICAgICJ0eXBlIjogImFmZmlsaWF0ZSIsCiAgICAgICAgICAgICJ1cmwiOiAiaHR0cDovL21vemlsbGEtZXVyb3BlLm9yZy9maXJlZm94L3N5bmMiCiAgICAgICAgfSwKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDUwNywKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy8yMmZiODU2Y2Q1ODM2NTg1NWViNzI1YjE1NjVmMDhhNzI0NjRlMDM5LjE4NzE3LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzA2OGUwY2NiZDg3MDFhMjhlMmYwNzhjNjQwZWUwNzJiOWExNmUyZTEuMTI0OTAucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIlByaXZhY3kgUHJpbmNpcGxlcyIsCiAgICAgICAgICAgICJ0eXBlIjogImFmZmlsaWF0ZSIsCiAgICAgICAgICAgICJ1cmwiOiAiaHR0cDovL2V1cm9wZS5tb3ppbGxhLm9yZy9wcml2YWN5L3lvdSIKICAgICAgICB9CiAgICBdLAogICAgInN1Z2dlc3RlZCI6IFsKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiNjYWUxZjQiLAogICAgICAgICAgICAiZGlyZWN0b3J5SWQiOiA3MDIsCiAgICAgICAgICAgICJmcmVjZW50X3NpdGVzIjogWwogICAgICAgICAgICAgICAgImFkZG9ucy5tb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAiYWlyLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJibG9nLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJidWd6aWxsYS5tb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAiZGV2ZWxvcGVyLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJldGhlcnBhZC5tb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAiaGFja3MubW96aWxsYS5vcmciLAogICAgICAgICAgICAgICAgImhnLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJtb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAicGxhbmV0Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJxdWFsaXR5Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJzdXBwb3J0Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJzdXBwb3J0Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJ0cmVlaGVyZGVyLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJ3aWtpLm1vemlsbGEub3JnIgogICAgICAgICAgICBdLAogICAgICAgICAgICAiaW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy85ZWUyYjI2NTY3OGYyNzc1ZGUyZTRiZjY4MGRmNjAwYjUwMmU2MDM4LjM4NzUucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIlRoYW5rcyBmb3IgdGVzdGluZyEiLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHBzOi8vd3d3Lm1vemlsbGEuY29tL2ZpcmVmb3gvdGlsZXMiCiAgICAgICAgfQogICAgXQp9Cg%3D%3D";
        }
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

  


  _setDefaultEnhanced: function DirectoryLinksProvider_setDefaultEnhanced() {
    if (!Services.prefs.prefHasUserValue(PREF_NEWTAB_ENHANCED)) {
      let enhanced = true;
      try {
        
        if (Services.prefs.getBoolPref("privacy.donottrackheader.enabled")) {
          enhanced = false;
        }
      }
      catch(ex) {}
      Services.prefs.setBoolPref(PREF_NEWTAB_ENHANCED, enhanced);
    }
  },

  observe: function DirectoryLinksProvider_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed") {
      switch (aData) {
        
        case this._observedPrefs.enhanced:
          this._setDefaultEnhanced();
          break;

        case this._observedPrefs.linksURL:
          delete this.__linksURL;
          

        
        case this._observedPrefs.matchOSLocale:
        case this._observedPrefs.prefSelectedLocale:
          this._fetchAndCacheLinksIfNecessary(true);
          break;
      }
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

  _cacheSuggestedLinks: function(link) {
    if (!link.frecent_sites || "sponsored" == link.type) {
      
      return;
    }
    for (let suggestedSite of link.frecent_sites) {
      let suggestedMap = this._suggestedLinks.get(suggestedSite) || new Map();
      suggestedMap.set(link.url, link);
      this._suggestedLinks.set(suggestedSite, suggestedMap);
    }
  },

  _fetchAndCacheLinks: function DirectoryLinksProvider_fetchAndCacheLinks(uri) {
    
    uri = uri.replace("%LOCALE%", this.locale);
    uri = uri.replace("%CHANNEL%", UpdateChannel.get());

    let deferred = Promise.defer();
    let xmlHttp = new XMLHttpRequest();

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
      xmlHttp.open("GET", uri);
      
      xmlHttp.overrideMimeType(DIRECTORY_LINKS_TYPE);
      
      xmlHttp.setRequestHeader("Content-Type", DIRECTORY_LINKS_TYPE);
      xmlHttp.send();
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
    let emptyOutput = {directory: [], suggested: [], enhanced: []};
    return OS.File.read(this._directoryFilePath).then(binaryData => {
      let output;
      try {
        let json = gTextDecoder.decode(binaryData);
        let linksObj = JSON.parse(json);
        output = {directory: linksObj.directory || [],
                  suggested: linksObj.suggested || [],
                  enhanced:  linksObj.enhanced  || []};
      }
      catch (e) {
        Cu.reportError(e);
      }
      return output || emptyOutput;
    },
    error => {
      Cu.reportError(error);
      return emptyOutput;
    });
  },

  






  reportSitesAction: function DirectoryLinksProvider_reportSitesAction(sites, action, triggeringSiteIndex) {
    
    if (action == "view") {
      sites.slice(0, triggeringSiteIndex + 1).forEach(site => {
        let {targetedSite, url} = site.link;
        if (targetedSite) {
          this._decreaseFrequencyCap(url, 1);
        }
      });
    }
    
    else if (action == "click") {
      let {targetedSite, url} = sites[triggeringSiteIndex].link;
      if (targetedSite) {
        this._decreaseFrequencyCap(url, DEFAULT_FREQUENCY_CAP);
      }
    }

    let newtabEnhanced = false;
    let pingEndPoint = "";
    try {
      newtabEnhanced = Services.prefs.getBoolPref(PREF_NEWTAB_ENHANCED);
      pingEndPoint = Services.prefs.getCharPref(PREF_DIRECTORY_PING);
    }
    catch (ex) {}

    
    let invalidAction = PING_ACTIONS.indexOf(action) == -1;
    if (!newtabEnhanced || pingEndPoint == "" || invalidAction) {
      return Promise.resolve();
    }

    let actionIndex;
    let data = {
      locale: this.locale,
      tiles: sites.reduce((tiles, site, pos) => {
        
        if (site) {
          
          let {link} = site;
          let tilesIndex = tiles.length;
          if (triggeringSiteIndex == pos) {
            actionIndex = tilesIndex;
          }

          
          let id = link.directoryId;
          tiles.push({
            id: id || site.enhancedId,
            pin: site.isPinned() ? 1 : undefined,
            pos: pos != tilesIndex ? pos : undefined,
            score: Math.round(link.frecency / PING_SCORE_DIVISOR) || undefined,
            url: site.enhancedId && "",
          });
        }
        return tiles;
      }, []),
    };

    
    if (actionIndex !== undefined) {
      data[action] = actionIndex;
    }

    
    let ping = new XMLHttpRequest();
    ping.open("POST", pingEndPoint + (action == "view" ? "view" : "click"));
    ping.send(JSON.stringify(data));

    
    return this._fetchAndCacheLinksIfNecessary();
  },

  


  getEnhancedLink: function DirectoryLinksProvider_getEnhancedLink(link) {
    
    return link.enhancedImageURI && link ? link :
           this._enhancedLinks.get(NewTabUtils.extractSite(link.url));
  },

  


  isURLAllowed: function DirectoryLinksProvider_isURLAllowed(url, allowed) {
    
    if (!url) {
      return true;
    }

    let scheme = "";
    try {
      
      scheme = Services.io.newURI(url, null, null).scheme;
    }
    catch(ex) {}
    return allowed.has(scheme);
  },

  



  getLinks: function DirectoryLinksProvider_getLinks(aCallback) {
    this._readDirectoryLinksFile().then(rawLinks => {
      
      this._enhancedLinks.clear();
      this._frequencyCaps.clear();
      this._suggestedLinks.clear();

      let validityFilter = function(link) {
        
        return this.isURLAllowed(link.url, ALLOWED_LINK_SCHEMES) &&
               this.isURLAllowed(link.imageURI, ALLOWED_IMAGE_SCHEMES) &&
               this.isURLAllowed(link.enhancedImageURI, ALLOWED_IMAGE_SCHEMES);
      }.bind(this);

      rawLinks.suggested.filter(validityFilter).forEach((link, position) => {
        link.lastVisitDate = rawLinks.suggested.length - position;

        
        
        this._cacheSuggestedLinks(link);
        this._frequencyCaps.set(link.url, DEFAULT_FREQUENCY_CAP);
      });

      rawLinks.enhanced.filter(validityFilter).forEach((link, position) => {
        link.lastVisitDate = rawLinks.enhanced.length - position;

        
        if (link.enhancedImageURI) {
          this._enhancedLinks.set(NewTabUtils.extractSite(link.url), link);
        }
      });

      let links = rawLinks.directory.filter(validityFilter).map((link, position) => {
        link.lastVisitDate = rawLinks.directory.length - position;
        link.frecency = DIRECTORY_FRECENCY;
        return link;
      });

      
      this.maxNumLinks = links.length + 1;

      return links;
    }).catch(ex => {
      Cu.reportError(ex);
      return [];
    }).then(links => {
      aCallback(links);
      this._populatePlacesLinks();
    });
  },

  init: function DirectoryLinksProvider_init() {
    this._setDefaultEnhanced();
    this._addPrefsObserver();
    
    this._directoryFilePath = OS.Path.join(OS.Constants.Path.localProfileDir, DIRECTORY_LINKS_FILE);
    this._lastDownloadMS = 0;

    NewTabUtils.placesProvider.addObserver(this);

    return Task.spawn(function() {
      
      let doesFileExists = yield OS.File.exists(this._directoryFilePath);
      if (doesFileExists) {
        let fileInfo = yield OS.File.stat(this._directoryFilePath);
        this._lastDownloadMS = Date.parse(fileInfo.lastModificationDate);
      }
      
      yield this._fetchAndCacheLinksIfNecessary();
    }.bind(this));
  },

  _handleManyLinksChanged: function() {
    this._topSitesWithSuggestedLinks.clear();
    this._suggestedLinks.forEach((suggestedLinks, site) => {
      if (NewTabUtils.isTopPlacesSite(site)) {
        this._topSitesWithSuggestedLinks.add(site);
      }
    });
    this._updateSuggestedTile();
  },

  




  _handleLinkChanged: function(aLink) {
    let changedLinkSite = NewTabUtils.extractSite(aLink.url);
    let linkStored = this._topSitesWithSuggestedLinks.has(changedLinkSite);

    if (!NewTabUtils.isTopPlacesSite(changedLinkSite) && linkStored) {
      this._topSitesWithSuggestedLinks.delete(changedLinkSite);
      return true;
    }

    if (this._suggestedLinks.has(changedLinkSite) &&
        NewTabUtils.isTopPlacesSite(changedLinkSite) && !linkStored) {
      this._topSitesWithSuggestedLinks.add(changedLinkSite);
      return true;
    }
    return false;
  },

  _populatePlacesLinks: function () {
    NewTabUtils.links.populateProviderCache(NewTabUtils.placesProvider, () => {
      this._handleManyLinksChanged();
    });
  },

  onLinkChanged: function (aProvider, aLink) {
    
    setTimeout(() => {
      if (this._handleLinkChanged(aLink)) {
        this._updateSuggestedTile();
      }
    }, 0);
  },

  onManyLinksChanged: function () {
    
    setTimeout(() => {
      this._handleManyLinksChanged();
    }, 0);
  },

  




  _decreaseFrequencyCap(url, amount) {
    let remainingViews = this._frequencyCaps.get(url) - amount;
    this._frequencyCaps.set(url, remainingViews);

    
    if (remainingViews <= 0) {
      this._updateSuggestedTile();
    }
  },

  





  _updateSuggestedTile: function() {
    let sortedLinks = NewTabUtils.getProviderLinks(this);

    if (!sortedLinks) {
      
      
      return;
    }

    
    let initialLength = sortedLinks.length;
    if (initialLength) {
      let mostFrecentLink = sortedLinks[0];
      if (mostFrecentLink.targetedSite) {
        this._callObservers("onLinkChanged", {
          url: mostFrecentLink.url,
          frecency: SUGGESTED_FRECENCY,
          lastVisitDate: mostFrecentLink.lastVisitDate,
          type: mostFrecentLink.type,
        }, 0, true);
      }
    }

    if (this._topSitesWithSuggestedLinks.size == 0) {
      
      return;
    }

    
    
    
    
    
    let possibleLinks = new Map();
    let targetedSites = new Map();
    this._topSitesWithSuggestedLinks.forEach(topSiteWithSuggestedLink => {
      let suggestedLinksMap = this._suggestedLinks.get(topSiteWithSuggestedLink);
      suggestedLinksMap.forEach((suggestedLink, url) => {
        
        if (this._frequencyCaps.get(url) <= 0) {
          return;
        }

        possibleLinks.set(url, suggestedLink);

        
        
        if (!targetedSites.get(url)) {
          targetedSites.set(url, []);
        }
        targetedSites.get(url).push(topSiteWithSuggestedLink);
      })
    });

    
    let numLinks = possibleLinks.size;
    if (numLinks == 0) {
      return;
    }

    let flattenedLinks = [...possibleLinks.values()];

    
    let suggestedIndex = Math.floor(Math.random() * numLinks);
    let chosenSuggestedLink = flattenedLinks[suggestedIndex];

    
    this._callObservers("onLinkChanged", Object.assign({
      frecency: SUGGESTED_FRECENCY,

      
      
      
      targetedSite: targetedSites.get(chosenSuggestedLink.url).length ?
        targetedSites.get(chosenSuggestedLink.url)[0] : null
    }, chosenSuggestedLink));
    return chosenSuggestedLink;
   },

  


  reset: function DirectoryLinksProvider_reset() {
    delete this.__linksURL;
    this._removePrefsObserver();
    this._removeObservers();
  },

  addObserver: function DirectoryLinksProvider_addObserver(aObserver) {
    this._observers.add(aObserver);
  },

  removeObserver: function DirectoryLinksProvider_removeObserver(aObserver) {
    this._observers.delete(aObserver);
  },

  _callObservers(methodName, ...args) {
    for (let obs of this._observers) {
      if (typeof(obs[methodName]) == "function") {
        try {
          obs[methodName](this, ...args);
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
