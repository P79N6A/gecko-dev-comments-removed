











































let Storage = {
  GROUP_DATA_IDENTIFIER: "tabview-group",
  GROUPS_DATA_IDENTIFIER: "tabview-groups",
  TAB_DATA_IDENTIFIER: "tabview-tab",
  UI_DATA_IDENTIFIER: "tabview-ui",
  CACHE_CLIENT_IDENTIFIER: "tabview-cache",
  CACHE_PREFIX: "moz-panorama:",

  
  
  
  toString: function Storage_toString() {
    return "[Storage]";
  },

  
  
  
  init: function Storage_init() {
    this._sessionStore =
      Cc["@mozilla.org/browser/sessionstore;1"].
        getService(Ci.nsISessionStore);
    
    
    let cacheService = 
      Cc["@mozilla.org/network/cache-service;1"].
        getService(Ci.nsICacheService);
    this._cacheSession = cacheService.createSession(
      this.CACHE_CLIENT_IDENTIFIER, Ci.nsICache.STORE_ON_DISK, true);
    this.StringInputStream = Components.Constructor(
      "@mozilla.org/io/string-input-stream;1", "nsIStringInputStream",
      "setData");
    this.StorageStream = Components.Constructor(
      "@mozilla.org/storagestream;1", "nsIStorageStream", 
      "init");
  },

  
  
  uninit: function Storage_uninit () {
    this._sessionStore = null;
    this._cacheSession = null;
    this.StringInputStream = null;
    this.StorageStream = null;
  },

  
  
  
  wipe: function Storage_wipe() {
    try {
      var self = this;

      
      AllTabs.tabs.forEach(function(tab) {
        if (tab.ownerDocument.defaultView != gWindow)
          return;

        self.saveTab(tab, null);
      });

      
      this.saveGroupItemsData(gWindow, {});
      this.saveUIData(gWindow, {});

      this._sessionStore.setWindowValue(gWindow, this.GROUP_DATA_IDENTIFIER,
        JSON.stringify({}));
    } catch (e) {
      Utils.log("Error in wipe: "+e);
    }
  },

  
  
  
  
  
  _openCacheEntry: function Storage__openCacheEntry(url, access, successCallback, errorCallback) {
    let onCacheEntryAvailable = function (entry, accessGranted, status) {
      if (entry && access == accessGranted && Components.isSuccessCode(status)) {
        successCallback(entry);
      } else {
        entry && entry.close();
        errorCallback();
      }
    }

    let key = this.CACHE_PREFIX + url;

    
    if (UI.isDOMWindowClosing) {
      let entry = this._cacheSession.openCacheEntry(key, access, true);
      let status = Components.results.NS_OK;
      onCacheEntryAvailable(entry, entry.accessGranted, status);
    } else {
      let listener = new CacheListener(onCacheEntryAvailable);
      this._cacheSession.asyncOpenCacheEntry(key, access, listener);
    }
  },

  
  
  
  
  
  saveThumbnail: function Storage_saveThumbnail(url, imageData, callback) {
    Utils.assert(url, "url");
    Utils.assert(imageData, "imageData");
    Utils.assert(typeof callback == "function", "callback arg must be a function");

    let self = this;
    let StringInputStream = this.StringInputStream;

    let onCacheEntryAvailable = function (entry) {
      let outputStream = entry.openOutputStream(0);

      let cleanup = function () {
        outputStream.close();
        entry.close();
      }

      
      if (UI.isDOMWindowClosing) {
        outputStream.write(imageData, imageData.length);
        cleanup();
        callback(true);
        return;
      }

      
      let inputStream = new StringInputStream(imageData, imageData.length);
      gNetUtil.asyncCopy(inputStream, outputStream, function (result) {
        cleanup();
        inputStream.close();
        callback(Components.isSuccessCode(result));
      });
    }

    let onCacheEntryUnavailable = function () {
      callback(false);
    }

    this._openCacheEntry(url, Ci.nsICache.ACCESS_WRITE,
        onCacheEntryAvailable, onCacheEntryUnavailable);
  },

  
  
  
  
  
  loadThumbnail: function Storage_loadThumbnail(url, callback) {
    Utils.assert(url, "url");
    Utils.assert(typeof callback == "function", "callback arg must be a function");

    let self = this;

    let onCacheEntryAvailable = function (entry) {
      let imageChunks = [];
      let nativeInputStream = entry.openInputStream(0);

      const CHUNK_SIZE = 0x10000; 
      const PR_UINT32_MAX = 0xFFFFFFFF;
      let storageStream = new self.StorageStream(CHUNK_SIZE, PR_UINT32_MAX, null);
      let storageOutStream = storageStream.getOutputStream(0);

      let cleanup = function () {
        nativeInputStream.close();
        storageStream.close();
        storageOutStream.close();
        entry.close();
      }

      gNetUtil.asyncCopy(nativeInputStream, storageOutStream, function (result) {
        
        if (typeof UI == "undefined") {
          cleanup();
          return;
        }

        let imageData = null;
        let isSuccess = Components.isSuccessCode(result);

        if (isSuccess) {
          let storageInStream = storageStream.newInputStream(0);
          imageData = gNetUtil.readInputStreamToString(storageInStream,
            storageInStream.available());
          storageInStream.close();
        }

        cleanup();
        callback(isSuccess, imageData);
      });
    }

    let onCacheEntryUnavailable = function () {
      callback(false);
    }

    this._openCacheEntry(url, Ci.nsICache.ACCESS_READ,
        onCacheEntryAvailable, onCacheEntryUnavailable);
  },

  
  
  
  saveTab: function Storage_saveTab(tab, data) {
    Utils.assert(tab, "tab");

    if (data != null) {
      let imageData = data.imageData;
      
      delete data.imageData;
      if (imageData != null) {
        this.saveThumbnail(data.url, imageData, function (status) {
          if (status) {
            
            tab._tabViewTabItem._sendToSubscribers("savedCachedImageData");
          } else {
            Utils.log("Error while saving thumbnail: " + e);
          }
        });
      }
    }

    this._sessionStore.setTabValue(tab, this.TAB_DATA_IDENTIFIER,
      JSON.stringify(data));
  },

  
  
  
  
  getTabData: function Storage_getTabData(tab, callback) {
    Utils.assert(tab, "tab");
    Utils.assert(typeof callback == "function", "callback arg must be a function");

    let existingData = null;

    try {
      let tabData = this._sessionStore.getTabValue(tab, this.TAB_DATA_IDENTIFIER);
      if (tabData != "") {
        existingData = JSON.parse(tabData);
      }
    } catch (e) {
      
      Utils.log(e);
    }

    if (existingData) {
      this.loadThumbnail(existingData.url, function (status, imageData) {
        if (status) {
          callback(imageData);

          
          tab._tabViewTabItem._sendToSubscribers("loadedCachedImageData");
        } else {
          Utils.log("Error while loading thumbnail");
        }
      });
    }

    return existingData;
  },

  
  
  
  saveGroupItem: function Storage_saveGroupItem(win, data) {
    var id = data.id;
    var existingData = this.readGroupItemData(win);
    existingData[id] = data;
    this._sessionStore.setWindowValue(win, this.GROUP_DATA_IDENTIFIER,
      JSON.stringify(existingData));
  },

  
  
  
  deleteGroupItem: function Storage_deleteGroupItem(win, id) {
    var existingData = this.readGroupItemData(win);
    delete existingData[id];
    this._sessionStore.setWindowValue(win, this.GROUP_DATA_IDENTIFIER,
      JSON.stringify(existingData));
  },

  
  
  
  readGroupItemData: function Storage_readGroupItemData(win) {
    var existingData = {};
    let data;
    try {
      data = this._sessionStore.getWindowValue(win, this.GROUP_DATA_IDENTIFIER);
      if (data)
        existingData = JSON.parse(data);
    } catch (e) {
      
      Utils.log("Error in readGroupItemData: "+e, data);
    }
    return existingData;
  },

  
  
  
  saveGroupItemsData: function Storage_saveGroupItemsData(win, data) {
    this.saveData(win, this.GROUPS_DATA_IDENTIFIER, data);
  },

  
  
  
  readGroupItemsData: function Storage_readGroupItemsData(win) {
    return this.readData(win, this.GROUPS_DATA_IDENTIFIER);
  },

  
  
  
  saveUIData: function Storage_saveUIData(win, data) {
    this.saveData(win, this.UI_DATA_IDENTIFIER, data);
  },

  
  
  
  readUIData: function Storage_readUIData(win) {
    return this.readData(win, this.UI_DATA_IDENTIFIER);
  },

  
  
  
  saveVisibilityData: function Storage_saveVisibilityData(win, data) {
    this._sessionStore.setWindowValue(
      win, win.TabView.VISIBILITY_IDENTIFIER, data);
  },

  
  
  
  saveData: function Storage_saveData(win, id, data) {
    try {
      this._sessionStore.setWindowValue(win, id, JSON.stringify(data));
    } catch (e) {
      Utils.log("Error in saveData: "+e);
    }
  },

  
  
  
  readData: function Storage_readData(win, id) {
    var existingData = {};
    try {
      var data = this._sessionStore.getWindowValue(win, id);
      if (data)
        existingData = JSON.parse(data);
    } catch (e) {
      Utils.log("Error in readData: "+e);
    }

    return existingData;
  }
};






function CacheListener(callback) {
  Utils.assert(typeof callback == "function", "callback arg must be a function");
  this.callback = callback;
};

CacheListener.prototype = {
  
  
  
  toString: function CacheListener_toString() {
    return "[CacheListener]";
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsICacheListener]),
  onCacheEntryAvailable: function (entry, access, status) {
    this.callback(entry, access, status);
  }
};
