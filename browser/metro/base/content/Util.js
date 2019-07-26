



let Util = {
  



  getWindowUtils: function getWindowUtils(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
  },

  
  getAllDocuments: function getAllDocuments(doc, resultSoFar) {
    resultSoFar = resultSoFar || [doc];
    if (!doc.defaultView)
      return resultSoFar;
    let frames = doc.defaultView.frames;
    if (!frames)
      return resultSoFar;

    let i;
    let currentDoc;
    for (i = 0; i < frames.length; i++) {
      currentDoc = frames[i].document;
      resultSoFar.push(currentDoc);
      this.getAllDocuments(currentDoc, resultSoFar);
    }

    return resultSoFar;
  },

  
  
  forceOnline: function forceOnline() {
    Services.io.offline = false;
  },

  



  
  executeSoon: function executeSoon(aFunc) {
    Services.tm.mainThread.dispatch({
      run: function() {
        aFunc();
      }
    }, Ci.nsIThread.DISPATCH_NORMAL);
  },

  



  dumpf: function dumpf(str) {
    let args = arguments;
    let i = 1;
    dump(str.replace(/%s/g, function() {
      if (i >= args.length) {
        throw "dumps received too many placeholders and not enough arguments";
      }
      return args[i++].toString();
    }));
  },

  
  dumpLn: function dumpLn() {
    for (let i = 0; i < arguments.length; i++)
      dump(arguments[i] + " ");
    dump("\n");
  },

  dumpElement: function dumpElement(aElement) {
    this.dumpLn(aElement.id);
  },

  dumpElementTree: function dumpElementTree(aElement) {
    let node = aElement;
    while (node) {
      this.dumpLn("node:", node, "id:", node.id, "class:", node.classList);
      node = node.parentNode;
    }
  },

  



  highlightElement: function highlightElement(aElement) {
    if (aElement == null) {
      this.dumpLn("aElement is null");
      return;
    }
    aElement.style.border = "2px solid red";
  },

  getHrefForElement: function getHrefForElement(target) {
    let link = null;
    while (target) {
      if (target instanceof Ci.nsIDOMHTMLAnchorElement || 
          target instanceof Ci.nsIDOMHTMLAreaElement ||
          target instanceof Ci.nsIDOMHTMLLinkElement) {
          if (target.hasAttribute("href"))
            link = target;
      }
      target = target.parentNode;
    }

    if (link && link.hasAttribute("href"))
      return link.href;
    else
      return null;
  },

  



  pointWithinRect: function pointWithinRect(aX, aY, aRect) {
    return (aRect.left < aX && aRect.top < aY &&
            aRect.right > aX && aRect.bottom > aY);
  },

  pointWithinDOMRect: function pointWithinDOMRect(aX, aY, aRect) {
    if (!aRect.width || !aRect.height)
      return false;
    return this.pointWithinRect(aX, aY, aRect);
  },

  isEmptyDOMRect: function isEmptyDOMRect(aRect) {
    if ((aRect.bottom - aRect.top) <= 0 &&
        (aRect.right - aRect.left) <= 0)
      return true;
    return false;
  },

  
  dumpDOMRect: function dumpDOMRect(aMsg, aRect) {
    try {
      Util.dumpLn(aMsg,
                  "left:" + Math.round(aRect.left) + ",",
                  "top:" + Math.round(aRect.top) + ",",
                  "right:" + Math.round(aRect.right) + ",",
                  "bottom:" + Math.round(aRect.bottom) + ",",
                  "width:" + Math.round(aRect.right - aRect.left) + ",",
                  "height:" + Math.round(aRect.bottom - aRect.top) );
    } catch (ex) {
      Util.dumpLn("dumpDOMRect:", ex.message);
    }
  },

  



  makeURI: function makeURI(aURL, aOriginCharset, aBaseURI) {
    return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
  },

  makeURLAbsolute: function makeURLAbsolute(base, url) {
    
    return this.makeURI(url, null, this.makeURI(base)).spec;
  },

  isLocalScheme: function isLocalScheme(aURL) {
    return ((aURL.indexOf("about:") == 0 &&
             aURL != "about:blank" &&
             aURL != "about:empty" &&
             aURL != "about:start") ||
            aURL.indexOf("chrome:") == 0);
  },

  isOpenableScheme: function isShareableScheme(aProtocol) {
    let dontOpen = /^(mailto|javascript|news|snews)$/;
    return (aProtocol && !dontOpen.test(aProtocol));
  },

  isShareableScheme: function isShareableScheme(aProtocol) {
    let dontShare = /^(chrome|about|file|javascript|resource)$/;
    return (aProtocol && !dontShare.test(aProtocol));
  },

  
  isURLEmpty: function isURLEmpty(aURL) {
    return (!aURL ||
            aURL == "about:blank" ||
            aURL == "about:empty" ||
            aURL == "about:home" ||
            aURL == "about:start");
  },

  
  isURLMemorable: function isURLMemorable(aURL) {
    return !(aURL == "about:blank" ||
             aURL == "about:empty" ||
             aURL == "about:start");
  },

  



  clamp: function(num, min, max) {
    return Math.max(min, Math.min(max, num));
  },

  



  get displayDPI() {
    delete this.displayDPI;
    return this.displayDPI = this.getWindowUtils(window).displayDPI;
  },

  isPortrait: function isPortrait() {
    return (window.innerWidth <= window.innerHeight);
  },

  LOCALE_DIR_RTL: -1,
  LOCALE_DIR_LTR: 1,
  get localeDir() {
    
    let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry);
    return chromeReg.isLocaleRTL("global") ? this.LOCALE_DIR_RTL : this.LOCALE_DIR_LTR;
  },

  



  isParentProcess: function isInParentProcess() {
    let appInfo = Cc["@mozilla.org/xre/app-info;1"];
    return (!appInfo || appInfo.getService(Ci.nsIXULRuntime).processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT);
  },

  



  modifierMaskFromEvent: function modifierMaskFromEvent(aEvent) {
    return (aEvent.altKey   ? Ci.nsIDOMEvent.ALT_MASK     : 0) |
           (aEvent.ctrlKey  ? Ci.nsIDOMEvent.CONTROL_MASK : 0) |
           (aEvent.shiftKey ? Ci.nsIDOMEvent.SHIFT_MASK   : 0) |
           (aEvent.metaKey  ? Ci.nsIDOMEvent.META_MASK    : 0);
  },

  



  insertDownload: function insertDownload(aSrcUri, aFile) {
    let dm = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    let db = dm.DBConnection;

    let stmt = db.createStatement(
      "INSERT INTO moz_downloads (name, source, target, startTime, endTime, state, referrer) " +
      "VALUES (:name, :source, :target, :startTime, :endTime, :state, :referrer)"
    );

    stmt.params.name = aFile.leafName;
    stmt.params.source = aSrcUri.spec;
    stmt.params.target = aFile.path;
    stmt.params.startTime = Date.now() * 1000;
    stmt.params.endTime = Date.now() * 1000;
    stmt.params.state = Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED;
    stmt.params.referrer = aSrcUri.spec;

    stmt.execute();
    stmt.finalize();

    let newItemId = db.lastInsertRowID;
    let download = dm.getDownload(newItemId);
    
    
  },

  



  copyImageToClipboard: function Util_copyImageToClipboard(aImageLoadingContent) {
    let image = aImageLoadingContent.QueryInterface(Ci.nsIImageLoadingContent);
    if (!image) {
      Util.dumpLn("copyImageToClipboard error: image is not an nsIImageLoadingContent");
      return;
    }
    try {
      let xferable = Cc["@mozilla.org/widget/transferable;1"].createInstance(Ci.nsITransferable);
      xferable.init(null);
      let imgRequest = aImageLoadingContent.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
      let mimeType = imgRequest.mimeType;
      let imgContainer = imgRequest.image;
      let imgPtr = Cc["@mozilla.org/supports-interface-pointer;1"].createInstance(Ci.nsISupportsInterfacePointer);
      imgPtr.data = imgContainer;
      xferable.setTransferData(mimeType, imgPtr, null);
      let clip = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
      clip.setData(xferable, null, Ci.nsIClipboard.kGlobalClipboard);
    } catch (e) {
      Util.dumpLn(e.message);
    }
  },
};








Util.Timeout = function(aCallback) {
  this._callback = aCallback;
  this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._type = null;
};

Util.Timeout.prototype = {
  
  notify: function notify() {
    if (this._type == this._timer.TYPE_ONE_SHOT)
      this._type = null;

    if (this._callback.notify)
      this._callback.notify();
    else
      this._callback.apply(null);
  },

  
  _start: function _start(aDelay, aType, aCallback) {
    if (aCallback)
      this._callback = aCallback;
    this.clear();
    this._timer.initWithCallback(this, aDelay, aType);
    this._type = aType;
    return this;
  },

  
  once: function once(aDelay, aCallback) {
    return this._start(aDelay, this._timer.TYPE_ONE_SHOT, aCallback);
  },

  
  interval: function interval(aDelay, aCallback) {
    return this._start(aDelay, this._timer.TYPE_REPEATING_SLACK, aCallback);
  },

  
  clear: function clear() {
    if (this.isPending()) {
      this._timer.cancel();
      this._type = null;
    }
    return this;
  },

  
  flush: function flush() {
    if (this.isPending()) {
      this.notify();
      this.clear();
    }
    return this;
  },

  
  isPending: function isPending() {
    return this._type !== null;
  }
};

