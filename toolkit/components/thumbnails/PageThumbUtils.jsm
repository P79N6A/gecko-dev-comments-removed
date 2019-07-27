








this.EXPORTED_SYMBOLS = ["PageThumbUtils"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm", this);

this.PageThumbUtils = {
  
  THUMBNAIL_BG_COLOR: "#fff",
  
  HTML_NAMESPACE: "http://www.w3.org/1999/xhtml",

  







  createCanvas: function (aWindow) {
    let doc = (aWindow || Services.appShell.hiddenDOMWindow).document;
    let canvas = doc.createElementNS(this.HTML_NAMESPACE, "canvas");
    canvas.mozOpaque = true;
    canvas.mozImageSmoothingEnabled = true;
    let [thumbnailWidth, thumbnailHeight] = this.getThumbnailSize();
    canvas.width = thumbnailWidth;
    canvas.height = thumbnailHeight;
    return canvas;
  },

  






  getThumbnailSize: function () {
    if (!this._thumbnailWidth || !this._thumbnailHeight) {
      let screenManager = Cc["@mozilla.org/gfx/screenmanager;1"]
                            .getService(Ci.nsIScreenManager);
      let left = {}, top = {}, width = {}, height = {};
      screenManager.primaryScreen.GetRectDisplayPix(left, top, width, height);
      this._thumbnailWidth = Math.round(width.value / 3);
      this._thumbnailHeight = Math.round(height.value / 3);
    }
    return [this._thumbnailWidth, this._thumbnailHeight];
  },

  







  determineCropSize: function (aWindow, aCanvas) {
    if (Cu.isCrossProcessWrapper(aWindow)) {
      throw new Error('Do not pass cpows here.');
    }
    let utils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    
    let sbWidth = {}, sbHeight = {};

    try {
      utils.getScrollbarSize(false, sbWidth, sbHeight);
    } catch (e) {
      
      Cu.reportError("Unable to get scrollbar size in determineCropSize.");
      sbWidth.value = sbHeight.value = 0;
    }

    
    
    let width = aWindow.innerWidth - sbWidth.value;
    let height = aWindow.innerHeight - sbHeight.value;

    let {width: thumbnailWidth, height: thumbnailHeight} = aCanvas;
    let scale = Math.min(Math.max(thumbnailWidth / width, thumbnailHeight / height), 1);
    let scaledWidth = width * scale;
    let scaledHeight = height * scale;

    if (scaledHeight > thumbnailHeight)
      height -= Math.floor(Math.abs(scaledHeight - thumbnailHeight) * scale);

    if (scaledWidth > thumbnailWidth)
      width -= Math.floor(Math.abs(scaledWidth - thumbnailWidth) * scale);

    return [width, height, scale];
  },

  shouldStoreContentThumbnail: function (aDocument, aDocShell) {
    
    
    if (aDocument instanceof Ci.nsIDOMXMLDocument) {
      return false;
    }

    let webNav = aDocShell.QueryInterface(Ci.nsIWebNavigation);

    
    if (webNav.currentURI.schemeIs("about")) {
      return false;
    }

    
    if (aDocShell.busyFlags != Ci.nsIDocShell.BUSY_FLAGS_NONE) {
      return false;
    }

    let channel = aDocShell.currentDocumentChannel;

    
    if (!channel) {
      return false;
    }

    
    
    let uri = channel.originalURI;
    if (uri.schemeIs("about")) {
      return false;
    }

    let httpChannel;
    try {
      httpChannel = channel.QueryInterface(Ci.nsIHttpChannel);
    } catch (e) {  }

    if (httpChannel) {
      
      try {
        if (Math.floor(httpChannel.responseStatus / 100) != 2) {
        return false;
        }
      } catch (e) {
        
        
        return false;
      }

      
      if (httpChannel.isNoStoreResponse()) {
        return false;
      }

      
      if (uri.schemeIs("https") &&
          !Services.prefs.getBoolPref("browser.cache.disk_cache_ssl")) {
        return false;
      }
    } 
    return true;
  }
};
