



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
    for (let suggestedSite of link.frecent_sites) {
      let suggestedMap = this._suggestedLinks.get(suggestedSite) || new Map();
      suggestedMap.set(link.url, link);
      this._suggestedLinks.set(suggestedSite, suggestedMap);
    }
  },

  _fetchAndCacheLinks: function DirectoryLinksProvider_fetchAndCacheLinks(uri) {
    
    uri = uri.replace("%LOCALE%", this.locale);

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
    let emptyOutput = {directory: [], suggested: []};
    return OS.File.read(this._directoryFilePath).then(binaryData => {
      let output;
      try {
        let json = gTextDecoder.decode(binaryData);
        let linksObj = JSON.parse(json);
        output = {directory: linksObj.directory || [], suggested: linksObj.suggested || []};
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
    
    return link.enhancedImageURI && link ||
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

      let setCommonProperties = function(link, length, position) {
        
        if (link.enhancedImageURI) {
          this._enhancedLinks.set(NewTabUtils.extractSite(link.url), link);
        }
        link.lastVisitDate = length - position;
      }.bind(this);

      rawLinks.suggested.filter(validityFilter).forEach((link, position) => {
        setCommonProperties(link, rawLinks.suggested.length, position);

        
        
        this._cacheSuggestedLinks(link);
        this._frequencyCaps.set(link.url, DEFAULT_FREQUENCY_CAP);
      });

      let links = rawLinks.directory.filter(validityFilter).map((link, position) => {
        setCommonProperties(link, rawLinks.directory.length, position);
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

    
    this._callObservers("onLinkChanged", {
      url: chosenSuggestedLink.url,
      title: chosenSuggestedLink.title,
      frecency: SUGGESTED_FRECENCY,
      lastVisitDate: chosenSuggestedLink.lastVisitDate,
      type: chosenSuggestedLink.type,

      
      
      
      targetedSite: targetedSites.get(chosenSuggestedLink.url).length ?
        targetedSites.get(chosenSuggestedLink.url)[0] : null
    });
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
