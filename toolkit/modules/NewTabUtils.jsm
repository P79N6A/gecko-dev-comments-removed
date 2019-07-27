



"use strict";

this.EXPORTED_SYMBOLS = ["NewTabUtils"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageThumbs",
  "resource://gre/modules/PageThumbs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BinarySearch",
  "resource://gre/modules/BinarySearch.jsm");

XPCOMUtils.defineLazyGetter(this, "gPrincipal", function () {
  let uri = Services.io.newURI("about:newtab", null, null);
  return Services.scriptSecurityManager.getNoAppCodebasePrincipal(uri);
});

XPCOMUtils.defineLazyGetter(this, "gCryptoHash", function () {
  return Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
});

XPCOMUtils.defineLazyGetter(this, "gUnicodeConverter", function () {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = 'utf8';
  return converter;
});


const PREF_NEWTAB_ENABLED = "browser.newtabpage.enabled";
const PREF_NEWTAB_ENHANCED = "browser.newtabpage.enhanced";


const PREF_NEWTAB_ROWS = "browser.newtabpage.rows";


const PREF_NEWTAB_COLUMNS = "browser.newtabpage.columns";


const HISTORY_RESULTS_LIMIT = 100;


const LINKS_GET_LINKS_LIMIT = 100;


const TOPIC_GATHER_TELEMETRY = "gather-telemetry";







function toHash(aValue) {
  let value = gUnicodeConverter.convertToByteArray(aValue);
  gCryptoHash.init(gCryptoHash.MD5);
  gCryptoHash.update(value, value.length);
  return gCryptoHash.finish(true);
}




XPCOMUtils.defineLazyGetter(this, "Storage", function() {
  return new LinksStorage();
});

function LinksStorage() {
  
  try {
    if (this._storedVersion < this._version) {
      
      if (this._storedVersion < 1) {
        
        
        throw new Error("Unsupported newTab storage version");
      }
      
    }
    else {
      
      
      
      
      
    }
  } catch (ex) {
    
    
    Components.utils.reportError(
      "Unable to migrate the newTab storage to the current version. "+
      "Restarting from scratch.\n" + ex);
    this.clear();
  }

  
  this._storedVersion = this._version;
}

LinksStorage.prototype = {
  get _version() 1,

  get _prefs() Object.freeze({
    pinnedLinks: "browser.newtabpage.pinned",
    blockedLinks: "browser.newtabpage.blocked",
  }),

  get _storedVersion() {
    if (this.__storedVersion === undefined) {
      try {
        this.__storedVersion =
          Services.prefs.getIntPref("browser.newtabpage.storageVersion");
      } catch (ex) {
        
        
        
        
        
        
        this.__storedVersion = 1;
      }
    }
    return this.__storedVersion;
  },
  set _storedVersion(aValue) {
    Services.prefs.setIntPref("browser.newtabpage.storageVersion", aValue);
    this.__storedVersion = aValue;
    return aValue;
  },

  





  get: function Storage_get(aKey, aDefault) {
    let value;
    try {
      let prefValue = Services.prefs.getComplexValue(this._prefs[aKey],
                                                     Ci.nsISupportsString).data;
      value = JSON.parse(prefValue);
    } catch (e) {}
    return value || aDefault;
  },

  




  set: function Storage_set(aKey, aValue) {
    
    let string = Cc["@mozilla.org/supports-string;1"]
                   .createInstance(Ci.nsISupportsString);
    string.data = JSON.stringify(aValue);
    Services.prefs.setComplexValue(this._prefs[aKey], Ci.nsISupportsString,
                                   string);
  },

  



  remove: function Storage_remove(aKey) {
    Services.prefs.clearUserPref(this._prefs[aKey]);
  },

  


  clear: function Storage_clear() {
    for (let key in this._prefs) {
      this.remove(key);
    }
  }
};





let AllPages = {
  


  _pages: [],

  


  _enabled: null,

  


  _enhanced: null,

  



  register: function AllPages_register(aPage) {
    this._pages.push(aPage);
    this._addObserver();
  },

  



  unregister: function AllPages_unregister(aPage) {
    let index = this._pages.indexOf(aPage);
    if (index > -1)
      this._pages.splice(index, 1);
  },

  


  get enabled() {
    if (this._enabled === null)
      this._enabled = Services.prefs.getBoolPref(PREF_NEWTAB_ENABLED);

    return this._enabled;
  },

  


  set enabled(aEnabled) {
    if (this.enabled != aEnabled)
      Services.prefs.setBoolPref(PREF_NEWTAB_ENABLED, !!aEnabled);
  },

  


  get enhanced() {
    if (this._enhanced === null)
      this._enhanced = Services.prefs.getBoolPref(PREF_NEWTAB_ENHANCED);

    return this._enhanced;
  },

  


  set enhanced(aEnhanced) {
    if (this.enhanced != aEnhanced)
      Services.prefs.setBoolPref(PREF_NEWTAB_ENHANCED, !!aEnhanced);
  },

  



  get length() {
    return this._pages.length;
  },

  




  update(aExceptPage, aReason = "") {
    for (let page of this._pages.slice()) {
      if (aExceptPage != page) {
        page.update(aReason);
      }
    }
  },

  



  observe: function AllPages_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed") {
      
      switch (aData) {
        case PREF_NEWTAB_ENABLED:
          this._enabled = null;
          break;
        case PREF_NEWTAB_ENHANCED:
          this._enhanced = null;
          break;
      }
    }
    
    this._pages.forEach(function (aPage) {
      aPage.observe(aSubject, aTopic, aData);
    }, this);
  },

  



  _addObserver: function AllPages_addObserver() {
    Services.prefs.addObserver(PREF_NEWTAB_ENABLED, this, true);
    Services.prefs.addObserver(PREF_NEWTAB_ENHANCED, this, true);
    Services.obs.addObserver(this, "page-thumbnail:create", true);
    this._addObserver = function () {};
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};




let GridPrefs = {
  


  _gridRows: null,
  get gridRows() {
    if (!this._gridRows) {
      this._gridRows = Math.max(1, Services.prefs.getIntPref(PREF_NEWTAB_ROWS));
    }

    return this._gridRows;
  },

  


  _gridColumns: null,
  get gridColumns() {
    if (!this._gridColumns) {
      this._gridColumns = Math.max(1, Services.prefs.getIntPref(PREF_NEWTAB_COLUMNS));
    }

    return this._gridColumns;
  },


  


  init: function GridPrefs_init() {
    Services.prefs.addObserver(PREF_NEWTAB_ROWS, this, false);
    Services.prefs.addObserver(PREF_NEWTAB_COLUMNS, this, false);
  },

  



  observe: function GridPrefs_observe(aSubject, aTopic, aData) {
    if (aData == PREF_NEWTAB_ROWS) {
      this._gridRows = null;
    } else {
      this._gridColumns = null;
    }

    AllPages.update();
  }
};

GridPrefs.init();





let PinnedLinks = {
  


  _links: null,

  


  get links() {
    if (!this._links)
      this._links = Storage.get("pinnedLinks", []);

    return this._links;
  },

  




  pin: function PinnedLinks_pin(aLink, aIndex) {
    
    this.unpin(aLink);

    this.links[aIndex] = aLink;
    this.save();
  },

  



  unpin: function PinnedLinks_unpin(aLink) {
    let index = this._indexOfLink(aLink);
    if (index == -1)
      return;
    let links = this.links;
    links[index] = null;
    
    let i=links.length-1;
    while (i >= 0 && links[i] == null)
      i--;
    links.splice(i +1);
    this.save();
  },

  


  save: function PinnedLinks_save() {
    Storage.set("pinnedLinks", this.links);
  },

  




  isPinned: function PinnedLinks_isPinned(aLink) {
    return this._indexOfLink(aLink) != -1;
  },

  


  resetCache: function PinnedLinks_resetCache() {
    this._links = null;
  },

  




  _indexOfLink: function PinnedLinks_indexOfLink(aLink) {
    for (let i = 0; i < this.links.length; i++) {
      let link = this.links[i];
      if (link && link.url == aLink.url)
        return i;
    }

    
    return -1;
  }
};




let BlockedLinks = {
  


  _links: null,

  


  get links() {
    if (!this._links)
      this._links = Storage.get("blockedLinks", {});

    return this._links;
  },

  



  block: function BlockedLinks_block(aLink) {
    this.links[toHash(aLink.url)] = 1;
    this.save();

    
    PinnedLinks.unpin(aLink);
  },

  



  unblock: function BlockedLinks_unblock(aLink) {
    if (this.isBlocked(aLink)) {
      delete this.links[toHash(aLink.url)];
      this.save();
    }
  },

  


  save: function BlockedLinks_save() {
    Storage.set("blockedLinks", this.links);
  },

  



  isBlocked: function BlockedLinks_isBlocked(aLink) {
    return (toHash(aLink.url) in this.links);
  },

  



  isEmpty: function BlockedLinks_isEmpty() {
    return Object.keys(this.links).length == 0;
  },

  


  resetCache: function BlockedLinks_resetCache() {
    this._links = null;
  }
};





let PlacesProvider = {
  


  maxNumLinks: HISTORY_RESULTS_LIMIT,

  


  init: function PlacesProvider_init() {
    PlacesUtils.history.addObserver(this, true);
  },

  



  getLinks: function PlacesProvider_getLinks(aCallback) {
    let options = PlacesUtils.history.getNewQueryOptions();
    options.maxResults = this.maxNumLinks;

    
    options.sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_FRECENCY_DESCENDING

    let links = [];

    let callback = {
      handleResult: function (aResultSet) {
        let row;

        while ((row = aResultSet.getNextRow())) {
          let url = row.getResultByIndex(1);
          if (LinkChecker.checkLoadURI(url)) {
            let title = row.getResultByIndex(2);
            let frecency = row.getResultByIndex(12);
            let lastVisitDate = row.getResultByIndex(5);
            links.push({
              url: url,
              title: title,
              frecency: frecency,
              lastVisitDate: lastVisitDate,
              type: "history",
            });
          }
        }
      },

      handleError: function (aError) {
        
        aCallback([]);
      },

      handleCompletion: function (aReason) {
        
        
        
        
        
        
        let i = 1;
        let outOfOrder = [];
        while (i < links.length) {
          if (Links.compareLinks(links[i - 1], links[i]) > 0)
            outOfOrder.push(links.splice(i, 1)[0]);
          else
            i++;
        }
        for (let link of outOfOrder) {
          i = BinarySearch.insertionIndexOf(Links.compareLinks, links, link);
          links.splice(i, 0, link);
        }

        aCallback(links);
      }
    };

    
    let query = PlacesUtils.history.getNewQuery();
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase);
    db.asyncExecuteLegacyQueries([query], 1, options, callback);
  },

  















  addObserver: function PlacesProvider_addObserver(aObserver) {
    this._observers.push(aObserver);
  },

  _observers: [],

  


  onFrecencyChanged: function PlacesProvider_onFrecencyChanged(aURI, aNewFrecency, aGUID, aHidden, aLastVisitDate) {
    
    
    if (!aHidden && aLastVisitDate) {
      this._callObservers("onLinkChanged", {
        url: aURI.spec,
        frecency: aNewFrecency,
        lastVisitDate: aLastVisitDate,
        type: "history",
      });
    }
  },

  


  onManyFrecenciesChanged: function PlacesProvider_onManyFrecenciesChanged() {
    this._callObservers("onManyLinksChanged");
  },

  


  onTitleChanged: function PlacesProvider_onTitleChanged(aURI, aNewTitle, aGUID) {
    this._callObservers("onLinkChanged", {
      url: aURI.spec,
      title: aNewTitle
    });
  },

  _callObservers: function PlacesProvider__callObservers(aMethodName, aArg) {
    for (let obs of this._observers) {
      if (obs[aMethodName]) {
        try {
          obs[aMethodName](this, aArg);
        } catch (err) {
          Cu.reportError(err);
        }
      }
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver,
                                         Ci.nsISupportsWeakReference]),
};













let Links = {
  


  maxNumLinks: LINKS_GET_LINKS_LIMIT,

  





  _providers: new Map(),

  


  _sortProperties: [
    "frecency",
    "lastVisitDate",
    "url",
  ],

  


  _populateCallbacks: [],

  



  addProvider: function Links_addProvider(aProvider) {
    this._providers.set(aProvider, null);
    aProvider.addObserver(this);
  },

  



  removeProvider: function Links_removeProvider(aProvider) {
    if (!this._providers.delete(aProvider))
      throw new Error("Unknown provider");
  },

  




  populateCache: function Links_populateCache(aCallback, aForce) {
    let callbacks = this._populateCallbacks;

    
    callbacks.push(aCallback);

    
    
    if (callbacks.length > 1)
      return;

    function executeCallbacks() {
      while (callbacks.length) {
        let callback = callbacks.shift();
        if (callback) {
          try {
            callback();
          } catch (e) {
            
          }
        }
      }
    }

    let numProvidersRemaining = this._providers.size;
    for (let [provider, links] of this._providers) {
      this._populateProviderCache(provider, () => {
        if (--numProvidersRemaining == 0)
          executeCallbacks();
      }, aForce);
    }

    this._addObserver();
  },

  



  getLinks: function Links_getLinks() {
    let pinnedLinks = Array.slice(PinnedLinks.links);
    let links = this._getMergedProviderLinks();

    let sites = new Set();
    for (let link of pinnedLinks) {
      if (link)
        sites.add(NewTabUtils.extractSite(link.url));
    }

    
    links = links.filter(function (link) {
      let site = NewTabUtils.extractSite(link.url);
      if (site == null || sites.has(site))
        return false;
      sites.add(site);

      return !BlockedLinks.isBlocked(link) && !PinnedLinks.isPinned(link);
    });

    
    for (let i = 0; i < pinnedLinks.length && links.length; i++)
      if (!pinnedLinks[i])
        pinnedLinks[i] = links.shift();

    
    if (links.length)
      pinnedLinks = pinnedLinks.concat(links);

    return pinnedLinks;
  },

  


  resetCache: function Links_resetCache() {
    for (let provider of this._providers.keys()) {
      this._providers.set(provider, null);
    }
  },

  









  compareLinks: function Links_compareLinks(aLink1, aLink2) {
    for (let prop of this._sortProperties) {
      if (!(prop in aLink1) || !(prop in aLink2))
        throw new Error("Comparable link missing required property: " + prop);
    }
    return aLink2.frecency - aLink1.frecency ||
           aLink2.lastVisitDate - aLink1.lastVisitDate ||
           aLink1.url.localeCompare(aLink2.url);
  },

  _incrementSiteMap: function(map, link) {
    let site = NewTabUtils.extractSite(link.url);
    map.set(site, (map.get(site) || 0) + 1);
  },

  _decrementSiteMap: function(map, link) {
    let site = NewTabUtils.extractSite(link.url);
    let previousURLCount = map.get(site);
    if (previousURLCount === 1) {
      map.delete(site);
    } else {
      map.set(site, previousURLCount - 1);
    }
  },

  populateProviderCache: function(provider, callback) {
    if (!this._providers.has(provider)) {
      throw new Error("Can only populate provider cache for existing provider.");
    }

    return this._populateProviderCache(provider, callback, false);
  },

  






  _populateProviderCache: function (aProvider, aCallback, aForce) {
    let cache = this._providers.get(aProvider);
    let createCache = !cache;
    if (createCache) {
      cache = {
        
        populatePromise: new Promise(resolve => resolve()),
      };
      this._providers.set(aProvider, cache);
    }
    
    cache.populatePromise = cache.populatePromise.then(() => {
      return new Promise(resolve => {
        if (!createCache && !aForce) {
          aCallback();
          resolve();
          return;
        }
        aProvider.getLinks(links => {
          
          
          links = links.filter((link) => !!link);
          cache.sortedLinks = links;
          cache.siteMap = links.reduce((map, link) => {
            this._incrementSiteMap(map, link);
            return map;
          }, new Map());
          cache.linkMap = links.reduce((map, link) => {
            map.set(link.url, link);
            return map;
          }, new Map());
          aCallback();
          resolve();
        });
      });
    });
  },

  



  _getMergedProviderLinks: function Links__getMergedProviderLinks() {
    
    let linkLists = [];
    for (let provider of this._providers.keys()) {
      if (!AllPages.enhanced && provider != PlacesProvider) {
        
        continue;
      }
      let links = this._providers.get(provider);
      if (links && links.sortedLinks) {
        linkLists.push(links.sortedLinks.slice());
      }
    }

    function getNextLink() {
      let minLinks = null;
      for (let links of linkLists) {
        if (links.length &&
            (!minLinks || Links.compareLinks(links[0], minLinks[0]) < 0))
          minLinks = links;
      }
      return minLinks ? minLinks.shift() : null;
    }

    let finalLinks = [];
    for (let nextLink = getNextLink();
         nextLink && finalLinks.length < this.maxNumLinks;
         nextLink = getNextLink()) {
      finalLinks.push(nextLink);
    }

    return finalLinks;
  },

  









  onLinkChanged: function Links_onLinkChanged(aProvider, aLink, aIndex=-1, aDeleted=false) {
    if (!("url" in aLink))
      throw new Error("Changed links must have a url property");

    let links = this._providers.get(aProvider);
    if (!links)
      
      
      
      return;

    let { sortedLinks, siteMap, linkMap } = links;
    let existingLink = linkMap.get(aLink.url);
    let insertionLink = null;
    let updatePages = false;

    if (existingLink) {
      
      
      if (this._sortProperties.some(prop => prop in aLink)) {
        let idx = aIndex;
        if (idx < 0) {
          idx = this._indexOf(sortedLinks, existingLink);
        } else if (this.compareLinks(aLink, sortedLinks[idx]) != 0) {
          throw new Error("aLink should be the same as sortedLinks[idx]");
        }

        if (idx < 0) {
          throw new Error("Link should be in _sortedLinks if in _linkMap");
        }
        sortedLinks.splice(idx, 1);

        if (aDeleted) {
          updatePages = true;
          linkMap.delete(existingLink.url);
          this._decrementSiteMap(siteMap, existingLink);
        } else {
          
          Object.assign(existingLink, aLink);

          
          insertionLink = existingLink;
        }
      }
      
      if ("title" in aLink && aLink.title != existingLink.title) {
        existingLink.title = aLink.title;
        updatePages = true;
      }
    }
    else if (this._sortProperties.every(prop => prop in aLink)) {
      
      
      if (sortedLinks.length && sortedLinks.length == aProvider.maxNumLinks) {
        let lastLink = sortedLinks[sortedLinks.length - 1];
        if (this.compareLinks(lastLink, aLink) < 0) {
          return;
        }
      }
      
      
      insertionLink = {};
      for (let prop in aLink) {
        insertionLink[prop] = aLink[prop];
      }
      linkMap.set(aLink.url, insertionLink);
      this._incrementSiteMap(siteMap, aLink);
    }

    if (insertionLink) {
      let idx = this._insertionIndexOf(sortedLinks, insertionLink);
      sortedLinks.splice(idx, 0, insertionLink);
      if (sortedLinks.length > aProvider.maxNumLinks) {
        let lastLink = sortedLinks.pop();
        linkMap.delete(lastLink.url);
        this._decrementSiteMap(siteMap, lastLink);
      }
      updatePages = true;
    }

    if (updatePages) {
      AllPages.update(null, "links-changed");
    }
  },

  


  onManyLinksChanged: function Links_onManyLinksChanged(aProvider) {
    this._populateProviderCache(aProvider, () => {
      AllPages.update(null, "links-changed");
    }, true);
  },

  _indexOf: function Links__indexOf(aArray, aLink) {
    return this._binsearch(aArray, aLink, "indexOf");
  },

  _insertionIndexOf: function Links__insertionIndexOf(aArray, aLink) {
    return this._binsearch(aArray, aLink, "insertionIndexOf");
  },

  _binsearch: function Links__binsearch(aArray, aLink, aMethod) {
    return BinarySearch[aMethod](this.compareLinks, aArray, aLink);
  },

  



  observe: function Links_observe(aSubject, aTopic, aData) {
    
    
    if (AllPages.length && AllPages.enabled)
      this.populateCache(function () { AllPages.update() }, true);
    else
      this.resetCache();
  },

  



  _addObserver: function Links_addObserver() {
    Services.obs.addObserver(this, "browser:purge-session-history", true);
    this._addObserver = function () {};
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};

Links.compareLinks = Links.compareLinks.bind(Links);





let Telemetry = {
  


  init: function Telemetry_init() {
    Services.obs.addObserver(this, TOPIC_GATHER_TELEMETRY, false);
  },

  


  _collect: function Telemetry_collect() {
    let probes = [
      { histogram: "NEWTAB_PAGE_ENABLED",
        value: AllPages.enabled },
      { histogram: "NEWTAB_PAGE_PINNED_SITES_COUNT",
        value: PinnedLinks.links.length },
      { histogram: "NEWTAB_PAGE_BLOCKED_SITES_COUNT",
        value: Object.keys(BlockedLinks.links).length }
    ];

    probes.forEach(function Telemetry_collect_forEach(aProbe) {
      Services.telemetry.getHistogramById(aProbe.histogram)
        .add(aProbe.value);
    });
  },

  


  observe: function Telemetry_observe(aSubject, aTopic, aData) {
    this._collect();
  }
};






let LinkChecker = {
  _cache: {},

  get flags() {
    return Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL |
           Ci.nsIScriptSecurityManager.DONT_REPORT_ERRORS;
  },

  checkLoadURI: function LinkChecker_checkLoadURI(aURI) {
    if (!(aURI in this._cache))
      this._cache[aURI] = this._doCheckLoadURI(aURI);

    return this._cache[aURI];
  },

  _doCheckLoadURI: function Links_doCheckLoadURI(aURI) {
    try {
      Services.scriptSecurityManager.
        checkLoadURIStrWithPrincipal(gPrincipal, aURI, this.flags);
      return true;
    } catch (e) {
      
      return false;
    }
  }
};

let ExpirationFilter = {
  init: function ExpirationFilter_init() {
    PageThumbs.addExpirationFilter(this);
  },

  filterForThumbnailExpiration:
  function ExpirationFilter_filterForThumbnailExpiration(aCallback) {
    if (!AllPages.enabled) {
      aCallback([]);
      return;
    }

    Links.populateCache(function () {
      let urls = [];

      
      for (let link of Links.getLinks().slice(0, 25)) {
        if (link && link.url)
          urls.push(link.url);
      }

      aCallback(urls);
    });
  }
};




this.NewTabUtils = {
  _initialized: false,

  





  extractSite: function Links_extractSite(url) {
    let uri;
    try {
      uri = Services.io.newURI(url, null, null);
    } catch (ex) {
      return null;
    }

    
    return uri.asciiHost.replace(/^(m|mobile|www\d*)\./, "");
  },

  init: function NewTabUtils_init() {
    if (this.initWithoutProviders()) {
      PlacesProvider.init();
      Links.addProvider(PlacesProvider);
    }
  },

  initWithoutProviders: function NewTabUtils_initWithoutProviders() {
    if (!this._initialized) {
      this._initialized = true;
      ExpirationFilter.init();
      Telemetry.init();
      return true;
    }
    return false;
  },

  getProviderLinks: function(aProvider) {
    let cache = Links._providers.get(aProvider);
    if (cache && cache.sortedLinks) {
      return cache.sortedLinks;
    }
    return [];
  },

  isTopSiteGivenProvider: function(aSite, aProvider) {
    let cache = Links._providers.get(aProvider);
    if (cache && cache.siteMap) {
      return cache.siteMap.has(aSite);
    }
    return false;
  },

  isTopPlacesSite: function(aSite) {
    return this.isTopSiteGivenProvider(aSite, PlacesProvider);
  },

  


  restore: function NewTabUtils_restore() {
    Storage.clear();
    Links.resetCache();
    PinnedLinks.resetCache();
    BlockedLinks.resetCache();

    Links.populateCache(function () {
      AllPages.update();
    }, true);
  },

  




  undoAll: function NewTabUtils_undoAll(aCallback) {
    Storage.remove("blockedLinks");
    Links.resetCache();
    BlockedLinks.resetCache();
    Links.populateCache(aCallback, true);
  },

  links: Links,
  allPages: AllPages,
  linkChecker: LinkChecker,
  pinnedLinks: PinnedLinks,
  blockedLinks: BlockedLinks,
  gridPrefs: GridPrefs,
  placesProvider: PlacesProvider
};
