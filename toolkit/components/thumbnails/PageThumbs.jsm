



"use strict";

this.EXPORTED_SYMBOLS = ["PageThumbs", "PageThumbsStorage"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const PREF_STORAGE_VERSION = "browser.pagethumbnails.storage_version";
const LATEST_STORAGE_VERSION = 3;

const EXPIRATION_MIN_CHUNK_SIZE = 50;
const EXPIRATION_INTERVAL_SECS = 3600;

var gRemoteThumbId = 0;




const MAX_THUMBNAIL_AGE_SECS = 172800; 




const THUMBNAIL_DIRECTORY = "thumbnails";

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/PromiseWorker.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
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
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PageThumbUtils",
  "resource://gre/modules/PageThumbUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");




const TaskUtils = {
  






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

   







   getThumbnailPath: function PageThumbs_getThumbnailPath(aUrl) {
     return PageThumbsStorage.getFilePathForURL(aUrl);
   },

  







  captureToBlob: function PageThumbs_captureToBlob(aBrowser) {
    if (!this._prefEnabled()) {
      return null;
    }

    let deferred = Promise.defer();

    let canvas = this.createCanvas();
    this.captureToCanvas(aBrowser, canvas, () => {
      canvas.toBlob(blob => {
        deferred.resolve(blob, this.contentType);
      });
    });

    return deferred.promise;
  },

  









  captureToCanvas: function PageThumbs_captureToCanvas(aBrowser, aCanvas, aCallback) {
    let telemetryCaptureTime = new Date();
    this._captureToCanvas(aBrowser, aCanvas, function () {
      Services.telemetry
              .getHistogramById("FX_THUMBNAILS_CAPTURE_TIME_MS")
              .add(new Date() - telemetryCaptureTime);
      if (aCallback) {
        aCallback(aCanvas);
      }
    });
  },

  











  shouldStoreThumbnail: function (aBrowser, aCallback) {
    
    if (PrivateBrowsingUtils.isBrowserPrivate(aBrowser)) {
      aCallback(false);
      return;
    }
    if (aBrowser.isRemoteBrowser) {
      let mm = aBrowser.messageManager;
      let resultFunc = function (aMsg) {
        mm.removeMessageListener("Browser:Thumbnail:CheckState:Response", resultFunc);
        aCallback(aMsg.data.result);
      }
      mm.addMessageListener("Browser:Thumbnail:CheckState:Response", resultFunc);
      mm.sendAsyncMessage("Browser:Thumbnail:CheckState");
    } else {
      aCallback(PageThumbUtils.shouldStoreContentThumbnail(aBrowser.contentDocument,
                                                           aBrowser.docShell));
    }
  },

  
  
  _captureToCanvas: function (aBrowser, aCanvas, aCallback) {
    if (aBrowser.isRemoteBrowser) {
      Task.spawn(function () {
        let data =
          yield this._captureRemoteThumbnail(aBrowser, aCanvas);
        let canvas = data.thumbnail;
        let ctx = canvas.getContext("2d");
        let imgData = ctx.getImageData(0, 0, canvas.width, canvas.height);
        aCanvas.getContext("2d").putImageData(imgData, 0, 0);
        if (aCallback) {
          aCallback(aCanvas);
        }
      }.bind(this));
      return;
    }

    
    let [width, height, scale] =
      PageThumbUtils.determineCropSize(aBrowser.contentWindow, aCanvas);
    let ctx = aCanvas.getContext("2d");

    
    ctx.save();
    ctx.scale(scale, scale);

    try {
      
      ctx.drawWindow(aBrowser.contentWindow, 0, 0, width, height,
                     PageThumbUtils.THUMBNAIL_BG_COLOR,
                     ctx.DRAWWINDOW_DO_NOT_FLUSH);
    } catch (e) {
      
    }
    ctx.restore();

    if (aCallback) {
      aCallback(aCanvas);
    }
  },

  






  _captureRemoteThumbnail: function (aBrowser, aCanvas) {
    let deferred = Promise.defer();

    
    
    let index = gRemoteThumbId++;

    
    let mm = aBrowser.messageManager;

    
    let thumbFunc = function (aMsg) {
      
      if (aMsg.data.id != index) {
        return;
      }

      mm.removeMessageListener("Browser:Thumbnail:Response", thumbFunc);
      let imageBlob = aMsg.data.thumbnail;
      let doc = aBrowser.parentElement.ownerDocument;
      let reader = Cc["@mozilla.org/files/filereader;1"].
                   createInstance(Ci.nsIDOMFileReader);
      reader.addEventListener("loadend", function() {
        let image = doc.createElementNS(PageThumbUtils.HTML_NAMESPACE, "img");
        image.onload = function () {
          let thumbnail = doc.createElementNS(PageThumbUtils.HTML_NAMESPACE, "canvas");
          thumbnail.width = image.naturalWidth;
          thumbnail.height = image.naturalHeight;
          let ctx = thumbnail.getContext("2d");
          ctx.drawImage(image, 0, 0);
          deferred.resolve({
            thumbnail: thumbnail
          });
        }
        image.src = reader.result;
      });
      
      reader.readAsDataURL(imageBlob);
    }

    
    mm.addMessageListener("Browser:Thumbnail:Response", thumbFunc);
    mm.sendAsyncMessage("Browser:Thumbnail:Request", {
      canvasWidth: aCanvas.width,
      canvasHeight: aCanvas.height,
      background: PageThumbUtils.THUMBNAIL_BG_COLOR,
      id: index
    });

    return deferred.promise;
  },

  




  captureAndStore: function PageThumbs_captureAndStore(aBrowser, aCallback) {
    if (!this._prefEnabled()) {
      return;
    }

    let url = aBrowser.currentURI.spec;
    let originalURL;
    let channelError = false;

    if (!aBrowser.isRemoteBrowser) {
      let channel = aBrowser.docShell.currentDocumentChannel;
      originalURL = channel.originalURI.spec;
      
      channelError = this._isChannelErrorResponse(channel);
    } else {
      
      originalURL = url;
    }

    Task.spawn((function task() {
      let isSuccess = true;
      try {
        let blob = yield this.captureToBlob(aBrowser);
        let buffer = yield TaskUtils.readBlob(blob);
        yield this._store(originalURL, url, buffer, channelError);
      } catch (ex) {
        Components.utils.reportError("Exception thrown during thumbnail capture: '" + ex + "'");
        isSuccess = false;
      }
      if (aCallback) {
        aCallback(isSuccess);
      }
    }).bind(this));
  },

  








  captureAndStoreIfStale: function PageThumbs_captureAndStoreIfStale(aBrowser, aCallback) {
    let url = aBrowser.currentURI.spec;
    PageThumbsStorage.isFileRecentForURL(url).then(recent => {
      if (!recent &&
          
          
          aBrowser.currentURI &&
          aBrowser.currentURI.spec == url) {
        this.captureAndStore(aBrowser, aCallback);
      } else if (aCallback) {
        aCallback(true);
      }
    }, err => {
      if (aCallback)
        aCallback(false);
    });
  },

  











  _store: function PageThumbs__store(aOriginalURL, aFinalURL, aData, aNoOverwrite) {
    return Task.spawn(function () {
      let telemetryStoreTime = new Date();
      yield PageThumbsStorage.writeData(aFinalURL, aData, aNoOverwrite);
      Services.telemetry.getHistogramById("FX_THUMBNAILS_STORE_TIME_MS")
        .add(new Date() - telemetryStoreTime);

      Services.obs.notifyObservers(null, "page-thumbnail:create", aFinalURL);
      
      
      
      
      
      
      
      
      
      
      
      if (aFinalURL != aOriginalURL) {
        yield PageThumbsStorage.copy(aFinalURL, aOriginalURL, aNoOverwrite);
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

  





  createCanvas: function PageThumbs_createCanvas(aWindow) {
    return PageThumbUtils.createCanvas(aWindow);
  },

  



  _isChannelErrorResponse: function(channel) {
    
    if (!channel)
      return true;
    if (!(channel instanceof Ci.nsIHttpChannel))
      
      return false;
    try {
      return !channel.requestSucceeded;
    } catch (_) {
      
      return true;
    }
  },

  _prefEnabled: function PageThumbs_prefEnabled() {
    try {
      return !Services.prefs.getBoolPref("browser.pagethumbnails.capturing_disabled");
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

  











  writeData: function Storage_writeData(aURL, aData, aNoOverwrite) {
    let path = this.getFilePathForURL(aURL);
    this.ensurePath();
    aData = new Uint8Array(aData);
    let msg = [
      path,
      aData,
      {
        tmpPath: path + ".tmp",
        bytes: aData.byteLength,
        noOverwrite: aNoOverwrite,
        flush: false 
      }];
    return PageThumbsWorker.post("writeAtomic", msg,
      msg 

).
      then(null, this._eatNoOverwriteError(aNoOverwrite));
  },

  









  copy: function Storage_copy(aSourceURL, aTargetURL, aNoOverwrite) {
    this.ensurePath();
    let sourceFile = this.getFilePathForURL(aSourceURL);
    let targetFile = this.getFilePathForURL(aTargetURL);
    let options = { noOverwrite: aNoOverwrite };
    return PageThumbsWorker.post("copy", [sourceFile, targetFile, options]).
      then(null, this._eatNoOverwriteError(aNoOverwrite));
  },

  




  remove: function Storage_remove(aURL) {
    return PageThumbsWorker.post("remove", [this.getFilePathForURL(aURL)]);
  },

  




  wipe: Task.async(function* Storage_wipe() {
    
    
    
    
    
    
    
    
    
    

    let blocker = () => promise;

    
    
    
    AsyncShutdown.profileBeforeChange.addBlocker(
      "PageThumbs: removing all thumbnails",
      blocker);

    
    

    let promise = PageThumbsWorker.post("wipe", [this.path]);
    try {
      yield promise;
    }  finally {
       
       
       if ("removeBlocker" in AsyncShutdown.profileBeforeChange) {
         
         
         
         AsyncShutdown.profileBeforeChange.removeBlocker(blocker);
       }
    }
  }),

  fileExistsForURL: function Storage_fileExistsForURL(aURL) {
    return PageThumbsWorker.post("exists", [this.getFilePathForURL(aURL)]);
  },

  isFileRecentForURL: function Storage_isFileRecentForURL(aURL) {
    return PageThumbsWorker.post("isFileRecent",
                                 [this.getFilePathForURL(aURL),
                                  MAX_THUMBNAIL_AGE_SECS]);
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

  









  _eatNoOverwriteError: function Storage__eatNoOverwriteError(aNoOverwrite) {
    return function onError(err) {
      if (!aNoOverwrite ||
          !(err instanceof OS.File.Error) ||
          !err.becauseExists) {
        throw err;
      }
    };
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




let PageThumbsWorker = new BasePromiseWorker("resource://gre/modules/PageThumbsWorker.js");


PageThumbsWorker.ExceptionHandlers["OS.File.Error"] = OS.File.Error.fromMsg;

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
