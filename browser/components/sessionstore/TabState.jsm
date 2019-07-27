



"use strict";

this.EXPORTED_SYMBOLS = ["TabState"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivacyFilter",
  "resource:///modules/sessionstore/PrivacyFilter.jsm");
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

  update: function (browser, data) {
    TabStateInternal.update(browser, data);
  },

  flush: function (browser) {
    TabStateInternal.flush(browser);
  },

  flushAsync: function (browser) {
    TabStateInternal.flushAsync(browser);
  },

  flushWindow: function (window) {
    TabStateInternal.flushWindow(window);
  },

  collect: function (tab) {
    return TabStateInternal.collect(tab);
  },

  clone: function (tab) {
    return TabStateInternal.clone(tab);
  }
});

let TabStateInternal = {
  
  
  
  _syncHandlers: new WeakMap(),

  
  
  _latestMessageID: new WeakMap(),

  


  setSyncHandler: function (browser, handler) {
    this._syncHandlers.set(browser.permanentKey, handler);
    this._latestMessageID.set(browser.permanentKey, 0);
  },

  


  update: function (browser, {id, data}) {
    
    
    
    if (id > this._latestMessageID.get(browser.permanentKey)) {
      this._latestMessageID.set(browser.permanentKey, id);
      TabStateCache.update(browser, data);
    }
  },

  


  flush: function (browser) {
    if (this._syncHandlers.has(browser.permanentKey)) {
      let lastID = this._latestMessageID.get(browser.permanentKey);
      this._syncHandlers.get(browser.permanentKey).flush(lastID);
    }
  },

  





  flushAsync: function(browser) {
    if (this._syncHandlers.has(browser.permanentKey)) {
      this._syncHandlers.get(browser.permanentKey).flushAsync();
    }
  },

  


  flushWindow: function (window) {
    for (let browser of window.gBrowser.browsers) {
      this.flush(browser);
    }
  },

  









  collect: function (tab) {
    return this._collectBaseTabData(tab);
  },

  










  clone: function (tab) {
    return this._collectBaseTabData(tab, {includePrivateData: true});
  },

  









  _collectBaseTabData: function (tab, options) {
    let tabData = {entries: [], lastAccessed: tab.lastAccessed };
    let browser = tab.linkedBrowser;

    if (!browser || !browser.currentURI) {
      
      return tabData;
    }
    if (browser.__SS_data) {
      
      
      
      tabData = Utils.shallowCopy(browser.__SS_data);
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

    
    
    this._copyFromCache(tab, tabData, options);

    return tabData;
  },

  









  _copyFromCache: function (tab, tabData, options = {}) {
    let data = TabStateCache.get(tab.linkedBrowser);
    if (!data) {
      return;
    }

    
    let includePrivateData = options && options.includePrivateData;

    for (let key of Object.keys(data)) {
      let value = data[key];

      
      if (!includePrivateData) {
        if (key === "storage") {
          value = PrivacyFilter.filterSessionStorageData(value, tab.pinned);
        } else if (key === "formdata") {
          value = PrivacyFilter.filterFormData(value, tab.pinned);
        }
      }

      if (key === "history") {
        tabData.entries = value.entries;

        if (value.hasOwnProperty("index")) {
          tabData.index = value.index;
        }
      } else if (value) {
        tabData[key] = value;
      }
    }
  }
};
