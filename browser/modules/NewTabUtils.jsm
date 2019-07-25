



"use strict";

let EXPORTED_SYMBOLS = ["NewTabUtils"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gPrivateBrowsing",
  "@mozilla.org/privatebrowsing;1", "nsIPrivateBrowsingService");

XPCOMUtils.defineLazyModuleGetter(this, "Dict", "resource://gre/modules/Dict.jsm");


const PREF_NEWTAB_ENABLED = "browser.newtabpage.enabled";


const HISTORY_RESULTS_LIMIT = 100;




let Storage = {
  


  get domStorage() {
    let uri = Services.io.newURI("about:newtab", null, null);
    let principal = Services.scriptSecurityManager.getCodebasePrincipal(uri);

    let sm = Services.domStorageManager;
    let storage = sm.getLocalStorageForPrincipal(principal, "");

    
    let descriptor = {value: storage, enumerable: true};
    Object.defineProperty(this, "domStorage", descriptor);

    return storage;
  },

  




  get currentStorage() {
    let storage = this.domStorage;

    
    if (gPrivateBrowsing.privateBrowsingEnabled)
      storage = new PrivateBrowsingStorage(storage);

    
    Services.obs.addObserver(this, "private-browsing", true);

    
    let descriptor = {value: storage, enumerable: true, writable: true};
    Object.defineProperty(this, "currentStorage", descriptor);

    return storage;
  },

  





  get: function Storage_get(aKey, aDefault) {
    let value;

    try {
      value = JSON.parse(this.currentStorage.getItem(aKey));
    } catch (e) {}

    return value || aDefault;
  },

  




  set: function Storage_set(aKey, aValue) {
    this.currentStorage.setItem(aKey, JSON.stringify(aValue));
  },

  


  clear: function Storage_clear() {
    this.currentStorage.clear();
  },

  



  observe: function Storage_observe(aSubject, aTopic, aData) {
    if (aData == "enter") {
      
      
      
      this.currentStorage = new PrivateBrowsingStorage(this.domStorage);
    } else {
      
      this.currentStorage = this.domStorage;

      
      
      
      PinnedLinks.resetCache();
      BlockedLinks.resetCache();

      Pages.update();
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};





function PrivateBrowsingStorage(aStorage) {
  this._data = new Dict();

  for (let i = 0; i < aStorage.length; i++) {
    let key = aStorage.key(i);
    this._data.set(key, aStorage.getItem(key));
  }
}

PrivateBrowsingStorage.prototype = {
  


  _data: null,

  





  getItem: function PrivateBrowsingStorage_getItem(aKey) {
    return this._data.get(aKey);
  },

  




  setItem: function PrivateBrowsingStorage_setItem(aKey, aValue) {
    this._data.set(aKey, aValue);
  },

  


  clear: function PrivateBrowsingStorage_clear() {
    this._data.listkeys().forEach(function (akey) {
      this._data.del(aKey);
    }, this);
  }
};




let AllPages = {
  


  _pages: [],

  


  _observing: false,

  


  _enabled: null,

  



  register: function AllPages_register(aPage) {
    this._pages.push(aPage);

    
    if (!this._observing) {
      this._observing = true;
      Services.prefs.addObserver(PREF_NEWTAB_ENABLED, this, true);
    }
  },

  



  unregister: function AllPages_unregister(aPage) {
    let index = this._pages.indexOf(aPage);
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

  



  update: function AllPages_update(aExceptPage) {
    this._pages.forEach(function (aPage) {
      if (aExceptPage != aPage)
        aPage.update();
    });
  },

  



  observe: function AllPages_observe() {
    
    this._enabled = null;

    let args = Array.slice(arguments);

    this._pages.forEach(function (aPage) {
      aPage.observe.apply(aPage, args);
    }, this);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};





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
    Storage.set("pinnedLinks", this.links);
  },

  



  unpin: function PinnedLinks_unpin(aLink) {
    let index = this._indexOfLink(aLink);
    if (index != -1) {
      this.links[index] = null;
      Storage.set("pinnedLinks", this.links);
    }
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
    this.links[aLink.url] = 1;

    
    PinnedLinks.unpin(aLink);

    Storage.set("blockedLinks", this.links);
  },

  



  isBlocked: function BlockedLinks_isBlocked(aLink) {
    return (aLink.url in this.links);
  },

  



  isEmpty: function BlockedLinks_isEmpty() {
    return Object.keys(this.links).length == 0;
  },

  


  resetCache: function BlockedLinks_resetCache() {
    this._links = null;
  }
};





let PlacesProvider = {
  



  getLinks: function PlacesProvider_getLinks(aCallback) {
    let options = PlacesUtils.history.getNewQueryOptions();
    options.maxResults = HISTORY_RESULTS_LIMIT;

    
    options.sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_FRECENCY_DESCENDING

    
    options.redirectsMode = Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_TARGET;

    let links = [];

    let callback = {
      handleResult: function (aResultSet) {
        let row;

        while (row = aResultSet.getNextRow()) {
          let url = row.getResultByIndex(1);
          let title = row.getResultByIndex(2);
          links.push({url: url, title: title});
        }
      },

      handleError: function (aError) {
        
        aCallback([]);
      },

      handleCompletion: function (aReason) {
        aCallback(links);
      }
    };

    
    let query = PlacesUtils.history.getNewQuery();
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase);
    db.asyncExecuteLegacyQueries([query], 1, options, callback);
  }
};










let Links = {
  


  _links: [],

  


  _provider: PlacesProvider,

  



  populateCache: function Links_populateCache(aCallback) {
    let self = this;

    this._provider.getLinks(function (aLinks) {
      self._links = aLinks;
      aCallback && aCallback();
    });
  },

  



  getLinks: function Links_getLinks() {
    let pinnedLinks = Array.slice(PinnedLinks.links);

    
    let links = this._links.filter(function (link) {
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
    this._links = [];
  }
};




let NewTabUtils = {
  _initialized: false,

  


  init: function NewTabUtils_init() {
    if (!this._initialized) {
      
      Links.populateCache();

      this._initialized = true;
    }
  },

  


  reset: function NewTabUtils_reset() {
    Storage.clear();
    Links.resetCache();
    PinnedLinks.resetCache();
    BlockedLinks.resetCache();
  },

  allPages: AllPages,
  links: Links,
  pinnedLinks: PinnedLinks,
  blockedLinks: BlockedLinks
};
