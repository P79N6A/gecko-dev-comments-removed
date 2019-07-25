










































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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  _openCacheEntry:
    function ThumbnailStorage__openCacheEntry(url, access, successCallback,
                                              errorCallback, options) {
    Utils.assert(url, "invalid or missing argument <url>");
    Utils.assert(access, "invalid or missing argument <access>");
    Utils.assert(successCallback, "invalid or missing argument <successCallback>");
    Utils.assert(errorCallback, "invalid or missing argument <errorCallback>");

    function onCacheEntryAvailable(entry, accessGranted, status) {
      if (entry && access == accessGranted && Components.isSuccessCode(status)) {
        successCallback(entry);
      } else {
        if (entry)
          entry.close();

        errorCallback();
      }
    }

    let key = this.CACHE_PREFIX + url;

    if (options && options.synchronously) {
      let entry = this._cacheSession.openCacheEntry(key, access, true);
      let status = Cr.NS_OK;
      onCacheEntryAvailable(entry, entry.accessGranted, status);
    } else {
      let listener = new CacheListener(onCacheEntryAvailable);
      this._cacheSession.asyncOpenCacheEntry(key, access, listener);
    }
  },

  
  
  
  
  
  
  
  
  
  
  
  
  saveThumbnail:
    function ThumbnailStorage_saveThumbnail(url, imageData, callback, options) {
    Utils.assert(url, "invalid or missing argument <url>");
    Utils.assert(imageData, "invalid or missing argument <imageData>");
    Utils.assert(callback, "invalid or missing argument <callback>");

    let synchronously = (options && options.synchronously);
    let self = this;

    function onCacheEntryAvailable(entry) {
      let outputStream = entry.openOutputStream(0);

      function cleanup() {
        outputStream.close();
        entry.close();
      }

      
      if (synchronously) {
        outputStream.write(imageData, imageData.length);
        cleanup();
        callback();
        return;
      }

      
      let inputStream = new self._stringInputStream(imageData, imageData.length);
      gNetUtil.asyncCopy(inputStream, outputStream, function (result) {
        cleanup();
        inputStream.close();
        callback(Components.isSuccessCode(result) ? "" : "failure");
      });
    }

    function onCacheEntryUnavailable() {
      callback("unavailable");
    }

    this._openCacheEntry(url, Ci.nsICache.ACCESS_WRITE, onCacheEntryAvailable,
                         onCacheEntryUnavailable, options);
  },

  
  
  
  
  
  
  
  loadThumbnail: function ThumbnailStorage_loadThumbnail(url, callback) {
    Utils.assert(url, "invalid or missing argument <url>");
    Utils.assert(callback, "invalid or missing argument <callback>");

    let self = this;

    function onCacheEntryAvailable(entry) {
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
        callback(isSuccess ? "" : "failure", imageData);
      });
    }

    function onCacheEntryUnavailable() {
      callback("unavailable");
    }

    this._openCacheEntry(url, Ci.nsICache.ACCESS_READ, onCacheEntryAvailable,
                         onCacheEntryUnavailable);
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

