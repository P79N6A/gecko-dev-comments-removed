



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";
const BROWSER_FRAMES_ENABLED_PREF = "dom.mozBrowserFramesEnabled";










function BrowserElementAPI() {}
BrowserElementAPI.prototype = {
  classID: Components.ID("{5d6fcab3-6c12-4db6-80fb-352df7a41602}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  





  _chromeEventHandlersWatching: new WeakMap(),

  





  _topLevelBrowserWindows: new WeakMap(),

  _browserFramesPrefEnabled: function BA_browserFramesPrefEnabled() {
    var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    try {
      return prefs.getBoolPref(BROWSER_FRAMES_ENABLED_PREF);
    }
    catch(e) {
      return false;
    }
  },

  



  _init: function BA_init() {
    if (this._initialized) {
      return;
    }

    
    
    
    if (!this._browserFramesPrefEnabled()) {
      var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
      prefs.addObserver(BROWSER_FRAMES_ENABLED_PREF, this,  true);
      return;
    }

    this._initialized = true;
    this._progressListener._browserElementAPI = this;

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.addObserver(this, 'content-document-global-created',   true);
    os.addObserver(this, 'docshell-marked-as-browser-frame',  true);
  },

  







  _observeDocshellMarkedAsBrowserFrame: function BA_observeDocshellMarkedAsBrowserFrame(docshell) {
    docshell.QueryInterface(Ci.nsIWebProgress)
            .addProgressListener(this._progressListener,
                                 Ci.nsIWebProgress.NOTIFY_LOCATION |
                                 Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);
  },

  



  _observeContentGlobalCreated: function BA_observeContentGlobalCreated(win) {
    var docshell = win.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIWebNavigation)
                      .QueryInterface(Ci.nsIDocShell);

    
    
    if (!docshell.containedInBrowserFrame) {
      return;
    }

    this._initBrowserWindow(win, docshell.isBrowserFrame);

    
    
    if (docshell.isBrowserFrame) {
      this._topLevelBrowserWindows.set(win, true);
      this._initTopLevelBrowserWindow(win);
    }
  },

  






  _initBrowserWindow: function BA_initBrowserWindow(win, isTopLevel) {
    
    
    
    var unwrappedWin = XPCNativeWrapper.unwrap(win);

    Object.defineProperty(unwrappedWin, 'top', {
      get: function() {
        if (isTopLevel) {
          return win;
        }
        
        
        return XPCNativeWrapper.unwrap(win.parent).top;
      }
    });

    Object.defineProperty(unwrappedWin, 'parent', {
      get: function() {
        if (isTopLevel) {
          return win;
        }
        return win.parent;
      }
    });

    Object.defineProperty(unwrappedWin, 'frameElement', {
      get: function() {
        if (isTopLevel) {
          return null;
        }
        return win.frameElement;
      }
    });
  },

  


  _initTopLevelBrowserWindow: function BA_initTopLevelBrowserWindow(win) {
    
    
    var chromeHandler = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShell)
                           .chromeEventHandler;

    if (chromeHandler && !this._chromeEventHandlersWatching.has(chromeHandler)) {
      this._chromeEventHandlersWatching.set(chromeHandler, true);
      this._addChromeEventHandlerListeners(chromeHandler);
    }
  },

  



  _addChromeEventHandlerListeners: function BA_addChromeEventHandlerListeners(chromeHandler) {
    var browserElementAPI = this;

    
    
    chromeHandler.addEventListener(
      'DOMTitleChanged',
      function(e) {
        var win = e.target.defaultView;
        if (browserElementAPI._topLevelBrowserWindows.has(win)) {
          browserElementAPI._fireCustomEvent('titlechange', e.target.title,
                                             win, win.frameElement);
        }
      },
       false,
       false);
  },

  





  _fireEvent: function BA_fireEvent(name, win) {
    
    
    var evt = new win.Event('mozbrowser' + name);
    win.setTimeout(function() { win.frameElement.dispatchEvent(evt) }, 0);
  },

  



  _fireCustomEvent: function BA_fireCustomEvent(name, data, win) {
    var evt = new win.CustomEvent('mozbrowser' + name, {detail: data});
    win.setTimeout(function() { win.frameElement.dispatchEvent(evt) }, 0);
  },

  



  _progressListener: {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                           Ci.nsISupportsWeakReference,
                                           Ci.nsISupports]),

    _getWindow: function(webProgress) {
      return webProgress.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindow);
    },

    onLocationChange: function(webProgress, request, location, flags) {
      this._browserElementAPI._fireCustomEvent('locationchange', location.spec,
                                               this._getWindow(webProgress));
    },

    onStateChange: function(webProgress, request, stateFlags, status) {
      if (stateFlags & Ci.nsIWebProgressListener.STATE_START) {
        this._browserElementAPI._fireEvent('loadstart', this._getWindow(webProgress));
      }
      if (stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        this._browserElementAPI._fireEvent('loadend', this._getWindow(webProgress));
      }
    },

    onStatusChange: function(webProgress, request, status, message) {},
    onProgressChange: function(webProgress, request, curSelfProgress,
                               maxSelfProgress, curTotalProgress, maxTotalProgress) {},
    onSecurityChange: function(webProgress, request, aState) {}
  },

  


  observe: function BA_observe(subject, topic, data) {
    switch(topic) {
    case 'app-startup':
      this._init();
      break;
    case 'content-document-global-created':
      this._observeContentGlobalCreated(subject);
      break;
    case 'docshell-marked-as-browser-frame':
      this._observeDocshellMarkedAsBrowserFrame(subject);
      break;
    case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
      if (data == BROWSER_FRAMES_ENABLED_PREF) {
        this._init();
      }
      break;
    }
  },
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([BrowserElementAPI]);
