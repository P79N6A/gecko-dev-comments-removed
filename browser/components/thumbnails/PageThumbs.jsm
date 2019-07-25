



"use strict";

let EXPORTED_SYMBOLS = ["PageThumbs", "PageThumbsCache"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const HTML_NAMESPACE = "http://www.w3.org/1999/xhtml";







const THUMBNAIL_WIDTH = 201;







const THUMBNAIL_HEIGHT = 127;




const THUMBNAIL_BG_COLOR = "#fff";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");





let PageThumbs = {
  


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
    this.capture(aBrowser.contentWindow, function (aInputStream) {
      let telemetryStoreTime = new Date();

      function finish(aSuccessful) {
        if (aSuccessful) {
          Services.telemetry.getHistogramById("FX_THUMBNAILS_STORE_TIME_MS")
            .add(new Date() - telemetryStoreTime);
        }

        if (aCallback)
          aCallback(aSuccessful);
      }

      
      PageThumbsCache.getWriteEntry(aBrowser.currentURI.spec, function (aEntry) {
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

    let scale = Math.max(THUMBNAIL_WIDTH / sw, THUMBNAIL_HEIGHT / sh);
    let scaledWidth = sw * scale;
    let scaledHeight = sh * scale;

    if (scaledHeight > THUMBNAIL_HEIGHT)
      sh -= Math.floor(Math.abs(scaledHeight - THUMBNAIL_HEIGHT) * scale);

    if (scaledWidth > THUMBNAIL_WIDTH)
      sw -= Math.floor(Math.abs(scaledWidth - THUMBNAIL_WIDTH) * scale);

    return [sw, sh, scale];
  },

  



  _createCanvas: function PageThumbs_createCanvas() {
    let doc = Services.appShell.hiddenDOMWindow.document;
    let canvas = doc.createElementNS(HTML_NAMESPACE, "canvas");
    canvas.mozOpaque = true;
    canvas.mozImageSmoothingEnabled = true;
    canvas.width = THUMBNAIL_WIDTH;
    canvas.height = THUMBNAIL_HEIGHT;
    return canvas;
  }
};




let PageThumbsCache = {
  




  getReadEntry: function Cache_getReadEntry(aKey, aCallback) {
    
    this._openCacheEntry(aKey, Ci.nsICache.ACCESS_READ, aCallback);
  },

  




  getWriteEntry: function Cache_getWriteEntry(aKey, aCallback) {
    
    this._openCacheEntry(aKey, Ci.nsICache.ACCESS_WRITE, aCallback);
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
