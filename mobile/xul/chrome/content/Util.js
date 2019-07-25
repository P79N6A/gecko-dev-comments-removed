












































let Util = {
  
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

  getWindowUtils: function getWindowUtils(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
  },

  
  executeSoon: function executeSoon(aFunc) {
    Services.tm.mainThread.dispatch({
      run: function() {
        aFunc();
      }
    }, Ci.nsIThread.DISPATCH_NORMAL);
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

  makeURI: function makeURI(aURL, aOriginCharset, aBaseURI) {
    return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
  },

  makeURLAbsolute: function makeURLAbsolute(base, url) {
    
    return this.makeURI(url, null, this.makeURI(base)).spec;
  },

  isLocalScheme: function isLocalScheme(aURL) {
    return (aURL.indexOf("about:") == 0 && aURL != "about:blank" && aURL != "about:empty") || aURL.indexOf("chrome:") == 0;
  },

  isOpenableScheme: function isShareableScheme(aProtocol) {
    let dontOpen = /^(mailto|javascript|news|snews)$/;
    return (aProtocol && !dontOpen.test(aProtocol));
  },

  isShareableScheme: function isShareableScheme(aProtocol) {
    let dontShare = /^(chrome|about|file|javascript|resource)$/;
    return (aProtocol && !dontShare.test(aProtocol));
  },

  clamp: function(num, min, max) {
    return Math.max(min, Math.min(max, num));
  },

  
  isURLEmpty: function isURLEmpty(aURL) {
    return (!aURL || aURL == "about:blank" || aURL == "about:empty" || aURL == "about:home");
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

  isParentProcess: function isInParentProcess() {
    let appInfo = Cc["@mozilla.org/xre/app-info;1"];
    return (!appInfo || appInfo.getService(Ci.nsIXULRuntime).processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT);
  },

  isTablet: function isTablet(options) {
    let forceUpdate = options && 'forceUpdate' in options && options.forceUpdate;

    if ('_isTablet' in this && !forceUpdate)
      return this._isTablet;

    let tabletPref = Services.prefs.getIntPref("browser.ui.layout.tablet");

    
    
    if (tabletPref == 0)
      return this._isTablet = false;
    else if (tabletPref == 1)
      return this._isTablet = true;

#ifdef ANDROID
    
    let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
    let shellVersion = sysInfo.get("shellVersion") || "";
    let matches = shellVersion.match(/\((\d+)\)$/);
    if (matches) {
      let sdkVersion = parseInt(matches[1]);
      if (sdkVersion < 11 || sdkVersion > 13)
        return this._isTablet = false;
    }
#endif

    let dpi = this.displayDPI;
    if (dpi <= 96)
      return this._isTablet = (window.innerWidth > 1024);

    
    let tablet_panel_minwidth = 124;
    let dpmm = 25.4 * window.innerWidth / dpi;
    return this._isTablet = (dpmm >= tablet_panel_minwidth);
  },

  isPortrait: function isPortrait() {
#ifdef MOZ_PLATFORM_MAEMO
    return (screen.width <= screen.height);
#elifdef ANDROID
    return (screen.width <= screen.height);
#else
    return (window.innerWidth <= window.innerHeight);
#endif
  },

  modifierMaskFromEvent: function modifierMaskFromEvent(aEvent) {
    return (aEvent.altKey   ? Ci.nsIDOMNSEvent.ALT_MASK     : 0) |
           (aEvent.ctrlKey  ? Ci.nsIDOMNSEvent.CONTROL_MASK : 0) |
           (aEvent.shiftKey ? Ci.nsIDOMNSEvent.SHIFT_MASK   : 0) |
           (aEvent.metaKey  ? Ci.nsIDOMNSEvent.META_MASK    : 0);
  },

  get displayDPI() {
    delete this.displayDPI;
    return this.displayDPI = this.getWindowUtils(window).displayDPI;
  },

  LOCALE_DIR_RTL: -1,
  LOCALE_DIR_LTR: 1,
  get localeDir() {
    
    let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry);
    return chromeReg.isLocaleRTL("global") ? this.LOCALE_DIR_RTL : this.LOCALE_DIR_LTR;
  },

  createShortcut: function Util_createShortcut(aTitle, aURL, aIconURL, aType) {
    
    
    const kIconSize = 72;
    const kOverlaySize = 32;
    const kOffset = 20;

    
    aTitle = aTitle || aURL;

    let canvas = document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    canvas.setAttribute("style", "display: none");

    function _createShortcut() {
      let icon = canvas.toDataURL("image/png", "");
      canvas = null;
      try {
        let shell = Cc["@mozilla.org/browser/shell-service;1"].createInstance(Ci.nsIShellService);
        shell.createShortcut(aTitle, aURL, icon, aType);
      } catch(e) {
        Cu.reportError(e);
      }
    }

    
    let image = new Image();
    image.onload = function() {
      canvas.width = canvas.height = kIconSize;
      let ctx = canvas.getContext("2d");
      ctx.drawImage(image, 0, 0, kIconSize, kIconSize);

      
      if (aIconURL) {
        let favicon = new Image();
        favicon.onload = function() {
          
          ctx.drawImage(favicon, kOffset, kOffset, kOverlaySize, kOverlaySize);
          _createShortcut();
        }

        favicon.onerror = function() {
          Cu.reportError("CreateShortcut: favicon image load error");
        }

        favicon.src = aIconURL;
      } else {
        _createShortcut();
      }
    }

    image.onerror = function() {
      Cu.reportError("CreateShortcut: background image load error");
    }

    
    image.src = aIconURL ? "chrome://browser/skin/images/homescreen-blank-hdpi.png"
                         : "chrome://browser/skin/images/homescreen-default-hdpi.png";
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

