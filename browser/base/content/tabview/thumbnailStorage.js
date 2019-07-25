










































let ThumbnailStorage = {
  CACHE_CLIENT_IDENTIFIER: "tabview-cache",
  CACHE_PREFIX: "moz-panorama:",
  PREF_DISK_CACHE_SSL: "browser.cache.disk_cache_ssl",

  
  _cacheSession: null,

  
  _stringInputStream: null,

  
  _storageStream: null,

  
  _progressListener: null,

  
  enablePersistentHttpsCaching: null,

  
  excludedBrowsers: [],

  
  
  
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

    
    this.enablePersistentHttpsCaching =
      Services.prefs.getBoolPref(this.PREF_DISK_CACHE_SSL);

    Services.prefs.addObserver(this.PREF_DISK_CACHE_SSL, this, false);

    let self = this;
    
    
    gBrowser.browsers.forEach(function(browser) {
      let checkAndAddToList = function(browserObj) {
        if (!self.enablePersistentHttpsCaching &&
            browserObj.currentURI.schemeIs("https"))
          self.excludedBrowsers.push(browserObj);
      };
      if (browser.contentDocument.readyState != "complete" ||
          browser.webProgress.isLoadingDocument) {
        browser.addEventListener("load", function() {
          browser.removeEventListener("load", arguments.callee, true);
          checkAndAddToList(browser);
        }, true);
      } else {
        checkAndAddToList(browser);
      }
    });
    gBrowser.addTabsProgressListener(this);
  },

  
  
  uninit: function ThumbnailStorage_uninit() {
    gBrowser.removeTabsProgressListener(this);
    Services.prefs.removeObserver(this.PREF_DISK_CACHE_SSL, this);
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

  
  
  _shouldSaveThumbnail : function ThumbnailStorage__shouldSaveThumbnail(tab) {
    return (this.excludedBrowsers.indexOf(tab.linkedBrowser) == -1);
  },

  
  
  
  
  
  saveThumbnail: function ThumbnailStorage_saveThumbnail(tab, imageData, callback) {
    Utils.assert(tab, "tab");
    Utils.assert(imageData, "imageData");
    
    if (!this._shouldSaveThumbnail(tab)) {
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
  },

  
  
  
  observe: function ThumbnailStorage_observe(subject, topic, data) {
    this.enablePersistentHttpsCaching =
      Services.prefs.getBoolPref(this.PREF_DISK_CACHE_SSL);
  },

  
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  onStateChange: function ThumbnailStorage_onStateChange(
    browser, webProgress, request, flag, status) {
    if (flag & Ci.nsIWebProgressListener.STATE_START &&
        flag & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
      
      if (webProgress.DOMWindow.parent == webProgress.DOMWindow) {
        let index = this.excludedBrowsers.indexOf(browser);
        if (index != -1)
          this.excludedBrowsers.splice(index, 1);
      }
    }
    if (flag & Ci.nsIWebProgressListener.STATE_STOP &&
        flag & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
      
      if (webProgress.DOMWindow.parent == webProgress.DOMWindow &&
          request && request instanceof Ci.nsIHttpChannel) {
        request.QueryInterface(Ci.nsIHttpChannel);

        let inhibitPersistentThumb = false;
        if (request.isNoStoreResponse()) {
           inhibitPersistentThumb = true;
        } else if (!this.enablePersistentHttpsCaching &&
                   request.URI.schemeIs("https")) {
          let cacheControlHeader;
          try {
            cacheControlHeader = request.getResponseHeader("Cache-Control");
          } catch(e) {
            
            
          }
          if (cacheControlHeader && !(/public/i).test(cacheControlHeader))
            inhibitPersistentThumb = true;
        }

        if (inhibitPersistentThumb &&
            this.excludedBrowsers.indexOf(browser) == -1)
          this.excludedBrowsers.push(browser);
      }
    }
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

