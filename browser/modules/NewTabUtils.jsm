



"use strict";

let EXPORTED_SYMBOLS = ["NewTabUtils"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "gPrincipal", function () {
  let uri = Services.io.newURI("about:newtab", null, null);
  return Services.scriptSecurityManager.getNoAppCodebasePrincipal(uri);
});


const PREF_NEWTAB_ENABLED = "browser.newtabpage.enabled";


const PREF_NEWTAB_ROWS = "browser.newtabpage.rows";


const PREF_NEWTAB_COLUMNS = "browser.newtabpage.columns";


const HISTORY_RESULTS_LIMIT = 100;


const TOPIC_GATHER_TELEMETRY = "gather-telemetry";




let Storage = {
  


  get domStorage() {
    let sm = Services.domStorageManager;
    let storage = sm.getLocalStorageForPrincipal(gPrincipal, "");

    
    let descriptor = {value: storage, enumerable: true};
    Object.defineProperty(this, "domStorage", descriptor);

    return storage;
  },

  





  get: function Storage_get(aKey, aDefault) {
    let value;

    try {
      value = JSON.parse(this.domStorage.getItem(aKey));
    } catch (e) {}

    return value || aDefault;
  },

  




  set: function Storage_set(aKey, aValue) {
    this.domStorage.setItem(aKey, JSON.stringify(aValue));
  },

  


  clear: function Storage_clear() {
    this.domStorage.clear();
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
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

  



  _addObserver: function AllPages_addObserver() {
    Services.prefs.addObserver(PREF_NEWTAB_ENABLED, this, true);
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
    if (index != -1) {
      this.links[index] = null;
      this.save();
    }
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
    this.links[aLink.url] = 1;
    this.save();

    
    PinnedLinks.unpin(aLink);
  },

  



  unblock: function BlockedLinks_unblock(aLink) {
    if (this.isBlocked(aLink)) {
      delete this.links[aLink.url];
      this.save();
    }
  },

  


  save: function BlockedLinks_save() {
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

    let links = [];

    let callback = {
      handleResult: function (aResultSet) {
        let row;

        while (row = aResultSet.getNextRow()) {
          let url = row.getResultByIndex(1);
          if (LinkChecker.checkLoadURI(url)) {
            let title = row.getResultByIndex(2);
            links.push({url: url, title: title});
          }
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
  


  _links: null,

  


  _provider: PlacesProvider,

  


  _populateCallbacks: [],

  




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

    if (this._links && !aForce) {
      executeCallbacks();
    } else {
      this._provider.getLinks(function (aLinks) {
        this._links = aLinks;
        executeCallbacks();
      }.bind(this));

      this._addObserver();
    }
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
    this._links = null;
  },

  



  observe: function Links_observe(aSubject, aTopic, aData) {
    
    
    if (AllPages.length && AllPages.enabled)
      this.populateCache(function () { AllPages.update() }, true);
    else
      this._links = null;
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

Telemetry.init();






let LinkChecker = {
  _cache: {},

  get flags() {
    return Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;
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




let NewTabUtils = {
  


  restore: function NewTabUtils_restore() {
    Storage.clear();
    Links.resetCache();
    PinnedLinks.resetCache();
    BlockedLinks.resetCache();

    Links.populateCache(function () {
      AllPages.update();
    }, true);
  },

  links: Links,
  allPages: AllPages,
  linkChecker: LinkChecker,
  pinnedLinks: PinnedLinks,
  blockedLinks: BlockedLinks,
  gridPrefs: GridPrefs
};
