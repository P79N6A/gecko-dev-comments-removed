



"use strict";

this.EXPORTED_SYMBOLS = ["TabState"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "Messenger",
  "resource:///modules/sessionstore/Messenger.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivacyLevel",
  "resource:///modules/sessionstore/PrivacyLevel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TabStateCache",
  "resource:///modules/sessionstore/TabStateCache.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TabAttributes",
  "resource:///modules/sessionstore/TabAttributes.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Utils",
  "resource:///modules/sessionstore/Utils.jsm");




this.TabState = Object.freeze({
  setSyncHandler: function (browser, handler) {
    TabStateInternal.setSyncHandler(browser, handler);
  },

  onBrowserContentsSwapped: function (browser, otherBrowser) {
    TabStateInternal.onBrowserContentsSwapped(browser, otherBrowser);
  },

  update: function (browser, data) {
    TabStateInternal.update(browser, data);
  },

  flush: function (browser) {
    TabStateInternal.flush(browser);
  },

  flushWindow: function (window) {
    TabStateInternal.flushWindow(window);
  },

  collect: function (tab) {
    return TabStateInternal.collect(tab);
  },

  collectSync: function (tab) {
    return TabStateInternal.collectSync(tab);
  },

  clone: function (tab) {
    return TabStateInternal.clone(tab);
  },

  dropPendingCollections: function (browser) {
    TabStateInternal.dropPendingCollections(browser);
  }
});

let TabStateInternal = {
  
  
  _pendingCollections: new WeakMap(),

  
  
  
  _syncHandlers: new WeakMap(),

  
  
  _latestMessageID: new WeakMap(),

  


  setSyncHandler: function (browser, handler) {
    this._syncHandlers.set(browser, handler);
    this._latestMessageID.set(browser, 0);
  },

  


  update: function (browser, {id, data}) {
    
    
    
    if (id > this._latestMessageID.get(browser)) {
      this._latestMessageID.set(browser, id);
      TabStateCache.updatePersistent(browser, data);
    }
  },

  


  flush: function (browser) {
    if (this._syncHandlers.has(browser)) {
      let lastID = this._latestMessageID.get(browser);
      this._syncHandlers.get(browser).flush(lastID);
    }
  },

  


  flushWindow: function (window) {
    for (let browser of window.gBrowser.browsers) {
      this.flush(browser);
    }
  },

  





  onBrowserContentsSwapped: function (browser, otherBrowser) {
    
    
    
    this.dropPendingCollections(browser);
    this.dropPendingCollections(otherBrowser);

    
    [this._syncHandlers, this._latestMessageID]
      .forEach(map => Utils.swapMapEntries(map, browser, otherBrowser));
  },

  







  collect: function (tab) {
    if (!tab) {
      throw new TypeError("Expecting a tab");
    }

    
    if (TabStateCache.has(tab)) {
      return Promise.resolve(TabStateCache.get(tab));
    }

    
    
    if (!this._tabNeedsExtraCollection(tab)) {
      let tabData = this._collectBaseTabData(tab);
      return Promise.resolve(tabData);
    }

    let browser = tab.linkedBrowser;

    let promise = Task.spawn(function task() {
      
      
      let history = yield Messenger.send(tab, "SessionStore:collectSessionHistory");

      
      let tabData = this._collectBaseTabData(tab);

      
      tabData.entries = history.entries;
      if ("index" in history) {
        tabData.index = history.index;
      }

      
      this._copyFromPersistentCache(tab, tabData);

      
      
      
      if (this._pendingCollections.get(browser) == promise) {
        TabStateCache.set(tab, tabData);
        this._pendingCollections.delete(browser);
      }

      throw new Task.Result(tabData);
    }.bind(this));

    
    
    
    this._pendingCollections.set(browser, promise);

    return promise;
  },

  









  collectSync: function (tab) {
    if (!tab) {
      throw new TypeError("Expecting a tab");
    }
    if (TabStateCache.has(tab)) {
      return TabStateCache.get(tab);
    }

    let tabData = this._collectSyncUncached(tab);

    if (this._tabCachingAllowed(tab)) {
      TabStateCache.set(tab, tabData);
    }

    
    
    
    
    
    this.dropPendingCollections(tab.linkedBrowser);

    return tabData;
  },

  







  dropPendingCollections: function (browser) {
    this._pendingCollections.delete(browser);
  },

  










  clone: function (tab) {
    return this._collectSyncUncached(tab, {includePrivateData: true});
  },

  




  _collectSyncUncached: function (tab, options = {}) {
    
    let tabData = this._collectBaseTabData(tab);

    
    if (!this._tabNeedsExtraCollection(tab)) {
      return tabData;
    }

    
    
    
    
    if (!this._syncHandlers.has(tab.linkedBrowser)) {
      return tabData;
    }

    let syncHandler = this._syncHandlers.get(tab.linkedBrowser);

    let includePrivateData = options && options.includePrivateData;

    let history;
    try {
      history = syncHandler.collectSessionHistory(includePrivateData);
    } catch (e) {
      
      Cu.reportError(e);
      return tabData;
    }

    tabData.entries = history.entries;
    if ("index" in history) {
      tabData.index = history.index;
    }

    
    this._copyFromPersistentCache(tab, tabData, options);

    return tabData;
  },

  









  _copyFromPersistentCache: function (tab, tabData, options = {}) {
    let data = TabStateCache.getPersistent(tab.linkedBrowser);

    
    if (!data) {
      return;
    }

    let includePrivateData = options && options.includePrivateData;

    for (let key of Object.keys(data)) {
      if (key != "storage" || includePrivateData) {
        tabData[key] = data[key];
      } else {
        tabData.storage = {};
        let isPinned = tab.pinned;

        
        
        for (let host of Object.keys(data.storage)) {
          let isHttps = host.startsWith("https:");
          if (PrivacyLevel.canSave({isHttps: isHttps, isPinned: isPinned})) {
            tabData.storage[host] = data.storage[host];
          }
        }
      }
    }
  },

  



  _tabIsNew: function (tab) {
    let browser = tab.linkedBrowser;
    return (!browser || !browser.currentURI);
  },

  



  _tabIsRestoring: function (tab) {
    return !!tab.linkedBrowser.__SS_data;
  },

  









  _tabNeedsExtraCollection: function (tab) {
    if (this._tabIsNew(tab)) {
      
      return false;
    }

    if (this._tabIsRestoring(tab)) {
      
      return false;
    }

    
    return true;
  },

  



  _tabCachingAllowed: function (tab) {
    if (this._tabIsNew(tab)) {
      
      return false;
    }

    if (this._tabIsRestoring(tab)) {
      
      
      
      return false;
    }

    return true;
  },

  







  _collectBaseTabData: function (tab) {
    let tabData = {entries: [], lastAccessed: tab.lastAccessed };
    let browser = tab.linkedBrowser;

    if (!browser || !browser.currentURI) {
      
      return tabData;
    }
    if (browser.__SS_data) {
      
      
      
      tabData = JSON.parse(JSON.stringify(browser.__SS_data));
      if (tab.pinned)
        tabData.pinned = true;
      else
        delete tabData.pinned;
      tabData.hidden = tab.hidden;

      
      if (tab.__SS_extdata)
        tabData.extData = tab.__SS_extdata;
      
      
      if (tabData.extData && !Object.keys(tabData.extData).length)
        delete tabData.extData;
      return tabData;
    }

    
    
    
    
    if (browser.userTypedValue) {
      tabData.userTypedValue = browser.userTypedValue;
      tabData.userTypedClear = browser.userTypedClear;
    } else {
      delete tabData.userTypedValue;
      delete tabData.userTypedClear;
    }

    if (tab.pinned)
      tabData.pinned = true;
    else
      delete tabData.pinned;
    tabData.hidden = tab.hidden;

    
    tabData.attributes = TabAttributes.get(tab);

    
    let tabbrowser = tab.ownerDocument.defaultView.gBrowser;
    tabData.image = tabbrowser.getIcon(tab);

    if (tab.__SS_extdata)
      tabData.extData = tab.__SS_extdata;
    else if (tabData.extData)
      delete tabData.extData;

    return tabData;
  }
};
