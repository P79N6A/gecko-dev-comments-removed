



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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
      var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
      prefs.addObserver(BROWSER_FRAMES_ENABLED_PREF, this,  true);
      return;
    }

    debug("_init");
    this._initialized = true;

    
    
    
    this._bepMap = new WeakMap();

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

  _observeInProcessBrowserFrameShown: function(frameLoader) {
    debug("In-process browser frame shown " + frameLoader);
    this._createBrowserElementParent(frameLoader);
  },

  _observeRemoteBrowserFrameShown: function(frameLoader) {
    debug("Remote browser frame shown " + frameLoader);
    this._createBrowserElementParent(frameLoader);
  },

  _createBrowserElementParent: function(frameLoader) {
    let frameElement = frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerElement;
    this._bepMap.set(frameElement, new BrowserElementParent(frameLoader));
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
      this._observeRemoteBrowserFrameShown(subject);
      break;
    case 'in-process-browser-frame-shown':
      this._observeInProcessBrowserFrameShown(subject);
      break;
    case 'content-document-global-created':
      this._observeContentGlobalCreated(subject);
      break;
    }
  },
};

function BrowserElementParent(frameLoader) {
  debug("Creating new BrowserElementParent object for " + frameLoader);
  this._domRequestCounter = 0;
  this._pendingDOMRequests = {};

  this._frameElement = frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerElement;
  if (!this._frameElement) {
    debug("No frame element?");
    return;
  }

  this._mm = frameLoader.messageManager;

  
  

  let self = this;
  function addMessageListener(msg, handler) {
    self._mm.addMessageListener('browser-element-api:' + msg, handler.bind(self));
  }

  addMessageListener("hello", this._recvHello);
  addMessageListener("locationchange", this._fireEventFromMsg);
  addMessageListener("loadstart", this._fireEventFromMsg);
  addMessageListener("loadend", this._fireEventFromMsg);
  addMessageListener("titlechange", this._fireEventFromMsg);
  addMessageListener("iconchange", this._fireEventFromMsg);
  addMessageListener("close", this._fireEventFromMsg);
  addMessageListener("securitychange", this._fireEventFromMsg);
  addMessageListener("get-mozapp-manifest-url", this._sendMozAppManifestURL);
  addMessageListener("keyevent", this._fireKeyEvent);
  addMessageListener("showmodalprompt", this._handleShowModalPrompt);
  addMessageListener('got-screenshot', this._gotDOMRequestResult);
  addMessageListener('got-can-go-back', this._gotDOMRequestResult);
  addMessageListener('got-can-go-forward', this._gotDOMRequestResult);

  function defineMethod(name, fn) {
    XPCNativeWrapper.unwrap(self._frameElement)[name] = fn.bind(self);
  }

  function defineDOMRequestMethod(domName, msgName) {
    XPCNativeWrapper.unwrap(self._frameElement)[domName] = self._sendDOMRequest.bind(self, msgName);
  }

  
  defineMethod('setVisible', this._setVisible);
  defineMethod('goBack', this._goBack);
  defineMethod('goForward', this._goForward);
  defineDOMRequestMethod('getScreenshot', 'get-screenshot');
  defineDOMRequestMethod('getCanGoBack', 'get-can-go-back');
  defineDOMRequestMethod('getCanGoForward', 'get-can-go-forward');

  this._mm.loadFrameScript("chrome://global/content/BrowserElementChild.js",
                            true);
}

BrowserElementParent.prototype = {
  get _window() {
    return this._frameElement.ownerDocument.defaultView;
  },

  _sendAsyncMsg: function(msg, data) {
    this._frameElement.QueryInterface(Ci.nsIFrameLoaderOwner)
                      .frameLoader
                      .messageManager
                      .sendAsyncMessage('browser-element-api:' + msg, data);
  },

  _recvHello: function(data) {
    debug("recvHello");
  },

  



  _fireEventFromMsg: function(data) {
    let name = data.name.substring('browser-element-api:'.length);
    let detail = data.json;

    debug('fireEventFromMsg: ' + name + ', ' + detail);
    let evt = this._createEvent(name, detail,
                                 false);
    this._frameElement.dispatchEvent(evt);
  },

  _handleShowModalPrompt: function(data) {
    
    
    
    
    
    
    
    
    
    

    let detail = data.json;
    debug('handleShowPrompt ' + JSON.stringify(detail));

    
    
    let windowID = detail.windowID;
    delete detail.windowID;
    debug("Event will have detail: " + JSON.stringify(detail));
    let evt = this._createEvent('showmodalprompt', detail,
                                 true);

    let self = this;
    let unblockMsgSent = false;
    function sendUnblockMsg() {
      if (unblockMsgSent) {
        return;
      }
      unblockMsgSent = true;

      
      

      let data = { windowID: windowID,
                   returnValue: evt.detail.returnValue };
      self._sendAsyncMsg('unblock-modal-prompt', data);
    }

    XPCNativeWrapper.unwrap(evt.detail).unblock = function() {
      sendUnblockMsg();
    };

    this._frameElement.dispatchEvent(evt);

    if (!evt.defaultPrevented) {
      
      
      sendUnblockMsg();
    }
  },

  _createEvent: function(evtName, detail, cancelable) {
    
    
    if (detail !== undefined && detail !== null) {
      return new this._window.CustomEvent('mozbrowser' + evtName,
                                          { bubbles: true,
                                            cancelable: cancelable,
                                            detail: detail });
    }

    return new this._window.Event('mozbrowser' + evtName,
                                  { bubbles: true,
                                    cancelable: cancelable });
  },

  _sendMozAppManifestURL: function(data) {
    return this._frameElement.getAttribute('mozapp');
  },

  








  _sendDOMRequest: function(msgName) {
    let id = 'req_' + this._domRequestCounter++;
    let req = Services.DOMRequest.createRequest(this._window);
    this._pendingDOMRequests[id] = req;
    this._sendAsyncMsg(msgName, {id: id});
    return req;
  },

  







  _gotDOMRequestResult: function(data) {
    let req = this._pendingDOMRequests[data.json.id];
    delete this._pendingDOMRequests[data.json.id];
    Services.DOMRequest.fireSuccess(req, data.json.rv);
  },

  _setVisible: function(visible) {
    this._sendAsyncMsg('set-visible', {visible: visible});
  },

  _goBack: function() {
    this._sendAsyncMsg('go-back');
  },

  _goForward: function() {
    this._sendAsyncMsg('go-forward');
  },

  _fireKeyEvent: function(data) {
    let evt = this._window.document.createEvent("KeyboardEvent");
    evt.initKeyEvent(data.json.type, true, true, this._window,
                     false, false, false, false, 
                     data.json.keyCode,
                     data.json.charCode);

    this._frameElement.dispatchEvent(evt);
  },
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([BrowserElementParentFactory]);
