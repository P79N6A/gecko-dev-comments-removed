



"use strict";

this.EXPORTED_SYMBOLS = ["PageThumbs", "PageThumbsStorage"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const HTML_NAMESPACE = "http://www.w3.org/1999/xhtml";
const PREF_STORAGE_VERSION = "browser.pagethumbnails.storage_version";
const LATEST_STORAGE_VERSION = 3;

const EXPIRATION_MIN_CHUNK_SIZE = 50;
const EXPIRATION_INTERVAL_SECS = 3600;




const MAX_THUMBNAIL_AGE_SECS = 172800; 




const THUMBNAIL_DIRECTORY = "thumbnails";




const THUMBNAIL_BG_COLOR = "#fff";

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", this);
Cu.import("resource://gre/modules/osfile.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gUpdateTimerManager",
  "@mozilla.org/updates/timer-manager;1", "nsIUpdateTimerManager");

XPCOMUtils.defineLazyGetter(this, "gCryptoHash", function () {
  return Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
});

XPCOMUtils.defineLazyGetter(this, "gUnicodeConverter", function () {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = 'utf8';
  return converter;
});

XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");




const TaskUtils = {
  






  captureErrors: function captureErrors(promise) {
    return promise.then(
      null,
      function onError(reason) {
        Cu.reportError("Uncaught asynchronous error: " + reason + " at\n"
          + reason.stack + "\n");
        throw reason;
      }
    );
  },

  










  spawn: function spawn(gen) {
    return this.captureErrors(Task.spawn(gen));
  },
  






  readBlob: function readBlob(blob) {
    let deferred = Promise.defer();
    let reader = Cc["@mozilla.org/files/filereader;1"].createInstance(Ci.nsIDOMFileReader);
    reader.onloadend = function onloadend() {
      if (reader.readyState != Ci.nsIDOMFileReader.DONE) {
        deferred.reject(reader.error);
      } else {
        deferred.resolve(reader.result);
      }
    };
    reader.readAsArrayBuffer(blob);
    return deferred.promise;
  }
};








this.PageThumbs = {
  _initialized: false,

  


  _thumbnailWidth : 0,
  _thumbnailHeight : 0,

  


  get scheme() "moz-page-thumb",

  


  get staticHost() "thumbnail",

  


  get contentType() "image/png",

  init: function PageThumbs_init() {
    if (!this._initialized) {
      this._initialized = true;
      PlacesUtils.history.addObserver(PageThumbsHistoryObserver, false);

      
      PageThumbsStorageMigrator.migrate();
      PageThumbsExpiration.init();
    }
  },

  uninit: function PageThumbs_uninit() {
    if (this._initialized) {
      this._initialized = false;
      PlacesUtils.history.removeObserver(PageThumbsHistoryObserver);
    }
  },

  




  getThumbnailURL: function PageThumbs_getThumbnailURL(aUrl) {
    return this.scheme + "://" + this.staticHost +
           "?url=" + encodeURIComponent(aUrl);
  },

  






  captureIfStale: function PageThumbs_captureIfStale(aUrl) {
    let filePath = PageThumbsStorage.getFilePathForURL(aUrl);
    PageThumbsWorker.post("isFileRecent", [filePath, MAX_THUMBNAIL_AGE_SECS]
    ).then(result => {
      if (!result.ok) {
        
        
        let BPT = Cu.import("resource://gre/modules/BackgroundPageThumbs.jsm", {}).BackgroundPageThumbs;
        BPT.capture(aUrl);
      }
    });
  },

  






  capture: function PageThumbs_capture(aWindow, aCallback) {
    if (!this._prefEnabled()) {
      return;
    }

    let canvas = this._createCanvas();
    this.captureToCanvas(aWindow, canvas);

    
    
    
    Services.tm.currentThread.dispatch(function () {
      canvas.mozFetchAsStream(aCallback, this.contentType);
    }.bind(this), Ci.nsIThread.DISPATCH_NORMAL);
  },


  






  captureToBlob: function PageThumbs_captureToBlob(aWindow) {
    if (!this._prefEnabled()) {
      return null;
    }

    let canvas = this._createCanvas();
    this.captureToCanvas(aWindow, canvas);

    let deferred = Promise.defer();
    let type = this.contentType;
    
    
    
    canvas.toBlob(function asBlob(blob) {
      deferred.resolve(blob, type);
    });
    return deferred.promise;
  },

  




  captureToCanvas: function PageThumbs_captureToCanvas(aWindow, aCanvas) {
    let telemetryCaptureTime = new Date();
    this._captureToCanvas(aWindow, aCanvas);
    let telemetry = Services.telemetry;
    telemetry.getHistogramById("FX_THUMBNAILS_CAPTURE_TIME_MS")
      .add(new Date() - telemetryCaptureTime);
  },

  
  
  _captureToCanvas: function PageThumbs__captureToCanvas(aWindow, aCanvas) {
    let [sw, sh, scale] = this._determineCropSize(aWindow, aCanvas);
    let ctx = aCanvas.getContext("2d");

    
    ctx.save();
    ctx.scale(scale, scale);

    try {
      
      ctx.drawWindow(aWindow, 0, 0, sw, sh, THUMBNAIL_BG_COLOR,
                     ctx.DRAWWINDOW_DO_NOT_FLUSH);
    } catch (e) {
      
    }

    ctx.restore();
  },

  




  captureAndStore: function PageThumbs_captureAndStore(aBrowser, aCallback) {
    if (!this._prefEnabled()) {
      return;
    }

    let url = aBrowser.currentURI.spec;
    let channel = aBrowser.docShell.currentDocumentChannel;
    let originalURL = channel.originalURI.spec;

    TaskUtils.spawn((function task() {
      let isSuccess = true;
      try {
        let blob = yield this.captureToBlob(aBrowser.contentWindow);
        let buffer = yield TaskUtils.readBlob(blob);
        yield this._store(originalURL, url, buffer);
      } catch (_) {
        isSuccess = false;
      }
      if (aCallback) {
        aCallback(isSuccess);
      }
    }).bind(this));
  },

  









  _store: function PageThumbs__store(aOriginalURL, aFinalURL, aData) {
    return TaskUtils.spawn(function () {
      let telemetryStoreTime = new Date();
      yield PageThumbsStorage.writeData(aFinalURL, aData);
      Services.telemetry.getHistogramById("FX_THUMBNAILS_STORE_TIME_MS")
        .add(new Date() - telemetryStoreTime);

      Services.obs.notifyObservers(null, "page-thumbnail:create", aFinalURL);
      
      
      
      
      
      
      
      
      
      
      
      if (aFinalURL != aOriginalURL) {
        yield PageThumbsStorage.copy(aFinalURL, aOriginalURL);
        Services.obs.notifyObservers(null, "page-thumbnail:create", aOriginalURL);
      }
    });
  },

  













  addExpirationFilter: function PageThumbs_addExpirationFilter(aFilter) {
    PageThumbsExpiration.addFilter(aFilter);
  },

  



  removeExpirationFilter: function PageThumbs_removeExpirationFilter(aFilter) {
    PageThumbsExpiration.removeFilter(aFilter);
  },

  





  _determineCropSize: function PageThumbs_determineCropSize(aWindow, aCanvas) {
    let utils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    let sbWidth = {}, sbHeight = {};

    try {
      utils.getScrollbarSize(false, sbWidth, sbHeight);
    } catch (e) {
      
      Cu.reportError("Unable to get scrollbar size in _determineCropSize.");
      sbWidth.value = sbHeight.value = 0;
    }

    
    
    let sw = aWindow.innerWidth - sbWidth.value;
    let sh = aWindow.innerHeight - sbHeight.value;

    let {width: thumbnailWidth, height: thumbnailHeight} = aCanvas;
    let scale = Math.min(Math.max(thumbnailWidth / sw, thumbnailHeight / sh), 1);
    let scaledWidth = sw * scale;
    let scaledHeight = sh * scale;

    if (scaledHeight > thumbnailHeight)
      sh -= Math.floor(Math.abs(scaledHeight - thumbnailHeight) * scale);

    if (scaledWidth > thumbnailWidth)
      sw -= Math.floor(Math.abs(scaledWidth - thumbnailWidth) * scale);

    return [sw, sh, scale];
  },

  





  _createCanvas: function PageThumbs_createCanvas(aWindow) {
    let doc = (aWindow || Services.appShell.hiddenDOMWindow).document;
    let canvas = doc.createElementNS(HTML_NAMESPACE, "canvas");
    canvas.mozOpaque = true;
    canvas.mozImageSmoothingEnabled = true;
    let [thumbnailWidth, thumbnailHeight] = this._getThumbnailSize();
    canvas.width = thumbnailWidth;
    canvas.height = thumbnailHeight;
    return canvas;
  },

  



  _getThumbnailSize: function PageThumbs_getThumbnailSize() {
    if (!this._thumbnailWidth || !this._thumbnailHeight) {
      let screenManager = Cc["@mozilla.org/gfx/screenmanager;1"]
                            .getService(Ci.nsIScreenManager);
      let left = {}, top = {}, width = {}, height = {};
      screenManager.primaryScreen.GetRect(left, top, width, height);
      this._thumbnailWidth = Math.round(width.value / 3);
      this._thumbnailHeight = Math.round(height.value / 3);
    }
    return [this._thumbnailWidth, this._thumbnailHeight];
  },

  _prefEnabled: function PageThumbs_prefEnabled() {
    try {
      return Services.prefs.getBoolPref("browser.pageThumbs.enabled");
    }
    catch (e) {
      return true;
    }
  },
};

this.PageThumbsStorage = {
  
  _path: null,
  get path() {
    if (!this._path) {
      this._path = OS.Path.join(OS.Constants.Path.localProfileDir, THUMBNAIL_DIRECTORY);
    }
    return this._path;
  },

  ensurePath: function Storage_ensurePath() {
    
    
    
    
    
    return PageThumbsWorker.post("makeDir",
      [this.path, {ignoreExisting: true}]).then(
        null,
        function onError(aReason) {
          Components.utils.reportError("Could not create thumbnails directory" + aReason);
        });
  },

  getLeafNameForURL: function Storage_getLeafNameForURL(aURL) {
    if (typeof aURL != "string") {
      throw new TypeError("Expecting a string");
    }
    let hash = this._calculateMD5Hash(aURL);
    return hash + ".png";
  },

  getFilePathForURL: function Storage_getFilePathForURL(aURL) {
    return OS.Path.join(this.path, this.getLeafNameForURL(aURL));
  },

  









  writeData: function Storage_writeData(aURL, aData) {
    let path = this.getFilePathForURL(aURL);
    this.ensurePath();
    aData = new Uint8Array(aData);
    let msg = [
      path,
      aData,
      {
        tmpPath: path + ".tmp",
        bytes: aData.byteLength,
        flush: false 
      }];
    return PageThumbsWorker.post("writeAtomic", msg,
      msg 

);
  },

  







  copy: function Storage_copy(aSourceURL, aTargetURL) {
    this.ensurePath();
    let sourceFile = this.getFilePathForURL(aSourceURL);
    let targetFile = this.getFilePathForURL(aTargetURL);
    return PageThumbsWorker.post("copy", [sourceFile, targetFile]);
  },

  




  remove: function Storage_remove(aURL) {
    return PageThumbsWorker.post("remove", [this.getFilePathForURL(aURL)]);
  },

  




  wipe: function Storage_wipe() {
    return PageThumbsWorker.post("wipe", [this.path]);
  },

  _calculateMD5Hash: function Storage_calculateMD5Hash(aValue) {
    let hash = gCryptoHash;
    let value = gUnicodeConverter.convertToByteArray(aValue);

    hash.init(hash.MD5);
    hash.update(value, value.length);
    return this._convertToHexString(hash.finish(false));
  },

  _convertToHexString: function Storage_convertToHexString(aData) {
    let hex = "";
    for (let i = 0; i < aData.length; i++)
      hex += ("0" + aData.charCodeAt(i).toString(16)).slice(-2);
    return hex;
  },

  
  getFileForURL: function Storage_getFileForURL_DEPRECATED(aURL) {
    Deprecated.warning("PageThumbs.getFileForURL is deprecated. Please use PageThumbs.getFilePathForURL and OS.File",
                       "https://developer.mozilla.org/docs/JavaScript_OS.File");
    
    return new FileUtils.File(PageThumbsStorage.getFilePathForURL(aURL));
  }
};

let PageThumbsStorageMigrator = {
  get currentVersion() {
    try {
      return Services.prefs.getIntPref(PREF_STORAGE_VERSION);
    } catch (e) {
      
      return 0;
    }
  },

  set currentVersion(aVersion) {
    Services.prefs.setIntPref(PREF_STORAGE_VERSION, aVersion);
  },

  migrate: function Migrator_migrate() {
    let version = this.currentVersion;

    
    
    
    
    

    
    
    
    

    if (version < 3) {
      this.migrateToVersion3();
    }

    this.currentVersion = LATEST_STORAGE_VERSION;
  },

  











  migrateToVersion3: function Migrator_migrateToVersion3(
    local = OS.Constants.Path.localProfileDir,
    roaming = OS.Constants.Path.profileDir) {
    PageThumbsWorker.post(
      "moveOrDeleteAllThumbnails",
      [OS.Path.join(roaming, THUMBNAIL_DIRECTORY),
       OS.Path.join(local, THUMBNAIL_DIRECTORY)]
    );
  }
};

let PageThumbsExpiration = {
  _filters: [],

  init: function Expiration_init() {
    gUpdateTimerManager.registerTimer("browser-cleanup-thumbnails", this,
                                      EXPIRATION_INTERVAL_SECS);
  },

  addFilter: function Expiration_addFilter(aFilter) {
    this._filters.push(aFilter);
  },

  removeFilter: function Expiration_removeFilter(aFilter) {
    let index = this._filters.indexOf(aFilter);
    if (index > -1)
      this._filters.splice(index, 1);
  },

  notify: function Expiration_notify(aTimer) {
    let urls = [];
    let filtersToWaitFor = this._filters.length;

    let expire = function expire() {
      this.expireThumbnails(urls);
    }.bind(this);

    
    if (!filtersToWaitFor) {
      expire();
      return;
    }

    function filterCallback(aURLs) {
      urls = urls.concat(aURLs);
      if (--filtersToWaitFor == 0)
        expire();
    }

    for (let filter of this._filters) {
      if (typeof filter == "function")
        filter(filterCallback)
      else
        filter.filterForThumbnailExpiration(filterCallback);
    }
  },

  expireThumbnails: function Expiration_expireThumbnails(aURLsToKeep) {
    let path = this.path;
    let keep = [PageThumbsStorage.getLeafNameForURL(url) for (url of aURLsToKeep)];
    let msg = [
      PageThumbsStorage.path,
      keep,
      EXPIRATION_MIN_CHUNK_SIZE
    ];

    return PageThumbsWorker.post(
      "expireFilesInDirectory",
      msg
    );
  }
};





let PageThumbsWorker = (function() {
  let worker = new PromiseWorker("resource://gre/modules/PageThumbsWorker.js",
    OS.Shared.LOG.bind("PageThumbs"));
  return {
    post: function post(...args) {
      let promise = worker.post.apply(worker, args);
      return promise.then(
        null,
        function onError(error) {
          
          if (error instanceof PromiseWorker.WorkerError) {
            throw OS.File.Error.fromMsg(error.data);
          } else {
            throw error;
          }
        }
      );
    }
  };
})();

let PageThumbsHistoryObserver = {
  onDeleteURI: function Thumbnails_onDeleteURI(aURI, aGUID) {
    PageThumbsStorage.remove(aURI.spec);
  },

  onClearHistory: function Thumbnails_onClearHistory() {
    PageThumbsStorage.wipe();
  },

  onTitleChanged: function () {},
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onVisit: function () {},
  onPageChanged: function () {},
  onDeleteVisits: function () {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
};
