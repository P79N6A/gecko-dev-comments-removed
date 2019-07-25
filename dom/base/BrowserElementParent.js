



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";
const BROWSER_FRAMES_ENABLED_PREF = "dom.mozBrowserFramesEnabled";

function debug(msg) {
  
}










function BrowserElementParent() {}
BrowserElementParent.prototype = {
  classID: Components.ID("{ddeafdac-cb39-47c4-9cb8-c9027ee36d26}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  



  _init: function() {
    debug("_init");

    if (this._initialized) {
      return;
    }

    
    
    if (!this._browserFramesPrefEnabled()) {
      var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
      prefs.addObserver(BROWSER_FRAMES_ENABLED_PREF, this,  true);
      return;
    }

    this._initialized = true;

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.addObserver(this, 'remote-browser-frame-shown',  true);
    os.addObserver(this, 'in-process-browser-frame-shown',  true);
  },

  _browserFramesPrefEnabled: function() {
    var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    try {
      return prefs.getBoolPref(BROWSER_FRAMES_ENABLED_PREF);
    }
    catch(e) {
      return false;
    }
  },

  _observeInProcessBrowserFrameShown: function(frameLoader, isMozApp) {
    debug("In-process browser frame shown " + frameLoader);
    this._setUpMessageManagerListeners(frameLoader, isMozApp);
  },

  _observeRemoteBrowserFrameShown: function(frameLoader, isMozApp) {
    debug("Remote browser frame shown " + frameLoader);
    this._setUpMessageManagerListeners(frameLoader, isMozApp);
  },

  _setUpMessageManagerListeners: function(frameLoader, isMozApp) {
    let frameElement = frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerElement;
    if (!frameElement) {
      debug("No frame element?");
      return;
    }

    let mm = frameLoader.messageManager;

    
    

    function addMessageListener(msg, handler) {
      mm.addMessageListener('browser-element-api:' + msg, handler.bind(this, frameElement));
    }

    addMessageListener("hello", this._recvHello);
    addMessageListener("locationchange", this._fireEventFromMsg);
    addMessageListener("loadstart", this._fireEventFromMsg);
    addMessageListener("loadend", this._fireEventFromMsg);
    addMessageListener("titlechange", this._fireEventFromMsg);
    addMessageListener("iconchange", this._fireEventFromMsg);

    mm.loadFrameScript("chrome://global/content/BrowserElementChild.js",
                        true);
    if (isMozApp) {
      mm.loadFrameScript("data:,content.QueryInterface(Ci.nsIInterfaceRequestor)" +
                         "             .getInterface(Components.interfaces.nsIDOMWindowUtils)" +
                         "             .setIsApp(true);",
                          true);
    }
  },

  _recvHello: function(frameElement, data) {
    debug("recvHello " + frameElement);
  },

  



  _fireEventFromMsg: function(frameElement, data) {
    let name = data.name;
    let detail = data.json;

    debug('fireEventFromMsg: ' + name + ' ' + detail);
    let evtName = name.substring('browser-element-api:'.length);
    let win = frameElement.ownerDocument.defaultView;
    let evt;

    
    
    if (detail !== undefined && detail !== null) {
      evt = new win.CustomEvent('mozbrowser' + evtName, {detail: detail});
    }
    else {
      evt = new win.Event('mozbrowser' + evtName);
    }

    frameElement.dispatchEvent(evt);
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
    case 'remote-browser-frame-shown':
      this._observeRemoteBrowserFrameShown(subject, data == "is-moz-app:true");
      break;
    case 'in-process-browser-frame-shown':
      this._observeInProcessBrowserFrameShown(subject, data == "is-moz-app:true");
      break;
    case 'content-document-global-created':
      this._observeContentGlobalCreated(subject);
      break;
    }
  },
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([BrowserElementParent]);
