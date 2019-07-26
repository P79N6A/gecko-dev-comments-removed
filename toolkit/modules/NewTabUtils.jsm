



"use strict";

this.EXPORTED_SYMBOLS = ["NewTabUtils"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageThumbs",
  "resource://gre/modules/PageThumbs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BinarySearch",
  "resource://gre/modules/BinarySearch.jsm");

XPCOMUtils.defineLazyGetter(this, "Timer", () => {
  return Cu.import("resource://gre/modules/Timer.jsm", {});
});

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


const PREF_NEWTAB_ROWS = "browser.newtabpage.rows";


const PREF_NEWTAB_COLUMNS = "browser.newtabpage.columns";


const HISTORY_RESULTS_LIMIT = 100;


const LINKS_GET_LINKS_LIMIT = 100;


const TOPIC_GATHER_TELEMETRY = "gather-telemetry";


const SCHEDULE_UPDATE_TIMEOUT_MS = 1000;







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

  



  get length() {
    return this._pages.length;
  },

  





  update: function AllPages_update(aExceptPage, aHiddenPagesOnly=false) {
    this._pages.forEach(function (aPage) {
      if (aExceptPage != aPage)
        aPage.update(aHiddenPagesOnly);
    });
  },

  




  scheduleUpdateForHiddenPages: function AllPages_scheduleUpdateForHiddenPages() {
    if (!this._scheduleUpdateTimeout) {
      this._scheduleUpdateTimeout = Timer.setTimeout(() => {
        delete this._scheduleUpdateTimeout;
        this.update(null, true);
      }, SCHEDULE_UPDATE_TIMEOUT_MS);
    }
  },

  get updateScheduledForHiddenPages() {
    return !!this._scheduleUpdateTimeout;
  },

  



  observe: function AllPages_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed") {
      
      this._enabled = null;
    }
    
    this._pages.forEach(function (aPage) {
      aPage.observe(aSubject, aTopic, aData);
    }, this);
  },

  



  _addObserver: function AllPages_addObserver() {
    Services.prefs.addObserver(PREF_NEWTAB_ENABLED, this, true);
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
              bgColor: "transparent",
              type: "history",
              imageURISpec: null,
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
          i = BinarySearch.insertionIndexOf(links, link,
                                            Links.compareLinks.bind(Links));
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

  


  _providers: new Set(),

  




  _providerLinks: new Map(),

  


  _sortProperties: [
    "frecency",
    "lastVisitDate",
    "url",
  ],

  


  _populateCallbacks: [],

  



  addProvider: function Links_addProvider(aProvider) {
    this._providers.add(aProvider);
    aProvider.addObserver(this);
  },

  



  removeProvider: function Links_removeProvider(aProvider) {
    if (!this._providers.delete(aProvider))
      throw new Error("Unknown provider");
    this._providerLinks.delete(aProvider);
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
    for (let provider of this._providers) {
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

    
    links = links.filter(function (link) {
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
    this._providerLinks.clear();
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

  






  _populateProviderCache: function Links_populateProviderCache(aProvider, aCallback, aForce) {
    if (this._providerLinks.has(aProvider) && !aForce) {
      aCallback();
    } else {
      aProvider.getLinks(links => {
        
        
        links = links.filter((link) => !!link);
        this._providerLinks.set(aProvider, {
          sortedLinks: links,
          linkMap: links.reduce((map, link) => {
            map.set(link.url, link);
            return map;
          }, new Map()),
        });
        aCallback();
      });
    }
  },

  



  _getMergedProviderLinks: function Links__getMergedProviderLinks() {
    
    let linkLists = [];
    for (let links of this._providerLinks.values()) {
      linkLists.push(links.sortedLinks.slice());
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

  






  onLinkChanged: function Links_onLinkChanged(aProvider, aLink) {
    if (!("url" in aLink))
      throw new Error("Changed links must have a url property");

    let links = this._providerLinks.get(aProvider);
    if (!links)
      
      
      
      return;

    let { sortedLinks, linkMap } = links;

    
    
    if (!linkMap.has(aLink.url) &&
        sortedLinks.length &&
        sortedLinks.length == aProvider.maxNumLinks) {
      let lastLink = sortedLinks[sortedLinks.length - 1];
      if (this.compareLinks(lastLink, aLink) < 0)
        return;
    }

    let updatePages = false;

    
    if ("title" in aLink) {
      let link = linkMap.get(aLink.url);
      if (link && link.title != aLink.title) {
        link.title = aLink.title;
        updatePages = true;
      }
    }

    
    if (this._sortProperties.some((prop) => prop in aLink)) {
      let link = linkMap.get(aLink.url);
      if (link) {
        
        let idx = this._indexOf(sortedLinks, link);
        if (idx < 0)
          throw new Error("Link should be in _sortedLinks if in _linkMap");
        sortedLinks.splice(idx, 1);
        for (let prop of this._sortProperties) {
          if (prop in aLink)
            link[prop] = aLink[prop];
        }
      }
      else {
        
        for (let prop of this._sortProperties) {
          if (!(prop in aLink))
            throw new Error("New link missing required sort property: " + prop);
        }
        
        
        link = {};
        for (let [prop, val] of Iterator(aLink)) {
          link[prop] = val;
        }
        linkMap.set(link.url, link);
      }
      let idx = this._insertionIndexOf(sortedLinks, link);
      sortedLinks.splice(idx, 0, link);
      if (sortedLinks.length > aProvider.maxNumLinks) {
        let lastLink = sortedLinks.pop();
        linkMap.delete(lastLink.url);
      }
      updatePages = true;
    }

    if (updatePages)
      AllPages.scheduleUpdateForHiddenPages();
  },

  


  onManyLinksChanged: function Links_onManyLinksChanged(aProvider) {
    this._populateProviderCache(aProvider, () => {
      AllPages.scheduleUpdateForHiddenPages();
    }, true);
  },

  _indexOf: function Links__indexOf(aArray, aLink) {
    return this._binsearch(aArray, aLink, "indexOf");
  },

  _insertionIndexOf: function Links__insertionIndexOf(aArray, aLink) {
    return this._binsearch(aArray, aLink, "insertionIndexOf");
  },

  _binsearch: function Links__binsearch(aArray, aLink, aMethod) {
    return BinarySearch[aMethod](aArray, aLink, this.compareLinks.bind(this));
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
  gridPrefs: GridPrefs
};
