



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

  




  capture: function PageThumbs_capture(aWindow) {
    let [sx, sy, sw, sh, scale] = this._determineCropRectangle(aWindow);

    let canvas = this._createCanvas();
    let ctx = canvas.getContext("2d");

    
    ctx.scale(scale, scale);

    try {
      
      ctx.drawWindow(aWindow, sx, sy, sw, sh, THUMBNAIL_BG_COLOR,
                     ctx.DRAWWINDOW_DO_NOT_FLUSH);
    } catch (e) {
      
    }

    return canvas;
  },

  







  store: function PageThumbs_store(aKey, aCanvas, aCallback) {
    let self = this;

    function finish(aSuccessful) {
      if (aCallback)
        aCallback(aSuccessful);
    }

    
    PageThumbsCache.getWriteEntry(aKey, function (aEntry) {
      if (!aEntry) {
        finish(false);
        return;
      }

      
      self._readImageData(aCanvas, function (aData) {
        let outputStream = aEntry.openOutputStream(0);

        
        NetUtil.asyncCopy(aData, outputStream, function (aResult) {
          let success = Components.isSuccessCode(aResult);
          if (success)
            aEntry.markValid();

          aEntry.close();
          finish(success);
        });
      });
    });
  },

  




  _readImageData: function PageThumbs_readImageData(aCanvas, aCallback) {
    let dataUri = aCanvas.toDataURL(PageThumbs.contentType, "");
    let uri = Services.io.newURI(dataUri, "UTF8", null);

    NetUtil.asyncFetch(uri, function (aData, aResult) {
      if (Components.isSuccessCode(aResult) && aData && aData.available())
        aCallback(aData);
    });
  },

  





  _determineCropRectangle: function PageThumbs_determineCropRectangle(aWindow) {
    let sx = 0;
    let sy = 0;
    let sw = aWindow.innerWidth;
    let sh = aWindow.innerHeight;

    let scale = Math.max(THUMBNAIL_WIDTH / sw, THUMBNAIL_HEIGHT / sh);
    let scaledWidth = sw * scale;
    let scaledHeight = sh * scale;

    if (scaledHeight > THUMBNAIL_HEIGHT) {
      sy = Math.floor(Math.abs((scaledHeight - THUMBNAIL_HEIGHT) / 2) / scale);
      sh -= 2 * sy;
    }

    if (scaledWidth > THUMBNAIL_WIDTH) {
      sx = Math.floor(Math.abs((scaledWidth - THUMBNAIL_WIDTH) / 2) / scale);
      sw -= 2 * sx;
    }

    return [sx, sy, sw, sh, scale];
  },

  



  _createCanvas: function PageThumbs_createCanvas() {
    let doc = Services.appShell.hiddenDOMWindow.document;
    let canvas = doc.createElementNS(HTML_NAMESPACE, "canvas");
    canvas.mozOpaque = true;
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
