



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




const THUMBNAIL_DIRECTORY = "thumbnails";




const THUMBNAIL_BG_COLOR = "#fff";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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

  




  captureToCanvas: function PageThumbs_captureToCanvas(aWindow, aCanvas) {
    let telemetryCaptureTime = new Date();
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

    let telemetry = Services.telemetry;
    telemetry.getHistogramById("FX_THUMBNAILS_CAPTURE_TIME_MS")
      .add(new Date() - telemetryCaptureTime);
  },

  




  captureAndStore: function PageThumbs_captureAndStore(aBrowser, aCallback) {
    if (!this._prefEnabled()) {
      return;
    }

    let url = aBrowser.currentURI.spec;
    let channel = aBrowser.docShell.currentDocumentChannel;
    let originalURL = channel.originalURI.spec;

    this.capture(aBrowser.contentWindow, function (aInputStream) {
      let telemetryStoreTime = new Date();

      function finish(aSuccessful) {
        if (aSuccessful) {
          Services.telemetry.getHistogramById("FX_THUMBNAILS_STORE_TIME_MS")
            .add(new Date() - telemetryStoreTime);

          
          
          
          
          
          
          
          
          
          
          
          if (url != originalURL)
            PageThumbsStorage.copy(url, originalURL);
        }

        if (aCallback)
          aCallback(aSuccessful);
      }

      PageThumbsStorage.write(url, aInputStream, finish);
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

  



  _createCanvas: function PageThumbs_createCanvas() {
    let doc = Services.appShell.hiddenDOMWindow.document;
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
  getDirectory: function Storage_getDirectory(aCreate = true) {
    return FileUtils.getDir("ProfLD", [THUMBNAIL_DIRECTORY], aCreate);
  },

  getLeafNameForURL: function Storage_getLeafNameForURL(aURL) {
    let hash = this._calculateMD5Hash(aURL);
    return hash + ".png";
  },

  getFileForURL: function Storage_getFileForURL(aURL) {
    let file = this.getDirectory();
    file.append(this.getLeafNameForURL(aURL));
    return file;
  },

  write: function Storage_write(aURL, aDataStream, aCallback) {
    let file = this.getFileForURL(aURL);
    let fos = FileUtils.openSafeFileOutputStream(file);

    NetUtil.asyncCopy(aDataStream, fos, function (aResult) {
      FileUtils.closeSafeFileOutputStream(fos);
      aCallback(Components.isSuccessCode(aResult));
    });
  },

  copy: function Storage_copy(aSourceURL, aTargetURL) {
    let sourceFile = this.getFileForURL(aSourceURL);
    let targetFile = this.getFileForURL(aTargetURL);

    try {
      sourceFile.copyTo(targetFile.parent, targetFile.leafName);
    } catch (e) {
      
    }
  },

  remove: function Storage_remove(aURL) {
    let file = this.getFileForURL(aURL);
    PageThumbsWorker.postMessage({type: "removeFile", path: file.path});
  },

  wipe: function Storage_wipe() {
    let dir = this.getDirectory(false);
    dir.followLinks = false;
    try {
      dir.remove(true);
    } catch (e) {
      
    }
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

  






  migrateToVersion3: function Migrator_migrateToVersion3() {
    let local = FileUtils.getDir("ProfLD", [THUMBNAIL_DIRECTORY], true);
    let roaming = FileUtils.getDir("ProfD", [THUMBNAIL_DIRECTORY]);

    if (!roaming.equals(local)) {
      PageThumbsWorker.postMessage({
        type: "moveOrDeleteAllThumbnails",
        from: roaming.path,
        to: local.path
      });
    }
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

  expireThumbnails: function Expiration_expireThumbnails(aURLsToKeep, aCallback) {
    PageThumbsWorker.postMessage({
      type: "expireFilesInDirectory",
      minChunkSize: EXPIRATION_MIN_CHUNK_SIZE,
      path: PageThumbsStorage.getDirectory().path,
      filesToKeep: [PageThumbsStorage.getLeafNameForURL(url) for (url of aURLsToKeep)]
    }, aCallback);
  }
};




let PageThumbsWorker = {
  



  _callbacks: [],

  



  get _worker() {
    delete this._worker;
    this._worker = new ChromeWorker("resource://gre/modules/PageThumbsWorker.js");
    this._worker.addEventListener("message", this);
    return this._worker;
  },

  








  postMessage: function Worker_postMessage(message, callback) {
    this._callbacks.push(callback);
    this._worker.postMessage(message);
  },

  


  handleEvent: function Worker_handleEvent(aEvent) {
    let callback = this._callbacks.shift();
    if (callback)
      callback(aEvent.data);
  }
};

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
