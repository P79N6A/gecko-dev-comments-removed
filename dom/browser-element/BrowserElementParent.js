



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
let Cr = Components.results;






Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/BrowserElementPromptService.jsm");

XPCOMUtils.defineLazyGetter(this, "DOMApplicationRegistry", function () {
  Cu.import("resource://gre/modules/Webapps.jsm");
  return DOMApplicationRegistry;
});

function debug(msg) {
  
}

function getIntPref(prefName, def) {
  try {
    return Services.prefs.getIntPref(prefName);
  }
  catch(err) {
    return def;
  }
}

function visibilityChangeHandler(e) {
  
  let win = e.target.defaultView;

  if (!win._browserElementParents) {
    return;
  }

  let beps = Cu.nondeterministicGetWeakMapKeys(win._browserElementParents);
  if (beps.length == 0) {
    win.removeEventListener('visibilitychange', visibilityChangeHandler);
    return;
  }

  for (let i = 0; i < beps.length; i++) {
    beps[i]._ownerVisibilityChange();
  }
}

function defineNoReturnMethod(fn) {
  return function method() {
    if (!this._domRequestReady) {
      
      let args = Array.slice(arguments);
      args.unshift(this);
      this._pendingAPICalls.push(method.bind.apply(fn, args));
      return;
    }
    if (this._isAlive()) {
      fn.apply(this, arguments);
    }
  };
}

function defineDOMRequestMethod(msgName) {
  return function() {
    return this._sendDOMRequest(msgName);
  };
}

function BrowserElementParent() {
  debug("Creating new BrowserElementParent object");
  this._domRequestCounter = 0;
  this._domRequestReady = false;
  this._pendingAPICalls = [];
  this._pendingDOMRequests = {};
  this._pendingSetInputMethodActive = [];
  this._nextPaintListeners = [];

  Services.obs.addObserver(this, 'ask-children-to-exit-fullscreen',  true);
  Services.obs.addObserver(this, 'oop-frameloader-crashed',  true);
  Services.obs.addObserver(this, 'copypaste-docommand',  true);
}

BrowserElementParent.prototype = {

  classDescription: "BrowserElementAPI implementation",
  classID: Components.ID("{9f171ac4-0939-4ef8-b360-3408aedc3060}"),
  contractID: "@mozilla.org/dom/browser-element-api;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBrowserElementAPI,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  setFrameLoader: function(frameLoader) {
    this._frameLoader = frameLoader;
    this._frameElement = frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerElement;
    if (!this._frameElement) {
      debug("No frame element?");
      return;
    }
    
    
    
    
    
    
    
    
    
    
    if (!this._window._browserElementParents) {
      this._window._browserElementParents = new WeakMap();
      this._window.addEventListener('visibilitychange',
                                    visibilityChangeHandler,
                                     false,
                                     false);
    }

    this._window._browserElementParents.set(this, null);

    
    BrowserElementPromptService.mapFrameToBrowserElementParent(this._frameElement, this);
    this._setupMessageListener();
    this._registerAppManifest();
  },

  _runPendingAPICall: function() {
    if (!this._pendingAPICalls) {
      return;
    }
    for (let i = 0; i < this._pendingAPICalls.length; i++) {
      try {
        this._pendingAPICalls[i]();
      } catch (e) {
        
        debug('Exception when running pending API call: ' +  e);
      }
    }
    delete this._pendingAPICalls;
  },

  _registerAppManifest: function() {
    
    
    let appManifestURL =
          this._frameElement.QueryInterface(Ci.nsIMozBrowserFrame).appManifestURL;
    if (appManifestURL) {
      let inParent = Cc["@mozilla.org/xre/app-info;1"]
                       .getService(Ci.nsIXULRuntime)
                       .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
      if (inParent) {
        DOMApplicationRegistry.registerBrowserElementParentForApp(
          { manifestURL: appManifestURL }, this._mm);
      } else {
        this._mm.sendAsyncMessage("Webapps:RegisterBEP",
                                  { manifestURL: appManifestURL });
      }
    }
  },

  _setupMessageListener: function() {
    this._mm = this._frameLoader.messageManager;
    let self = this;
    let isWidget = this._frameLoader
                       .QueryInterface(Ci.nsIFrameLoader)
                       .ownerIsWidget;

    
    
    
    
    let mmCalls = {
      "hello": this._recvHello,
      "loadstart": this._fireProfiledEventFromMsg,
      "loadend": this._fireProfiledEventFromMsg,
      "close": this._fireEventFromMsg,
      "error": this._fireEventFromMsg,
      "firstpaint": this._fireProfiledEventFromMsg,
      "documentfirstpaint": this._fireProfiledEventFromMsg,
      "nextpaint": this._recvNextPaint,
      "got-purge-history": this._gotDOMRequestResult,
      "got-screenshot": this._gotDOMRequestResult,
      "got-contentdimensions": this._gotDOMRequestResult,
      "got-can-go-back": this._gotDOMRequestResult,
      "got-can-go-forward": this._gotDOMRequestResult,
      "fullscreen-origin-change": this._remoteFullscreenOriginChange,
      "rollback-fullscreen": this._remoteFrameFullscreenReverted,
      "exit-fullscreen": this._exitFullscreen,
      "got-visible": this._gotDOMRequestResult,
      "visibilitychange": this._childVisibilityChange,
      "got-set-input-method-active": this._gotDOMRequestResult,
      "selectionstatechanged": this._handleSelectionStateChanged,
      "scrollviewchange": this._handleScrollViewChange,
    };

    let mmSecuritySensitiveCalls = {
      "showmodalprompt": this._handleShowModalPrompt,
      "contextmenu": this._fireCtxMenuEvent,
      "securitychange": this._fireEventFromMsg,
      "locationchange": this._fireEventFromMsg,
      "iconchange": this._fireEventFromMsg,
      "scrollareachanged": this._fireEventFromMsg,
      "titlechange": this._fireProfiledEventFromMsg,
      "opensearch": this._fireEventFromMsg,
      "manifestchange": this._fireEventFromMsg,
      "metachange": this._fireEventFromMsg,
      "resize": this._fireEventFromMsg,
      "activitydone": this._fireEventFromMsg,
      "scroll": this._fireEventFromMsg,
      "opentab": this._fireEventFromMsg
    };

    this._mm.addMessageListener('browser-element-api:call', function(aMsg) {
      if (!self._isAlive()) {
        return;
      }

      if (aMsg.data.msg_name in mmCalls) {
        return mmCalls[aMsg.data.msg_name].apply(self, arguments);
      } else if (!isWidget && aMsg.data.msg_name in mmSecuritySensitiveCalls) {
        return mmSecuritySensitiveCalls[aMsg.data.msg_name]
                 .apply(self, arguments);
      }
    });
  },

  



  _isAlive: function() {
    return !Cu.isDeadWrapper(this._frameElement) &&
           !Cu.isDeadWrapper(this._frameElement.ownerDocument) &&
           !Cu.isDeadWrapper(this._frameElement.ownerDocument.defaultView);
  },

  get _window() {
    return this._frameElement.ownerDocument.defaultView;
  },

  get _windowUtils() {
    return this._window.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
  },

  promptAuth: function(authDetail, callback) {
    let evt;
    let self = this;
    let callbackCalled = false;
    let cancelCallback = function() {
      if (!callbackCalled) {
        callbackCalled = true;
        callback(false, null, null);
      }
    };

    
    
    if (authDetail.isOnlyPassword ||
        this._frameLoader.QueryInterface(Ci.nsIFrameLoader).ownerIsWidget) {
      cancelCallback();
      return;
    }

    
    let detail = {
      host:     authDetail.host,
      realm:    authDetail.realm,
      isProxy:  authDetail.isProxy
    };

    evt = this._createEvent('usernameandpasswordrequired', detail,
                             true);
    Cu.exportFunction(function(username, password) {
      if (callbackCalled)
        return;
      callbackCalled = true;
      callback(true, username, password);
    }, evt.detail, { defineAs: 'authenticate' });

    Cu.exportFunction(cancelCallback, evt.detail, { defineAs: 'cancel' });

    this._frameElement.dispatchEvent(evt);

    if (!evt.defaultPrevented) {
      cancelCallback();
    }
  },

  _sendAsyncMsg: function(msg, data) {
    try {
      if (!data) {
        data = { };
      }

      data.msg_name = msg;
      this._mm.sendAsyncMessage('browser-element-api:call', data);
    } catch (e) {
      return false;
    }
    return true;
  },

  _recvHello: function() {
    debug("recvHello");

    
    
    
    
    if (this._window.document.hidden) {
      this._ownerVisibilityChange();
    }

    if (!this._domRequestReady) {
      
      
      this._domRequestReady = true;
      this._runPendingAPICall();
    }
  },

  _fireCtxMenuEvent: function(data) {
    let detail = data.json;
    let evtName = detail.msg_name;

    debug('fireCtxMenuEventFromMsg: ' + evtName + ' ' + detail);
    let evt = this._createEvent(evtName, detail,  true);

    if (detail.contextmenu) {
      var self = this;
      Cu.exportFunction(function(id) {
        self._sendAsyncMsg('fire-ctx-callback', {menuitem: id});
      }, evt.detail, { defineAs: 'contextMenuItemSelected' });
    }

    
    
    
    return !this._frameElement.dispatchEvent(evt);
  },

  


  _fireProfiledEventFromMsg: function(data) {
    if (Services.profiler !== undefined) {
      Services.profiler.AddMarker(data.json.msg_name);
    }
    this._fireEventFromMsg(data);
  },

  



  _fireEventFromMsg: function(data) {
    let detail = data.json;
    let name = detail.msg_name;

    
    
    if ("_payload_" in detail) {
      detail = detail._payload_;
    }

    debug('fireEventFromMsg: ' + name + ', ' + JSON.stringify(detail));
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

    Cu.exportFunction(sendUnblockMsg, evt.detail, { defineAs: 'unblock' });

    this._frameElement.dispatchEvent(evt);

    if (!evt.defaultPrevented) {
      
      
      sendUnblockMsg();
    }
  },

  _handleSelectionStateChanged: function(data) {
    let evt = this._createEvent('selectionstatechanged', data.json,
                                 false);
    this._frameElement.dispatchEvent(evt);
  },

  _handleScrollViewChange: function(data) {
    let evt = this._createEvent("scrollviewchange", data.json,
                                 false);
    this._frameElement.dispatchEvent(evt);
  },

  _createEvent: function(evtName, detail, cancelable) {
    
    
    if (detail !== undefined && detail !== null) {
      detail = Cu.cloneInto(detail, this._window);
      return new this._window.CustomEvent('mozbrowser' + evtName,
                                          { bubbles: true,
                                            cancelable: cancelable,
                                            detail: detail });
    }

    return new this._window.Event('mozbrowser' + evtName,
                                  { bubbles: true,
                                    cancelable: cancelable });
  },

  











  _sendDOMRequest: function(msgName, args) {
    let id = 'req_' + this._domRequestCounter++;
    let req = Services.DOMRequest.createRequest(this._window);
    let self = this;
    let send = function() {
      if (!self._isAlive()) {
        return;
      }
      if (self._sendAsyncMsg(msgName, {id: id, args: args})) {
        self._pendingDOMRequests[id] = req;
      } else {
        Services.DOMRequest.fireErrorAsync(req, "fail");
      }
    };
    if (this._domRequestReady) {
      send();
    } else {
      
      this._pendingAPICalls.push(send);
    }
    return req;
  },

  












  _gotDOMRequestResult: function(data) {
    let req = this._pendingDOMRequests[data.json.id];
    delete this._pendingDOMRequests[data.json.id];

    if ('successRv' in data.json) {
      debug("Successful gotDOMRequestResult.");
      let clientObj = Cu.cloneInto(data.json.successRv, this._window);
      Services.DOMRequest.fireSuccess(req, clientObj);
    }
    else {
      debug("Got error in gotDOMRequestResult.");
      Services.DOMRequest.fireErrorAsync(req,
        Cu.cloneInto(data.json.errorMsg, this._window));
    }
  },

  setVisible: defineNoReturnMethod(function(visible) {
    this._sendAsyncMsg('set-visible', {visible: visible});
    this._frameLoader.visible = visible;
  }),

  getVisible: defineDOMRequestMethod('get-visible'),

  setActive: defineNoReturnMethod(function(active) {
    this._frameLoader.visible = active;
  }),

  getActive: function() {
    if (!this._isAlive()) {
      throw Components.Exception("Dead content process",
                                 Cr.NS_ERROR_DOM_INVALID_STATE_ERR);
    }

    return this._frameLoader.visible;
  },

  getChildProcessOffset: function() {
    let offset = { x: 0, y: 0 };
    let tabParent = this._frameLoader.tabParent;
    if (tabParent) {
      let offsetX = {};
      let offsetY = {};
      tabParent.getChildProcessOffset(offsetX, offsetY);
      offset.x = offsetX.value;
      offset.y = offsetY.value;
    }
    return offset;
  },

  sendMouseEvent: defineNoReturnMethod(function(type, x, y, button, clickCount, modifiers) {
    let offset = this.getChildProcessOffset();
    x += offset.x;
    y += offset.y;

    this._sendAsyncMsg("send-mouse-event", {
      "type": type,
      "x": x,
      "y": y,
      "button": button,
      "clickCount": clickCount,
      "modifiers": modifiers
    });
  }),

  sendTouchEvent: defineNoReturnMethod(function(type, identifiers, touchesX, touchesY,
                                                radiisX, radiisY, rotationAngles, forces,
                                                count, modifiers) {

    let tabParent = this._frameLoader.tabParent;
    if (tabParent && tabParent.useAsyncPanZoom) {
      tabParent.injectTouchEvent(type,
                                 identifiers,
                                 touchesX,
                                 touchesY,
                                 radiisX,
                                 radiisY,
                                 rotationAngles,
                                 forces,
                                 count,
                                 modifiers);
    } else {
      let offset = this.getChildProcessOffset();
      for (var i = 0; i < touchesX.length; i++) {
        touchesX[i] += offset.x;
      }
      for (var i = 0; i < touchesY.length; i++) {
        touchesY[i] += offset.y;
      }
      this._sendAsyncMsg("send-touch-event", {
        "type": type,
        "identifiers": identifiers,
        "touchesX": touchesX,
        "touchesY": touchesY,
        "radiisX": radiisX,
        "radiisY": radiisY,
        "rotationAngles": rotationAngles,
        "forces": forces,
        "count": count,
        "modifiers": modifiers
      });
    }
  }),

  getCanGoBack: defineDOMRequestMethod('get-can-go-back'),
  getCanGoForward: defineDOMRequestMethod('get-can-go-forward'),
  getContentDimensions: defineDOMRequestMethod('get-contentdimensions'),

  goBack: defineNoReturnMethod(function() {
    this._sendAsyncMsg('go-back');
  }),

  goForward: defineNoReturnMethod(function() {
    this._sendAsyncMsg('go-forward');
  }),

  reload: defineNoReturnMethod(function(hardReload) {
    this._sendAsyncMsg('reload', {hardReload: hardReload});
  }),

  stop: defineNoReturnMethod(function() {
    this._sendAsyncMsg('stop');
  }),

  


  zoom: defineNoReturnMethod(function(zoom) {
    zoom *= 100;
    zoom = Math.min(getIntPref("zoom.maxPercent", 300), zoom);
    zoom = Math.max(getIntPref("zoom.minPercent", 50), zoom);
    this._sendAsyncMsg('zoom', {zoom: zoom / 100.0});
  }),

  purgeHistory: defineDOMRequestMethod('purge-history'),


  download: function(_url, _options) {
    if (!this._isAlive()) {
      return null;
    }
    
    let uri = Services.io.newURI(_url, null, null);
    let url = uri.QueryInterface(Ci.nsIURL);

    debug('original _options = ' + uneval(_options));

    
    _options = _options || {};
    if (!_options.filename) {
      _options.filename = url.fileName;
    }

    debug('final _options = ' + uneval(_options));

    
    if (!_options.filename) {
      throw Components.Exception("Invalid argument", Cr.NS_ERROR_INVALID_ARG);
    }

    let interfaceRequestor =
      this._frameLoader.loadContext.QueryInterface(Ci.nsIInterfaceRequestor);
    let req = Services.DOMRequest.createRequest(this._window);

    function DownloadListener() {
      debug('DownloadListener Constructor');
    }
    DownloadListener.prototype = {
      extListener: null,
      onStartRequest: function(aRequest, aContext) {
        debug('DownloadListener - onStartRequest');
        let extHelperAppSvc =
          Cc['@mozilla.org/uriloader/external-helper-app-service;1'].
          getService(Ci.nsIExternalHelperAppService);
        let channel = aRequest.QueryInterface(Ci.nsIChannel);

        
        
        
        
        
        _options.filename = _options.filename.replace(/^\.+/, "");

        let ext = null;
        let mimeSvc = extHelperAppSvc.QueryInterface(Ci.nsIMIMEService);
        try {
          ext = '.' + mimeSvc.getPrimaryExtension(channel.contentType, '');
        } catch (e) { ext = null; }

        
        if (ext && !_options.filename.endsWith(ext)) {
          _options.filename += ext;
        }
        
        channel.contentDispositionFilename = _options.filename;

        this.extListener =
          extHelperAppSvc.doContent(
              channel.contentType,
              aRequest,
              interfaceRequestor,
              true);
        this.extListener.onStartRequest(aRequest, aContext);
      },
      onStopRequest: function(aRequest, aContext, aStatusCode) {
        debug('DownloadListener - onStopRequest (aStatusCode = ' +
               aStatusCode + ')');
        if (aStatusCode == Cr.NS_OK) {
          
          debug('DownloadListener - Download Successful.');
          Services.DOMRequest.fireSuccess(req, aStatusCode);
        }
        else {
          
          debug('DownloadListener - Download Failed!');
          Services.DOMRequest.fireError(req, aStatusCode);
        }

        if (this.extListener) {
          this.extListener.onStopRequest(aRequest, aContext, aStatusCode);
        }
      },
      onDataAvailable: function(aRequest, aContext, aInputStream,
                                aOffset, aCount) {
        this.extListener.onDataAvailable(aRequest, aContext, aInputStream,
                                         aOffset, aCount);
      },
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIStreamListener,
                                             Ci.nsIRequestObserver])
    };

    
    
    let referrer = null;
    let principal = null;
    if (_options.referrer) {
      
      try {
        referrer = Services.io.newURI(_options.referrer, null, null);
      }
      catch(e) {
        debug('Malformed referrer -- ' + e);
      }
      
      
      
      principal = 
        Services.scriptSecurityManager.getAppCodebasePrincipal(
          referrer, 
          this._frameLoader.loadContext.appId, 
          this._frameLoader.loadContext.isInBrowserElement);
    }

    debug('Using principal? ' + !!principal);

    let channel = 
      Services.io.newChannelFromURI2(url,
                                     null,       
                                     principal,  
                                     principal,  
                                     Ci.nsILoadInfo.SEC_NORMAL,
                                     Ci.nsIContentPolicy.TYPE_OTHER);

    
    channel.notificationCallbacks = interfaceRequestor;

    
    
    let flags = Ci.nsIChannel.LOAD_CALL_CONTENT_SNIFFERS |
                Ci.nsIChannel.LOAD_BYPASS_CACHE;
    if (channel instanceof Ci.nsICachingChannel) {
      debug('This is a caching channel. Forcing bypass.');
      flags |= Ci.nsICachingChannel.LOAD_BYPASS_LOCAL_CACHE_IF_BUSY;
    }

    channel.loadFlags |= flags;

    if (channel instanceof Ci.nsIHttpChannel) {
      debug('Setting HTTP referrer = ' + (referrer && referrer.spec)); 
      channel.referrer = referrer;
      if (channel instanceof Ci.nsIHttpChannelInternal) {
        channel.forceAllowThirdPartyCookie = true;
      }
    }

    
    channel.asyncOpen(new DownloadListener(), null);

    return req;
  },

  getScreenshot: function(_width, _height, _mimeType) {
    if (!this._isAlive()) {
      throw Components.Exception("Dead content process",
                                 Cr.NS_ERROR_DOM_INVALID_STATE_ERR);
    }

    let width = parseInt(_width);
    let height = parseInt(_height);
    let mimeType = (typeof _mimeType === 'string') ?
      _mimeType.trim().toLowerCase() : 'image/jpeg';
    if (isNaN(width) || isNaN(height) || width < 0 || height < 0) {
      throw Components.Exception("Invalid argument",
                                 Cr.NS_ERROR_INVALID_ARG);
    }

    return this._sendDOMRequest('get-screenshot',
                                {width: width, height: height,
                                 mimeType: mimeType});
  },

  _recvNextPaint: function(data) {
    let listeners = this._nextPaintListeners;
    this._nextPaintListeners = [];
    for (let listener of listeners) {
      try {
        listener.recvNextPaint();
      } catch (e) {
        
      }
    }
  },

  addNextPaintListener: function(listener) {
    if (!this._isAlive()) {
      throw Components.Exception("Dead content process",
                                 Cr.NS_ERROR_DOM_INVALID_STATE_ERR);
    }

    let self = this;
    let run = function() {
      if (self._nextPaintListeners.push(listener) == 1)
        self._sendAsyncMsg('activate-next-paint-listener');
    };
    if (!this._domRequestReady) {
      this._pendingAPICalls.push(run);
    } else {
      run();
    }
  },

  removeNextPaintListener: function(listener) {
    if (!this._isAlive()) {
      throw Components.Exception("Dead content process",
                                 Cr.NS_ERROR_DOM_INVALID_STATE_ERR);
    }

    let self = this;
    let run = function() {
      for (let i = self._nextPaintListeners.length - 1; i >= 0; i--) {
        if (self._nextPaintListeners[i] == listener) {
          self._nextPaintListeners.splice(i, 1);
          break;
        }
      }

      if (self._nextPaintListeners.length == 0)
        self._sendAsyncMsg('deactivate-next-paint-listener');
    };
    if (!this._domRequestReady) {
      this._pendingAPICalls.push(run);
    } else {
      run();
    }
  },

  setInputMethodActive: function(isActive) {
    if (!this._isAlive()) {
      throw Components.Exception("Dead content process",
                                 Cr.NS_ERROR_DOM_INVALID_STATE_ERR);
    }

    if (typeof isActive !== 'boolean') {
      throw Components.Exception("Invalid argument",
                                 Cr.NS_ERROR_INVALID_ARG);
    }

    return this._sendDOMRequest('set-input-method-active',
                                {isActive: isActive});
  },

  setNFCFocus: function(isFocus) {
    if (!this._isAlive()) {
      throw Components.Exception("Dead content process",
                                 Cr.NS_ERROR_DOM_INVALID_STATE_ERR);
    }

    
    
    
    
    try {
      var tabId = this._frameLoader.QueryInterface(Ci.nsIFrameLoader)
                                   .tabParent
                                   .tabId;
    } catch(e) {
      debug("SetNFCFocus for in-process mode is not yet supported");
      throw Components.Exception("SetNFCFocus for in-process mode is not yet supported",
                                 Cr.NS_ERROR_NOT_IMPLEMENTED);
    }

    try {
      let nfcContentHelper =
        Cc["@mozilla.org/nfc/content-helper;1"].getService(Ci.nsINfcBrowserAPI);
      nfcContentHelper.setFocusApp(tabId, isFocus);
    } catch(e) {
      
    }
  },

  


  _ownerVisibilityChange: function() {
    this._sendAsyncMsg('owner-visibility-change',
                       {visible: !this._window.document.hidden});
  },

  







  _childVisibilityChange: function(data) {
    debug("_childVisibilityChange(" + data.json.visible + ")");
    this._frameLoader.visible = data.json.visible;

    this._fireEventFromMsg(data);
  },

  _exitFullscreen: function() {
    this._windowUtils.exitFullscreen();
  },

  _remoteFullscreenOriginChange: function(data) {
    let origin = data.json._payload_;
    this._windowUtils.remoteFrameFullscreenChanged(this._frameElement, origin);
  },

  _remoteFrameFullscreenReverted: function(data) {
    this._windowUtils.remoteFrameFullscreenReverted();
  },

  _fireFatalError: function() {
    let evt = this._createEvent('error', {type: 'fatal'},
                                 false);
    this._frameElement.dispatchEvent(evt);
  },

  observe: function(subject, topic, data) {
    switch(topic) {
    case 'oop-frameloader-crashed':
      if (this._isAlive() && subject == this._frameLoader) {
        this._fireFatalError();
      }
      break;
    case 'ask-children-to-exit-fullscreen':
      if (this._isAlive() &&
          this._frameElement.ownerDocument == subject &&
          this._frameLoader.QueryInterface(Ci.nsIFrameLoader).tabParent) {
        this._sendAsyncMsg('exit-fullscreen');
      }
      break;
    case 'copypaste-docommand':
      if (this._isAlive() && this._frameElement.isEqualNode(subject.wrappedJSObject)) {
        this._sendAsyncMsg('do-command', { command: data });
      }
      break;
    default:
      debug('Unknown topic: ' + topic);
      break;
    };
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([BrowserElementParent]);
