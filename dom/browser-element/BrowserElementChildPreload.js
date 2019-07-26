



"use strict";

dump("######################## BrowserElementChildPreload.js loaded\n");

var BrowserElementIsReady = false;

let { classes: Cc, interfaces: Ci, results: Cr, utils: Cu }  = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/BrowserElementPromptService.jsm");


let whitelistedEvents = [
  Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE,   
  Ci.nsIDOMKeyEvent.DOM_VK_SLEEP,    
  Ci.nsIDOMKeyEvent.DOM_VK_CONTEXT_MENU,
  Ci.nsIDOMKeyEvent.DOM_VK_F5,       
  Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP,  
  Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN 
];

function debug(msg) {
  
}

function sendAsyncMsg(msg, data) {
  
  
  if (!BrowserElementIsReady)
    return;

  if (!data) {
    data = { };
  }

  data.msg_name = msg;
  sendAsyncMessage('browser-element-api:call', data);
}

function sendSyncMsg(msg, data) {
  
  
  if (!BrowserElementIsReady)
    return;

  if (!data) {
    data = { };
  }

  data.msg_name = msg;
  return sendSyncMessage('browser-element-api:call', data);
}

let CERTIFICATE_ERROR_PAGE_PREF = 'security.alternate_certificate_error_page';

let NS_ERROR_MODULE_BASE_OFFSET = 0x45;
let NS_ERROR_MODULE_SECURITY= 21;
function NS_ERROR_GET_MODULE(err) {
  return ((((err) >> 16) - NS_ERROR_MODULE_BASE_OFFSET) & 0x1fff) 
}

function NS_ERROR_GET_CODE(err) {
  return ((err) & 0xffff);
}

let SEC_ERROR_BASE = Ci.nsINSSErrorsService.NSS_SEC_ERROR_BASE;
let SEC_ERROR_UNKNOWN_ISSUER = (SEC_ERROR_BASE + 13);
let SEC_ERROR_CA_CERT_INVALID =   (SEC_ERROR_BASE + 36);
let SEC_ERROR_UNTRUSTED_ISSUER = (SEC_ERROR_BASE + 20);
let SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE = (SEC_ERROR_BASE + 30);
let SEC_ERROR_UNTRUSTED_CERT = (SEC_ERROR_BASE + 21);
let SEC_ERROR_INADEQUATE_KEY_USAGE = (SEC_ERROR_BASE + 90);
let SEC_ERROR_EXPIRED_CERTIFICATE = (SEC_ERROR_BASE + 11);
let SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED = (SEC_ERROR_BASE + 176);

let SSL_ERROR_BASE = Ci.nsINSSErrorsService.NSS_SSL_ERROR_BASE;
let SSL_ERROR_BAD_CERT_DOMAIN = (SSL_ERROR_BASE + 12);

function getErrorClass(errorCode) {
  let NSPRCode = -1 * NS_ERROR_GET_CODE(errorCode);
 
  switch (NSPRCode) {
    case SEC_ERROR_UNKNOWN_ISSUER:
    case SEC_ERROR_CA_CERT_INVALID:
    case SEC_ERROR_UNTRUSTED_ISSUER:
    case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
    case SEC_ERROR_UNTRUSTED_CERT:
    case SEC_ERROR_INADEQUATE_KEY_USAGE:
    case SSL_ERROR_BAD_CERT_DOMAIN:
    case SEC_ERROR_EXPIRED_CERTIFICATE:
    case SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED:
      return Ci.nsINSSErrorsService.ERROR_CLASS_BAD_CERT;
    default:
      return Ci.nsINSSErrorsService.ERROR_CLASS_SSL_PROTOCOL;
  }

  return null;
}












var global = this;

function BrowserElementChild() {
  
  this._windowIDDict = {};

  
  
  
  
  
  
  this._forcedVisible = true;
  this._ownerVisible = true;

  this._nextPaintHandler = null;

  this._init();
};

BrowserElementChild.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  _init: function() {
    debug("Starting up.");

    BrowserElementPromptService.mapWindowToBrowserElementChild(content, this);

    docShell.QueryInterface(Ci.nsIWebProgress)
            .addProgressListener(this._progressListener,
                                 Ci.nsIWebProgress.NOTIFY_LOCATION |
                                 Ci.nsIWebProgress.NOTIFY_SECURITY |
                                 Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);

    docShell.QueryInterface(Ci.nsIWebNavigation)
            .sessionHistory = Cc["@mozilla.org/browser/shistory;1"]
                                .createInstance(Ci.nsISHistory);

    
    var securityUI = Cc['@mozilla.org/secure_browser_ui;1']
                       .createInstance(Ci.nsISecureBrowserUI);
    securityUI.init(content);

    
    
    this._ctxHandlers = {};
    
    this._ctxCounter = 0;

    this._shuttingDown = false;

    addEventListener('DOMTitleChanged',
                     this._titleChangedHandler.bind(this),
                      true,
                      false);

    addEventListener('DOMLinkAdded',
                     this._linkAddedHandler.bind(this),
                      true,
                      false);

    
    
    
    
    
    addEventListener('unload',
                     this._unloadHandler.bind(this),
                      false,
                      false);

    
    this._addMozAfterPaintHandler(function () {
      sendAsyncMsg('firstpaint');
    });

    let self = this;

    let mmCalls = {
      "purge-history": this._recvPurgeHistory,
      "get-screenshot": this._recvGetScreenshot,
      "set-visible": this._recvSetVisible,
      "get-visible": this._recvVisible,
      "send-mouse-event": this._recvSendMouseEvent,
      "send-touch-event": this._recvSendTouchEvent,
      "get-can-go-back": this._recvCanGoBack,
      "get-can-go-forward": this._recvCanGoForward,
      "go-back": this._recvGoBack,
      "go-forward": this._recvGoForward,
      "reload": this._recvReload,
      "stop": this._recvStop,
      "unblock-modal-prompt": this._recvStopWaiting,
      "fire-ctx-callback": this._recvFireCtxCallback,
      "owner-visibility-change": this._recvOwnerVisibilityChange,
      "exit-fullscreen": this._recvExitFullscreen.bind(this),
      "activate-next-paint-listener": this._activateNextPaintListener.bind(this),
      "set-input-method-active": this._recvSetInputMethodActive.bind(this),
      "deactivate-next-paint-listener": this._deactivateNextPaintListener.bind(this)
    }

    addMessageListener("browser-element-api:call", function(aMessage) {
      if (aMessage.data.msg_name in mmCalls) {
        return mmCalls[aMessage.data.msg_name].apply(self, arguments);
      }
    });

    let els = Cc["@mozilla.org/eventlistenerservice;1"]
                .getService(Ci.nsIEventListenerService);

    
    
    els.addSystemEventListener(global, 'keydown',
                               this._keyEventHandler.bind(this),
                                true);
    els.addSystemEventListener(global, 'keypress',
                               this._keyEventHandler.bind(this),
                                true);
    els.addSystemEventListener(global, 'keyup',
                               this._keyEventHandler.bind(this),
                                true);
    els.addSystemEventListener(global, 'DOMWindowClose',
                               this._windowCloseHandler.bind(this),
                                false);
    els.addSystemEventListener(global, 'DOMWindowCreated',
                               this._windowCreatedHandler.bind(this),
                                true);
    els.addSystemEventListener(global, 'DOMWindowResize',
                               this._windowResizeHandler.bind(this),
                                false);
    els.addSystemEventListener(global, 'contextmenu',
                               this._contextmenuHandler.bind(this),
                                false);
    els.addSystemEventListener(global, 'scroll',
                               this._scrollEventHandler.bind(this),
                                false);

    Services.obs.addObserver(this,
                             "fullscreen-origin-change",
                              true);

    Services.obs.addObserver(this,
                             'ask-parent-to-exit-fullscreen',
                              true);

    Services.obs.addObserver(this,
                             'ask-parent-to-rollback-fullscreen',
                              true);

    Services.obs.addObserver(this,
                             'xpcom-shutdown',
                              true);
  },

  observe: function(subject, topic, data) {
    
    
    if (!content || subject != content.document)
      return;
    switch (topic) {
      case 'fullscreen-origin-change':
        sendAsyncMsg('fullscreen-origin-change', { _payload_: data });
        break;
      case 'ask-parent-to-exit-fullscreen':
        sendAsyncMsg('exit-fullscreen');
        break;
      case 'ask-parent-to-rollback-fullscreen':
        sendAsyncMsg('rollback-fullscreen');
        break;
      case 'xpcom-shutdown':
        this._shuttingDown = true;
        break;
    }
  },

  



  _unloadHandler: function() {
    this._shuttingDown = true;
  },

  _tryGetInnerWindowID: function(win) {
    let utils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils);
    try {
      return utils.currentInnerWindowID;
    }
    catch(e) {
      return null;
    }
  },

  


  showModalPrompt: function(win, args) {
    let utils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils);

    args.windowID = { outer: utils.outerWindowID,
                      inner: this._tryGetInnerWindowID(win) };
    sendAsyncMsg('showmodalprompt', args);

    let returnValue = this._waitForResult(win);

    if (args.promptType == 'prompt' ||
        args.promptType == 'confirm' ||
        args.promptType == 'custom-prompt') {
      return returnValue;
    }
  },

  



  _waitForResult: function(win) {
    debug("_waitForResult(" + win + ")");
    let utils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils);

    let outerWindowID = utils.outerWindowID;
    let innerWindowID = this._tryGetInnerWindowID(win);
    if (innerWindowID === null) {
      
      
      debug("_waitForResult: No inner window. Bailing.");
      return;
    }

    this._windowIDDict[outerWindowID] = Cu.getWeakReference(win);

    debug("Entering modal state (outerWindowID=" + outerWindowID + ", " +
                                "innerWindowID=" + innerWindowID + ")");

    utils.enterModalState();

    
    
    if (!win.modalDepth) {
      win.modalDepth = 0;
    }
    win.modalDepth++;
    let origModalDepth = win.modalDepth;

    let thread = Services.tm.currentThread;
    debug("Nested event loop - begin");
    while (win.modalDepth == origModalDepth && !this._shuttingDown) {
      
      
      
      if (this._tryGetInnerWindowID(win) !== innerWindowID) {
        debug("_waitForResult: Inner window ID changed " +
              "while in nested event loop.");
        break;
      }

      thread.processNextEvent( true);
    }
    debug("Nested event loop - finish");

    
    
    if (innerWindowID !== this._tryGetInnerWindowID(win)) {
      throw Components.Exception("Modal state aborted by navigation",
                                 Cr.NS_ERROR_NOT_AVAILABLE);
    }

    let returnValue = win.modalReturnValue;
    delete win.modalReturnValue;

    if (!this._shuttingDown) {
      utils.leaveModalState();
    }

    debug("Leaving modal state (outerID=" + outerWindowID + ", " +
                               "innerID=" + innerWindowID + ")");
    return returnValue;
  },

  _recvStopWaiting: function(msg) {
    let outerID = msg.json.windowID.outer;
    let innerID = msg.json.windowID.inner;
    let returnValue = msg.json.returnValue;
    debug("recvStopWaiting(outer=" + outerID + ", inner=" + innerID +
          ", returnValue=" + returnValue + ")");

    if (!this._windowIDDict[outerID]) {
      debug("recvStopWaiting: No record of outer window ID " + outerID);
      return;
    }

    let win = this._windowIDDict[outerID].get();
    delete this._windowIDDict[outerID];

    if (!win) {
      debug("recvStopWaiting, but window is gone\n");
      return;
    }

    if (innerID !== this._tryGetInnerWindowID(win)) {
      debug("recvStopWaiting, but inner ID has changed\n");
      return;
    }

    debug("recvStopWaiting " + win);
    win.modalReturnValue = returnValue;
    win.modalDepth--;
  },

  _recvExitFullscreen: function() {
    var utils = content.document.defaultView
                       .QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    utils.exitFullscreen();
  },

  _titleChangedHandler: function(e) {
    debug("Got titlechanged: (" + e.target.title + ")");
    var win = e.target.defaultView;

    
    
    if (win == content) {
      sendAsyncMsg('titlechange', { _payload_: e.target.title });
    }
    else {
      debug("Not top level!");
    }
  },

  _iconChangedHandler: function(e) {
    debug('Got iconchanged: (' + e.target.href + ')');

    sendAsyncMsg('iconchange', { _payload_: e.target.href });
  },

  _openSearchHandler: function(e) {
    debug('Got opensearch: (' + e.target.href + ')');

    if (e.target.type !== "application/opensearchdescription+xml") {
      return;
    }

    sendAsyncMsg('opensearch', { title: e.target.title,
                                 href: e.target.href });

  },

  
  _linkAddedHandler: function(e) {
    let win = e.target.ownerDocument.defaultView;
    
    
    if (win != content) {
      debug('Not top level!');
      return;
    }

    let handlers = {
      'icon': this._iconChangedHandler,
      'search': this._openSearchHandler
    };

    debug('Got linkAdded: (' + e.target.href + ') ' + e.target.rel);
    e.target.rel.split(' ').forEach(function(x) {
      let token = x.toLowerCase();
      if (handlers[token]) {
        handlers[token](e);
      }
    }, this);
  },

  _addMozAfterPaintHandler: function(callback) {
    function onMozAfterPaint() {
      let uri = docShell.QueryInterface(Ci.nsIWebNavigation).currentURI;
      if (uri.spec != "about:blank") {
        debug("Got afterpaint event: " + uri.spec);
        removeEventListener('MozAfterPaint', onMozAfterPaint,
                             true);
        callback();
      }
    }

    addEventListener('MozAfterPaint', onMozAfterPaint,  true);
    return onMozAfterPaint;
  },

  _removeMozAfterPaintHandler: function(listener) {
    removeEventListener('MozAfterPaint', listener,
                         true);
  },

  _activateNextPaintListener: function(e) {
    if (!this._nextPaintHandler) {
      this._nextPaintHandler = this._addMozAfterPaintHandler(function () {
        this._nextPaintHandler = null;
        sendAsyncMsg('nextpaint');
      }.bind(this));
    }
  },

  _deactivateNextPaintListener: function(e) {
    if (this._nextPaintHandler) {
      this._removeMozAfterPaintHandler(this._nextPaintHandler);
      this._nextPaintHandler = null;
    }
  },

  _windowCloseHandler: function(e) {
    let win = e.target;
    if (win != content || e.defaultPrevented) {
      return;
    }

    debug("Closing window " + win);
    sendAsyncMsg('close');

    
    e.preventDefault();
  },

  _windowCreatedHandler: function(e) {
    let targetDocShell = e.target.defaultView
          .QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIWebNavigation);
    if (targetDocShell != docShell) {
      return;
    }

    let uri = docShell.QueryInterface(Ci.nsIWebNavigation).currentURI;
    debug("Window created: " + uri.spec);
    if (uri.spec != "about:blank") {
      this._addMozAfterPaintHandler(function () {
        sendAsyncMsg('documentfirstpaint');
      });
    }
  },

  _windowResizeHandler: function(e) {
    let win = e.target;
    if (win != content || e.defaultPrevented) {
      return;
    }

    debug("resizing window " + win);
    sendAsyncMsg('resize', { width: e.detail.width, height: e.detail.height });

    
    e.preventDefault();
  },

  _contextmenuHandler: function(e) {
    debug("Got contextmenu");

    if (e.defaultPrevented) {
      return;
    }

    this._ctxCounter++;
    this._ctxHandlers = {};

    var elem = e.target;
    var menuData = {systemTargets: [], contextmenu: null};
    var ctxMenuId = null;

    while (elem && elem.parentNode) {
      var ctxData = this._getSystemCtxMenuData(elem);
      if (ctxData) {
        menuData.systemTargets.push({
          nodeName: elem.nodeName,
          data: ctxData
        });
      }

      if (!ctxMenuId && 'hasAttribute' in elem && elem.hasAttribute('contextmenu')) {
        ctxMenuId = elem.getAttribute('contextmenu');
      }
      elem = elem.parentNode;
    }

    if (ctxMenuId) {
      var menu = e.target.ownerDocument.getElementById(ctxMenuId);
      if (menu) {
        menuData.contextmenu = this._buildMenuObj(menu, '');
      }
    }

    
    
    
    
    
    
    if (sendSyncMsg('contextmenu', menuData)[0]) {
      e.preventDefault();
    } else {
      this._ctxHandlers = {};
    }
  },

  _getSystemCtxMenuData: function(elem) {
    if ((elem instanceof Ci.nsIDOMHTMLAnchorElement && elem.href) ||
        (elem instanceof Ci.nsIDOMHTMLAreaElement && elem.href)) {
      return {uri: elem.href};
    }
    if (elem instanceof Ci.nsIImageLoadingContent && elem.currentURI) {
      return {uri: elem.currentURI.spec};
    }
    if (elem instanceof Ci.nsIDOMHTMLImageElement) {
      return {uri: elem.src};
    }
    if (elem instanceof Ci.nsIDOMHTMLMediaElement) {
      let hasVideo = !(elem.readyState >= elem.HAVE_METADATA &&
                       (elem.videoWidth == 0 || elem.videoHeight == 0));
      return {uri: elem.currentSrc || elem.src, hasVideo: hasVideo};
    }
    return false;
  },

  _scrollEventHandler: function(e) {
    let win = e.target.defaultView;
    if (win != content) {
      return;
    }

    debug("scroll event " + win);
    sendAsyncMsg("scroll", { top: win.scrollY, left: win.scrollX });
  },

  _recvPurgeHistory: function(data) {
    debug("Received purgeHistory message: (" + data.json.id + ")");

    let history = docShell.QueryInterface(Ci.nsIWebNavigation).sessionHistory;

    try {
      if (history && history.count) {
        history.PurgeHistory(history.count);
      }
    } catch(e) {}

    sendAsyncMsg('got-purge-history', { id: data.json.id, successRv: true });
  },

  _recvGetScreenshot: function(data) {
    debug("Received getScreenshot message: (" + data.json.id + ")");

    let self = this;
    let maxWidth = data.json.args.width;
    let maxHeight = data.json.args.height;
    let domRequestID = data.json.id;

    let takeScreenshotClosure = function() {
      self._takeScreenshot(maxWidth, maxHeight, domRequestID);
    };

    let maxDelayMS = 2000;
    try {
      maxDelayMS = Services.prefs.getIntPref('dom.browserElement.maxScreenshotDelayMS');
    }
    catch(e) {}

    
    
    
    Cc['@mozilla.org/message-loop;1'].getService(Ci.nsIMessageLoop).postIdleTask(
      takeScreenshotClosure, maxDelayMS);
  },

  




  _takeScreenshot: function(maxWidth, maxHeight, domRequestID) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    debug("Taking a screenshot: maxWidth=" + maxWidth +
          ", maxHeight=" + maxHeight +
          ", domRequestID=" + domRequestID + ".");

    if (!content) {
      
      
      debug("No content yet!");
      return;
    }

    let scaleWidth = Math.min(1, maxWidth / content.innerWidth);
    let scaleHeight = Math.min(1, maxHeight / content.innerHeight);

    let scale = Math.max(scaleWidth, scaleHeight);

    let canvasWidth = Math.min(maxWidth, Math.round(content.innerWidth * scale));
    let canvasHeight = Math.min(maxHeight, Math.round(content.innerHeight * scale));

    var canvas = content.document
      .createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    canvas.mozOpaque = true;
    canvas.width = canvasWidth;
    canvas.height = canvasHeight;

    var ctx = canvas.getContext("2d");
    ctx.scale(scale, scale);
    ctx.drawWindow(content, 0, 0, content.innerWidth, content.innerHeight,
                   "rgb(255,255,255)");

    
    
    
    
    canvas.toBlob(function(blob) {
      sendAsyncMsg('got-screenshot', {
        id: domRequestID,
        successRv: blob
      });
    }, 'image/jpeg');
  },

  _recvFireCtxCallback: function(data) {
    debug("Received fireCtxCallback message: (" + data.json.menuitem + ")");
    
    if (data.json.menuitem in this._ctxHandlers) {
      this._ctxHandlers[data.json.menuitem].click();
      this._ctxHandlers = {};
    } else {
      debug("Ignored invalid contextmenu invocation");
    }
  },

  _buildMenuObj: function(menu, idPrefix) {
    function maybeCopyAttribute(src, target, attribute) {
      if (src.getAttribute(attribute)) {
        target[attribute] = src.getAttribute(attribute);
      }
    }

    var menuObj = {type: 'menu', items: []};
    maybeCopyAttribute(menu, menuObj, 'label');

    for (var i = 0, child; child = menu.children[i++];) {
      if (child.nodeName === 'MENU') {
        menuObj.items.push(this._buildMenuObj(child, idPrefix + i + '_'));
      } else if (child.nodeName === 'MENUITEM') {
        var id = this._ctxCounter + '_' + idPrefix + i;
        var menuitem = {id: id, type: 'menuitem'};
        maybeCopyAttribute(child, menuitem, 'label');
        maybeCopyAttribute(child, menuitem, 'icon');
        this._ctxHandlers[id] = child;
        menuObj.items.push(menuitem);
      }
    }
    return menuObj;
  },

  _recvSetVisible: function(data) {
    debug("Received setVisible message: (" + data.json.visible + ")");
    if (this._forcedVisible == data.json.visible) {
      return;
    }

    this._forcedVisible = data.json.visible;
    this._updateVisibility();
  },

  _recvVisible: function(data) {
    sendAsyncMsg('got-visible', {
      id: data.json.id,
      successRv: docShell.isActive
    });
  },

  



  _recvOwnerVisibilityChange: function(data) {
    debug("Received ownerVisibilityChange: (" + data.json.visible + ")");
    this._ownerVisible = data.json.visible;
    this._updateVisibility();
  },

  _updateVisibility: function() {
    var visible = this._forcedVisible && this._ownerVisible;
    if (docShell.isActive !== visible) {
      docShell.isActive = visible;
      sendAsyncMsg('visibilitychange', {visible: visible});
    }
  },

  _recvSendMouseEvent: function(data) {
    let json = data.json;
    let utils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    utils.sendMouseEvent(json.type, json.x, json.y, json.button,
                         json.clickCount, json.modifiers);
  },

  _recvSendTouchEvent: function(data) {
    let json = data.json;
    let utils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    utils.sendTouchEvent(json.type, json.identifiers, json.touchesX,
                         json.touchesY, json.radiisX, json.radiisY,
                         json.rotationAngles, json.forces, json.count,
                         json.modifiers);
  },

  _recvCanGoBack: function(data) {
    var webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
    sendAsyncMsg('got-can-go-back', {
      id: data.json.id,
      successRv: webNav.canGoBack
    });
  },

  _recvCanGoForward: function(data) {
    var webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
    sendAsyncMsg('got-can-go-forward', {
      id: data.json.id,
      successRv: webNav.canGoForward
    });
  },

  _recvGoBack: function(data) {
    try {
      docShell.QueryInterface(Ci.nsIWebNavigation).goBack();
    } catch(e) {
      
    }
  },

  _recvGoForward: function(data) {
    try {
      docShell.QueryInterface(Ci.nsIWebNavigation).goForward();
    } catch(e) {
      
    }
  },

  _recvReload: function(data) {
    let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
    let reloadFlags = data.json.hardReload ?
      webNav.LOAD_FLAGS_BYPASS_PROXY | webNav.LOAD_FLAGS_BYPASS_CACHE :
      webNav.LOAD_FLAGS_NONE;
    try {
      webNav.reload(reloadFlags);
    } catch(e) {
      
    }
  },

  _recvStop: function(data) {
    let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
    webNav.stop(webNav.STOP_NETWORK);
  },

  _recvSetInputMethodActive: function(data) {
    let msgData = { id: data.json.id };
    
    let nav = XPCNativeWrapper.unwrap(content.document.defaultView.navigator);
    if (nav.mozInputMethod) {
      
      new XPCNativeWrapper(nav.mozInputMethod).setActive(data.json.args.isActive);
      msgData.successRv = null;
    } else {
      msgData.errorMsg = 'Cannot access mozInputMethod.';
    }
    sendAsyncMsg('got-set-input-method-active', msgData);
  },

  _keyEventHandler: function(e) {
    if (whitelistedEvents.indexOf(e.keyCode) != -1 && !e.defaultPrevented) {
      sendAsyncMsg('keyevent', {
        type: e.type,
        keyCode: e.keyCode,
        charCode: e.charCode,
      });
    }
  },

  
  
  _progressListener: {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                           Ci.nsISupportsWeakReference]),
    _seenLoadStart: false,

    onLocationChange: function(webProgress, request, location, flags) {
      
      if (webProgress != docShell) {
        return;
      }

      
      
      if (!this._seenLoadStart) {
        return;
      }

      
      location = Cc["@mozilla.org/docshell/urifixup;1"]
        .getService(Ci.nsIURIFixup).createExposableURI(location);

      sendAsyncMsg('locationchange', { _payload_: location.spec });
    },

    onStateChange: function(webProgress, request, stateFlags, status) {
      if (webProgress != docShell) {
        return;
      }

      if (stateFlags & Ci.nsIWebProgressListener.STATE_START) {
        this._seenLoadStart = true;
        sendAsyncMsg('loadstart');
      }

      if (stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        let bgColor = 'transparent';
        try {
          bgColor = content.getComputedStyle(content.document.body)
                           .getPropertyValue('background-color');
        } catch (e) {}
        sendAsyncMsg('loadend', {backgroundColor: bgColor});

        
        
        if (status == Cr.NS_OK ||
            status == Cr.NS_BINDING_ABORTED) {
          return;
        }

        if (NS_ERROR_GET_MODULE(status) == NS_ERROR_MODULE_SECURITY && 
            getErrorClass(status) == Ci.nsINSSErrorsService.ERROR_CLASS_BAD_CERT) {

          
          
          
          
          var errorPage = null;
          try {
            errorPage = Services.prefs.getCharPref(CERTIFICATE_ERROR_PAGE_PREF);
          } catch(e) {}

          if (errorPage == 'certerror') {
            sendAsyncMsg('error', { type: 'certerror' });
            return;
          }
        }

        
        
        sendAsyncMsg('error', { type: 'other' });
      }
    },

    onSecurityChange: function(webProgress, request, state) {
      if (webProgress != docShell) {
        return;
      }

      var stateDesc;
      if (state & Ci.nsIWebProgressListener.STATE_IS_SECURE) {
        stateDesc = 'secure';
      }
      else if (state & Ci.nsIWebProgressListener.STATE_IS_BROKEN) {
        stateDesc = 'broken';
      }
      else if (state & Ci.nsIWebProgressListener.STATE_IS_INSECURE) {
        stateDesc = 'insecure';
      }
      else {
        debug("Unexpected securitychange state!");
        stateDesc = '???';
      }

      
      var isEV = !!(state & Ci.nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL);

      sendAsyncMsg('securitychange', { state: stateDesc, extendedValidation: isEV });
    },

    onStatusChange: function(webProgress, request, status, message) {},
    onProgressChange: function(webProgress, request, curSelfProgress,
                               maxSelfProgress, curTotalProgress, maxTotalProgress) {},
  },

  
  _messageManagerPublic: {
    sendAsyncMessage: global.sendAsyncMessage.bind(global),
    sendSyncMessage: global.sendSyncMessage.bind(global),
    addMessageListener: global.addMessageListener.bind(global),
    removeMessageListener: global.removeMessageListener.bind(global)
  },

  get messageManager() {
    return this._messageManagerPublic;
  }
};

var api = new BrowserElementChild();

