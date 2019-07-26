



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
let Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/BrowserElementParent.jsm");

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";
const BROWSER_FRAMES_ENABLED_PREF = "dom.mozBrowserFramesEnabled";

function debug(msg) {
  
}













function BrowserElementParentFactory() {
  this._initialized = false;
}

BrowserElementParentFactory.prototype = {
  classID: Components.ID("{ddeafdac-cb39-47c4-9cb8-c9027ee36d26}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  



  _init: function() {
    if (this._initialized) {
      return;
    }

    
    
    if (!this._browserFramesPrefEnabled()) {
      Services.prefs.addObserver(BROWSER_FRAMES_ENABLED_PREF, this,  true);
      return;
    }

    debug("_init");
    this._initialized = true;

    
    
    
    this._bepMap = new WeakMap();

    Services.obs.addObserver(this, 'remote-browser-shown',  true);
    Services.obs.addObserver(this, 'inprocess-browser-shown',  true);
  },

  _browserFramesPrefEnabled: function() {
    try {
      return Services.prefs.getBoolPref(BROWSER_FRAMES_ENABLED_PREF);
    }
    catch(e) {
      return false;
    }
  },

  _observeInProcessBrowserFrameShown: function(frameLoader) {
    
    if (!frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerIsBrowserOrAppFrame) {
      return;
    }
    debug("In-process browser frame shown " + frameLoader);
    this._createBrowserElementParent(frameLoader,  false);
  },

  _observeRemoteBrowserFrameShown: function(frameLoader) {
    
    if (!frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerIsBrowserOrAppFrame) {
      return;
    }
    debug("Remote browser frame shown " + frameLoader);
    this._createBrowserElementParent(frameLoader,  true);
  },

  _createBrowserElementParent: function(frameLoader, hasRemoteFrame) {
    let frameElement = frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerElement;
    this._bepMap.set(frameElement, BrowserElementParentBuilder.create(frameLoader, hasRemoteFrame));
  },

  observe: function(subject, topic, data) {
    switch(topic) {
    case 'app-startup':
      this._init();
      break;
    case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
      if (data == BROWSER_FRAMES_ENABLED_PREF) {
        this._init();
      }
      break;
    case 'remote-browser-shown':
      this._observeRemoteBrowserFrameShown(subject);
      break;
    case 'inprocess-browser-shown':
      this._observeInProcessBrowserFrameShown(subject);
      break;
    case 'content-document-global-created':
      this._observeContentGlobalCreated(subject);
      break;
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([BrowserElementParentFactory]);
