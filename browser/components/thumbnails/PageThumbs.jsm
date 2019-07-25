



"use strict";

let EXPORTED_SYMBOLS = ["PageThumbs", "PageThumbsCache"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const HTML_NAMESPACE = "http://www.w3.org/1999/xhtml";




const THUMBNAIL_BG_COLOR = "#fff";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");





let PageThumbs = {

  


  _thumbnailWidth : 0,
  _thumbnailHeight : 0,

  


  get scheme() "moz-page-thumb",

  


  get staticHost() "thumbnail",

  


  get contentType() "image/png",

  




  getThumbnailURL: function PageThumbs_getThumbnailURL(aUrl) {
    return this.scheme + "://" + this.staticHost +
           "?url=" + encodeURIComponent(aUrl);
  },

  






  capture: function PageThumbs_capture(aWindow, aCallback) {
    let telemetryCaptureTime = new Date();
    let [sw, sh, scale] = this._determineCropSize(aWindow);

    let canvas = this._createCanvas();
    let ctx = canvas.getContext("2d");

    
    ctx.scale(scale, scale);

    try {
      
      ctx.drawWindow(aWindow, 0, 0, sw, sh, THUMBNAIL_BG_COLOR,
                     ctx.DRAWWINDOW_DO_NOT_FLUSH);
    } catch (e) {
      
    }

    let telemetry = Services.telemetry;
    telemetry.getHistogramById("FX_THUMBNAILS_CAPTURE_TIME_MS")
      .add(new Date() - telemetryCaptureTime);

    canvas.mozFetchAsStream(aCallback, this.contentType);
  },

  




  captureAndStore: function PageThumbs_captureAndStore(aBrowser, aCallback) {
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
            PageThumbsCache._copy(url, originalURL);
        }

        if (aCallback)
          aCallback(aSuccessful);
      }

      
      PageThumbsCache.getWriteEntry(url, function (aEntry) {
        if (!aEntry) {
          finish(false);
          return;
        }

        let outputStream = aEntry.openOutputStream(0);

        
        NetUtil.asyncCopy(aInputStream, outputStream, function (aResult) {
          let success = Components.isSuccessCode(aResult);
          if (success)
            aEntry.markValid();

          aEntry.close();
          finish(success);
        });
      });
    });
  },

  




  _determineCropSize: function PageThumbs_determineCropSize(aWindow) {
    let sw = aWindow.innerWidth;
    let sh = aWindow.innerHeight;

    let [thumbnailWidth, thumbnailHeight] = this._getThumbnailSize();
    let scale = Math.max(thumbnailWidth / sw, thumbnailHeight / sh);
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
  }
};




let PageThumbsCache = {
  




  getReadEntry: function Cache_getReadEntry(aKey, aCallback) {
    
    this._openCacheEntry(aKey, Ci.nsICache.ACCESS_READ, aCallback);
  },

  




  getWriteEntry: function Cache_getWriteEntry(aKey, aCallback) {
    
    this._openCacheEntry(aKey, Ci.nsICache.ACCESS_WRITE, aCallback);
  },

  




  _copy: function Cache_copy(aSourceKey, aTargetKey) {
    let sourceEntry, targetEntry, waitingCount = 2;

    function finish() {
      if (sourceEntry)
        sourceEntry.close();

      if (targetEntry)
        targetEntry.close();
    }

    function copyDataWhenReady() {
      if (--waitingCount > 0)
        return;

      if (!sourceEntry || !targetEntry) {
        finish();
        return;
      }

      let inputStream = sourceEntry.openInputStream(0);
      let outputStream = targetEntry.openOutputStream(0);

      
      NetUtil.asyncCopy(inputStream, outputStream, function (aResult) {
        if (Components.isSuccessCode(aResult))
          targetEntry.markValid();

        finish();
      });
    }

    this.getReadEntry(aSourceKey, function (aSourceEntry) {
      sourceEntry = aSourceEntry;
      copyDataWhenReady();
    });

    this.getWriteEntry(aTargetKey, function (aTargetEntry) {
      targetEntry = aTargetEntry;
      copyDataWhenReady();
    });
  },

  





  _openCacheEntry: function Cache_openCacheEntry(aKey, aAccess, aCallback) {
    function onCacheEntryAvailable(aEntry, aAccessGranted, aStatus) {
      let validAccess = aAccess == aAccessGranted;
      let validStatus = Components.isSuccessCode(aStatus);

      
      
      if (aEntry && !(validAccess && validStatus)) {
        aEntry.close();
        aEntry = null;
      }

      aCallback(aEntry);
    }

    let listener = this._createCacheListener(onCacheEntryAvailable);
    this._cacheSession.asyncOpenCacheEntry(aKey, aAccess, listener);
  },

  




  _createCacheListener: function Cache_createCacheListener(aCallback) {
    return {
      onCacheEntryAvailable: aCallback,
      QueryInterface: XPCOMUtils.generateQI([Ci.nsICacheListener])
    };
  }
};




XPCOMUtils.defineLazyGetter(PageThumbsCache, "_cacheSession", function () {
  return Services.cache.createSession(PageThumbs.scheme,
                                     Ci.nsICache.STORE_ON_DISK, true);
});
