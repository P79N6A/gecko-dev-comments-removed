



"use strict";

this.EXPORTED_SYMBOLS = ["TabState"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "Messenger",
  "resource:///modules/sessionstore/Messenger.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TabStateCache",
  "resource:///modules/sessionstore/TabStateCache.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TabAttributes",
  "resource:///modules/sessionstore/TabAttributes.jsm");




this.TabState = Object.freeze({
  setSyncHandler: function (browser, handler) {
    TabStateInternal.setSyncHandler(browser, handler);
  },

  onSwapDocShells: function (browser, otherBrowser) {
    TabStateInternal.onSwapDocShells(browser, otherBrowser);
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

  


  setSyncHandler: function (browser, handler) {
    this._syncHandlers.set(browser, handler);
  },

  





  onSwapDocShells: function (browser, otherBrowser) {
    
    
    
    this.dropPendingCollections(browser);
    this.dropPendingCollections(otherBrowser);

    
    
    if (!this._syncHandlers.has(browser)) {
      [browser, otherBrowser] = [otherBrowser, browser];
      if (!this._syncHandlers.has(browser)) {
        return;
      }
    }

    
    
    let handler = this._syncHandlers.get(browser);
    if (this._syncHandlers.has(otherBrowser)) {
      let otherHandler = this._syncHandlers.get(otherBrowser);
      this._syncHandlers.set(browser, otherHandler);
      this._syncHandlers.set(otherBrowser, handler);
    } else {
      this._syncHandlers.set(otherBrowser, handler);
      this._syncHandlers.delete(browser);
    }
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

      
      let storage = yield Messenger.send(tab, "SessionStore:collectSessionStorage");

      
      let disallow = yield Messenger.send(tab, "SessionStore:collectDocShellCapabilities");

      let pageStyle = yield Messenger.send(tab, "SessionStore:collectPageStyle");

      
      let tabData = this._collectBaseTabData(tab);

      
      tabData.entries = history.entries;
      if ("index" in history) {
        tabData.index = history.index;
      }

      if (Object.keys(storage).length) {
        tabData.storage = storage;
      }

      if (disallow.length > 0) {
        tabData.disallow = disallow.join(",");
      }

      if (pageStyle) {
        tabData.pageStyle = pageStyle;
      }

      
      
      
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

    let history, storage, disallow, pageStyle;
    try {
      history = syncHandler.collectSessionHistory(includePrivateData);
      storage = syncHandler.collectSessionStorage();
      disallow = syncHandler.collectDocShellCapabilities();
      pageStyle = syncHandler.collectPageStyle();
    } catch (e) {
      
      Cu.reportError(e);
      return tabData;
    }

    tabData.entries = history.entries;
    if ("index" in history) {
      tabData.index = history.index;
    }

    if (Object.keys(storage).length) {
      tabData.storage = storage;
    }

    if (disallow.length > 0) {
      tabData.disallow = disallow.join(",");
    }

    if (pageStyle) {
      tabData.pageStyle = pageStyle;
    }

    return tabData;
  },

  



  _tabIsNew: function (tab) {
    let browser = tab.linkedBrowser;
    return (!browser || !browser.currentURI);
  },

  



  _tabIsRestoring: function (tab) {
    let browser = tab.linkedBrowser;
    return (browser.__SS_data && browser.__SS_tabStillLoading);
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
    if (browser.__SS_data && browser.__SS_tabStillLoading) {
      
      
      
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
