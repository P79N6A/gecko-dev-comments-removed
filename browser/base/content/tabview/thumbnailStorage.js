










































let ThumbnailStorage = {
  CACHE_CLIENT_IDENTIFIER: "tabview-cache",
  CACHE_PREFIX: "moz-panorama:",

  
  _cacheSession: null,

  
  _stringInputStream: null,

  
  _storageStream: null,

  
  
  
  toString: function ThumbnailStorage_toString() {
    return "[ThumbnailStorage]";
  },

  
  
  
  init: function ThumbnailStorage_init() {
    
    let cacheService = 
      Cc["@mozilla.org/network/cache-service;1"].
        getService(Ci.nsICacheService);
    this._cacheSession = cacheService.createSession(
      this.CACHE_CLIENT_IDENTIFIER, Ci.nsICache.STORE_ON_DISK, true);
    this._stringInputStream = Components.Constructor(
      "@mozilla.org/io/string-input-stream;1", "nsIStringInputStream",
      "setData");
    this._storageStream = Components.Constructor(
      "@mozilla.org/storagestream;1", "nsIStorageStream", 
      "init");
  },

  
  
  
  
  
  _openCacheEntry: function ThumbnailStorage__openCacheEntry(url, access, successCallback, errorCallback) {
    let onCacheEntryAvailable = function(entry, accessGranted, status) {
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
      let status = Cr.NS_OK;
      onCacheEntryAvailable(entry, entry.accessGranted, status);
    } else {
      let listener = new CacheListener(onCacheEntryAvailable);
      this._cacheSession.asyncOpenCacheEntry(key, access, listener);
    }
  },

  
  
  
  
  
  saveThumbnail: function ThumbnailStorage_saveThumbnail(tab, imageData, callback) {
    Utils.assert(tab, "tab");
    Utils.assert(imageData, "imageData");

    if (!StoragePolicy.canStoreThumbnailForTab(tab)) {
      tab._tabViewTabItem._sendToSubscribers("deniedToCacheImageData");
      if (callback)
        callback(false);
      return;
    }

    let self = this;

    let completed = function(status) {
      if (callback)
        callback(status);

      if (status) {
        
        tab._tabViewTabItem._sendToSubscribers("savedCachedImageData");
      } else {
        Utils.log("Error while saving thumbnail: " + e);
      }
    };

    let onCacheEntryAvailable = function(entry) {
      let outputStream = entry.openOutputStream(0);

      let cleanup = function() {
        outputStream.close();
        entry.close();
      }

      
      if (UI.isDOMWindowClosing) {
        outputStream.write(imageData, imageData.length);
        cleanup();
        completed(true);
        return;
      }

      
      let inputStream = new self._stringInputStream(imageData, imageData.length);
      gNetUtil.asyncCopy(inputStream, outputStream, function (result) {
        cleanup();
        inputStream.close();
        completed(Components.isSuccessCode(result));
      });
    }

    let onCacheEntryUnavailable = function() {
      completed(false);
    }

    this._openCacheEntry(tab.linkedBrowser.currentURI.spec, 
        Ci.nsICache.ACCESS_WRITE, onCacheEntryAvailable, 
        onCacheEntryUnavailable);
  },

  
  
  
  
  
  loadThumbnail: function ThumbnailStorage_loadThumbnail(tab, url, callback) {
    Utils.assert(tab, "tab");
    Utils.assert(url, "url");
    Utils.assert(typeof callback == "function", "callback arg must be a function");

    let self = this;

    let completed = function(status, imageData) {
      callback(status, imageData);

      if (status) {
        
        tab._tabViewTabItem._sendToSubscribers("loadedCachedImageData");
      } else {
        Utils.log("Error while loading thumbnail");
      }
    }

    let onCacheEntryAvailable = function(entry) {
      let imageChunks = [];
      let nativeInputStream = entry.openInputStream(0);

      const CHUNK_SIZE = 0x10000; 
      const PR_UINT32_MAX = 0xFFFFFFFF;
      let storageStream = new self._storageStream(CHUNK_SIZE, PR_UINT32_MAX, null);
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
        completed(isSuccess, imageData);
      });
    }

    let onCacheEntryUnavailable = function() {
      completed(false);
    }

    this._openCacheEntry(url, Ci.nsICache.ACCESS_READ,
        onCacheEntryAvailable, onCacheEntryUnavailable);
  }
}






function CacheListener(callback) {
  Utils.assert(typeof callback == "function", "callback arg must be a function");
  this.callback = callback;
};

CacheListener.prototype = {
  
  
  
  toString: function CacheListener_toString() {
    return "[CacheListener]";
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsICacheListener]),
  onCacheEntryAvailable: function CacheListener_onCacheEntryAvailable(
    entry, access, status) {
    this.callback(entry, access, status);
  }
};

