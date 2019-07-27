



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/DownloadsIPC.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");
XPCOMUtils.defineLazyServiceGetter(this, "volumeService",
                                   "@mozilla.org/telephony/volume-service;1",
                                    "nsIVolumeService");

function debug(aStr) {
#ifdef MOZ_DEBUG
  dump("-*- DownloadsAPI.js : " + aStr + "\n");
#endif
}

function DOMDownloadManagerImpl() {
  debug("DOMDownloadManagerImpl constructor");
}

DOMDownloadManagerImpl.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  
  init: function(aWindow) {
    debug("DownloadsManager init");
    this.initDOMRequestHelper(aWindow,
                              ["Downloads:Added",
                               "Downloads:Removed"]);
  },

  uninit: function() {
    debug("uninit");
    downloadsCache.evict(this._window);
  },

  set ondownloadstart(aHandler) {
    this.__DOM_IMPL__.setEventHandler("ondownloadstart", aHandler);
  },

  get ondownloadstart() {
    return this.__DOM_IMPL__.getEventHandler("ondownloadstart");
  },

  getDownloads: function() {
    debug("getDownloads()");

    return this.createPromise(function (aResolve, aReject) {
      DownloadsIPC.getDownloads().then(
        function(aDownloads) {
          
          
          let array = new this._window.Array();
          for (let id in aDownloads) {
            let dom = createDOMDownloadObject(this._window, aDownloads[id]);
            array.push(this._prepareForContent(dom));
          }
          aResolve(array);
        }.bind(this),
        function() {
          aReject("GetDownloadsError");
        }
      );
    }.bind(this));
  },

  clearAllDone: function() {
    debug("clearAllDone()");
    return this.createPromise(function (aResolve, aReject) {
      DownloadsIPC.clearAllDone().then(
        function(aDownloads) {
          
          
          let array = new this._window.Array();
          for (let id in aDownloads) {
            let dom = createDOMDownloadObject(this._window, aDownloads[id]);
            array.push(this._prepareForContent(dom));
          }
          aResolve(array);
        }.bind(this),
        function() {
          aReject("ClearAllDoneError");
        }
      );
    }.bind(this));
  },

  remove: function(aDownload) {
    debug("remove " + aDownload.url + " " + aDownload.id);
    return this.createPromise(function (aResolve, aReject) {
      if (!downloadsCache.has(this._window, aDownload.id)) {
        debug("no download " + aDownload.id);
        aReject("InvalidDownload");
        return;
      }

      DownloadsIPC.remove(aDownload.id).then(
        function(aResult) {
          let dom = createDOMDownloadObject(this._window, aResult);
          
          dom.wrappedJSObject.state = "finalized";
          aResolve(this._prepareForContent(dom));
        }.bind(this),
        function() {
          aReject("RemoveError");
        }
      );
    }.bind(this));
  },

  




  _prepareForContent: function(aChromeObject) {
    if (aChromeObject.__DOM_IMPL__) {
      return aChromeObject.__DOM_IMPL__;
    }
    let res = this._window.DOMDownload._create(this._window,
                                            aChromeObject.wrappedJSObject);
    return res;
  },

  receiveMessage: function(aMessage) {
    let data = aMessage.data;
    switch(aMessage.name) {
      case "Downloads:Added":
        debug("Adding " + uneval(data));
        let event = new this._window.DownloadEvent("downloadstart", {
          download:
            this._prepareForContent(createDOMDownloadObject(this._window, data))
        });
        this.__DOM_IMPL__.dispatchEvent(event);
        break;
    }
  },

  classID: Components.ID("{c6587afa-0696-469f-9eff-9dac0dd727fe}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

};




let downloadsCache = {
  init: function() {
    this.cache = new WeakMap();
  },

  has: function(aWindow, aId) {
    let downloads = this.cache.get(aWindow);
    return !!(downloads && downloads[aId]);
  },

  get: function(aWindow, aDownload) {
    let downloads = this.cache.get(aWindow);
    if (!(downloads && downloads[aDownload.id])) {
      debug("Adding download " + aDownload.id + " to cache.");
      if (!downloads) {
        this.cache.set(aWindow, {});
        downloads = this.cache.get(aWindow);
      }
      
      let impl = Cc["@mozilla.org/downloads/download;1"]
                   .createInstance(Ci.nsISupports);
      impl.wrappedJSObject._init(aWindow, aDownload);
      downloads[aDownload.id] = impl;
    }
    return downloads[aDownload.id];
  },

  evict: function(aWindow) {
    this.cache.delete(aWindow);
  }
};

downloadsCache.init();





function createDOMDownloadObject(aWindow, aDownload) {
  return downloadsCache.get(aWindow, aDownload);
}

function DOMDownloadImpl() {
  debug("DOMDownloadImpl constructor ");

  this.wrappedJSObject = this;
  this.totalBytes = 0;
  this.currentBytes = 0;
  this.url = null;
  this.path = null;
  this.storageName = null;
  this.storagePath = null;
  this.contentType = null;

  
  this._error = null;
  this._startTime = new Date();
  this._state = "stopped";

  
  this.id = null;
}

DOMDownloadImpl.prototype = {

  createPromise: function(aPromiseInit) {
    return new this._window.Promise(aPromiseInit);
  },

  pause: function() {
    debug("DOMDownloadImpl pause");
    let id = this.id;
    
    return this.createPromise(function(aResolve, aReject) {
      DownloadsIPC.pause(id).then(aResolve, aReject);
    });
  },

  resume: function() {
    debug("DOMDownloadImpl resume");
    let id = this.id;
    
    return this.createPromise(function(aResolve, aReject) {
      DownloadsIPC.resume(id).then(aResolve, aReject);
    });
  },

  set onstatechange(aHandler) {
    this.__DOM_IMPL__.setEventHandler("onstatechange", aHandler);
  },

  get onstatechange() {
    return this.__DOM_IMPL__.getEventHandler("onstatechange");
  },

  get error() {
    return this._error;
  },

  set error(aError) {
    this._error = aError;
  },

  get startTime() {
    return this._startTime;
  },

  set startTime(aStartTime) {
    if (aStartTime instanceof Date) {
      this._startTime = aStartTime;
    }
    else {
      this._startTime = new Date(aStartTime);
    }
  },

  get state() {
    return this._state;
  },

  
  
  
  
  set state(aState) {
    
    
    if (["downloading",
         "stopped",
         "succeeded",
         "finalized"].indexOf(aState) != -1) {
      this._state = aState;
    }
  },

  _init: function(aWindow, aDownload) {
    this._window = aWindow;
    this.id = aDownload.id;
    this._update(aDownload);
    Services.obs.addObserver(this, "downloads-state-change-" + this.id,
                              true);
    debug("observer set for " + this.id);
  },

  


  _update: function(aDownload) {
    debug("update " + uneval(aDownload));
    if (this.id != aDownload.id) {
      return;
    }

    let props = ["totalBytes", "currentBytes", "url", "path", "storageName",
                 "storagePath", "state", "contentType", "startTime"];
    let changed = false;
    let changedProps = {};

    props.forEach((prop) => {
      if (aDownload[prop] && (aDownload[prop] != this[prop])) {
        this[prop] = aDownload[prop];
        changedProps[prop] = changed = true;
      }
    });

    
    
    
    if (changedProps["path"]) {
      let storages = this._window.navigator.getDeviceStorages("sdcard");
      let preferredStorageName;
      
      
      storages.forEach((aStorage) => {
        if (aStorage.default || !preferredStorageName) {
          preferredStorageName = aStorage.storageName;
        }
      });
      
      let volume;
      if (preferredStorageName) {
        let volume = volumeService.getVolumeByName(preferredStorageName);
        if (volume) {
          
          
          
          this.storageName = preferredStorageName;
          this.storagePath =
            this.path.substring(this.path.indexOf(volume.mountPoint) +
                                volume.mountPoint.length + 1);
        }
      }
    }

    if (aDownload.error) {
      
      
      
      
      
      
      
      
      let result = aDownload.error.result;
      let storage = this._window.navigator.getDeviceStorage("sdcard");

      
      
      if (result == Cr.NS_ERROR_FAILURE && storage) {
        
        
        changed = false;
        debug("Attempting to infer error via device storage sanity checks.");
        
        let available = storage.available();
        available.onsuccess = (function() {
          debug("Storage Status = '" + available.result + "'");
          let inferredError = result;
          switch (available.result) {
            case "unavailable":
              inferredError = Cr.NS_ERROR_FILE_NOT_FOUND;
              break;
            case "shared":
              inferredError = Cr.NS_ERROR_FILE_ACCESS_DENIED;
              break;
          }
          this._updateWithError(aDownload, inferredError);
        }).bind(this);
        available.onerror = (function() {
          this._updateWithError(aDownload, result);
        }).bind(this);
      }

      this.error =
        new this._window.DOMError("DownloadError", result);
    } else {
      this.error = null;
    }

    
    if (!changed) {
      return;
    }

    this._sendStateChange();
  },

  _updateWithError: function(aDownload, aError) {
    this.error =
      new this._window.DOMError("DownloadError", aError);
    this._sendStateChange();
  },

  _sendStateChange: function() {
    
    if (this.__DOM_IMPL__) {
      let event = new this._window.DownloadEvent("statechange", {
        download: this.__DOM_IMPL__
      });
      debug("Dispatching statechange event. state=" + this.state);
      this.__DOM_IMPL__.dispatchEvent(event);
    }
  },

  observe: function(aSubject, aTopic, aData) {
    debug("DOMDownloadImpl observe " + aTopic);
    if (aTopic !== "downloads-state-change-" + this.id) {
      return;
    }

    try {
      let download = JSON.parse(aData);
      
      if (download.startTime) {
        download.startTime = new Date(download.startTime);
      }
      this._update(download);
    } catch(e) {}
  },

  classID: Components.ID("{96b81b99-aa96-439d-8c59-92eeed34705f}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DOMDownloadManagerImpl,
                                                     DOMDownloadImpl]);
