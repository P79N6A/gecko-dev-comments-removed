



"use strict";

dump("######################## BrowserElementChildPreload.js loaded\n");

var BrowserElementIsReady = false;

let { classes: Cc, interfaces: Ci, results: Cr, utils: Cu }  = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/BrowserElementPromptService.jsm");

let kLongestReturnedString = 128;

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

const OBSERVED_EVENTS = [
  'fullscreen-origin-change',
  'ask-parent-to-exit-fullscreen',
  'ask-parent-to-rollback-fullscreen',
  'xpcom-shutdown',
  'activity-done'
];

const COMMAND_MAP = {
  'cut': 'cmd_cut',
  'copy': 'cmd_copyAndCollapseToEnd',
  'paste': 'cmd_paste',
  'selectall': 'cmd_selectAll'
};












var global = this;

function BrowserElementChild() {
  
  this._windowIDDict = {};

  
  
  
  
  
  
  this._forcedVisible = true;
  this._ownerVisible = true;

  this._nextPaintHandler = null;

  this._isContentWindowCreated = false;
  this._pendingSetInputMethodActive = [];
  this._selectionStateChangedTarget = null;

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

    addEventListener('MozScrolledAreaChanged',
                     this._mozScrollAreaChanged.bind(this),
                      true,
                      false);

    addEventListener('DOMMetaAdded',
                     this._metaChangedHandler.bind(this),
                      true,
                      false);

    addEventListener('DOMMetaChanged',
                     this._metaChangedHandler.bind(this),
                      true,
                      false);

    addEventListener('DOMMetaRemoved',
                     this._metaChangedHandler.bind(this),
                      true,
                      false);

    addEventListener('mozselectionstatechanged',
                     this._selectionStateChangedHandler.bind(this),
                      true,
                      false);

    addEventListener('scrollviewchange',
                     this._ScrollViewChangeHandler.bind(this),
                      true,
                      false);

    addEventListener('click',
                     this._ClickHandler.bind(this),
                      false,
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
      "get-contentdimensions": this._recvGetContentDimensions,
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
      "zoom": this._recvZoom,
      "unblock-modal-prompt": this._recvStopWaiting,
      "fire-ctx-callback": this._recvFireCtxCallback,
      "owner-visibility-change": this._recvOwnerVisibilityChange,
      "exit-fullscreen": this._recvExitFullscreen.bind(this),
      "activate-next-paint-listener": this._activateNextPaintListener.bind(this),
      "set-input-method-active": this._recvSetInputMethodActive.bind(this),
      "deactivate-next-paint-listener": this._deactivateNextPaintListener.bind(this),
      "do-command": this._recvDoCommand
    }

    addMessageListener("browser-element-api:call", function(aMessage) {
      if (aMessage.data.msg_name in mmCalls) {
        return mmCalls[aMessage.data.msg_name].apply(self, arguments);
      }
    });

    let els = Cc["@mozilla.org/eventlistenerservice;1"]
                .getService(Ci.nsIEventListenerService);

    
    
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

    OBSERVED_EVENTS.forEach((aTopic) => {
      Services.obs.addObserver(this, aTopic, false);
    });
  },

  observe: function(subject, topic, data) {
    
    
    if (topic !== 'activity-done' && (!content || subject != content.document))
      return;
    if (topic == 'activity-done' && docShell !== subject)
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
      case 'activity-done':
        sendAsyncMsg('activitydone', { success: (data == 'activity-success') });
        break;
      case 'xpcom-shutdown':
        this._shuttingDown = true;
        break;
    }
  },

  



  _unloadHandler: function() {
    this._shuttingDown = true;
    OBSERVED_EVENTS.forEach((aTopic) => {
      Services.obs.removeObserver(this, aTopic);
    });
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

    Services.obs.notifyObservers(null, 'BEC:ShownModalPrompt', null);

    if (args.promptType == 'prompt' ||
        args.promptType == 'confirm' ||
        args.promptType == 'custom-prompt') {
      return returnValue;
    }
  },

  _isCommandEnabled: function(cmd) {
    let command = COMMAND_MAP[cmd];
    if (!command) {
      return false;
    }

    return docShell.isCommandEnabled(command);
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

    if (win.modalDepth == 0) {
      delete this._windowIDDict[outerWindowID];
    }

    
    
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

  _maybeCopyAttribute: function(src, target, attribute) {
    if (src.getAttribute(attribute)) {
      target[attribute] = src.getAttribute(attribute);
    }
  },

  _iconChangedHandler: function(e) {
    debug('Got iconchanged: (' + e.target.href + ')');
    let icon = { href: e.target.href };
    this._maybeCopyAttribute(e.target, icon, 'sizes');
    this._maybeCopyAttribute(e.target, icon, 'rel');
    sendAsyncMsg('iconchange', icon);
  },

  _openSearchHandler: function(e) {
    debug('Got opensearch: (' + e.target.href + ')');

    if (e.target.type !== "application/opensearchdescription+xml") {
      return;
    }

    sendAsyncMsg('opensearch', { title: e.target.title,
                                 href: e.target.href });

  },

  _manifestChangedHandler: function(e) {
    debug('Got manifestchanged: (' + e.target.href + ')');
    let manifest = { href: e.target.href };
    sendAsyncMsg('manifestchange', manifest);

  },

  
  _linkAddedHandler: function(e) {
    let win = e.target.ownerDocument.defaultView;
    
    
    if (win != content) {
      debug('Not top level!');
      return;
    }

    let handlers = {
      'icon': this._iconChangedHandler.bind(this),
      'apple-touch-icon': this._iconChangedHandler.bind(this),
      'search': this._openSearchHandler,
      'manifest': this._manifestChangedHandler
    };

    debug('Got linkAdded: (' + e.target.href + ') ' + e.target.rel);
    e.target.rel.split(' ').forEach(function(x) {
      let token = x.toLowerCase();
      if (handlers[token]) {
        handlers[token](e);
      }
    }, this);
  },

  _metaChangedHandler: function(e) {
    let win = e.target.ownerDocument.defaultView;
    
    
    if (win != content) {
      debug('Not top level!');
      return;
    }

    if (!e.target.name) {
      return;
    }

    debug('Got metaChanged: (' + e.target.name + ') ' + e.target.content);

    let handlers = {
      'theme-color': this._themeColorChangedHandler,
      'application-name': this._applicationNameChangedHandler
    };

    let handler = handlers[e.target.name];
    if (handler) {
      handler(e.type, e.target);
    }
  },

  _applicationNameChangedHandler: function(eventType, target) {
    if (eventType !== 'DOMMetaAdded') {
      
      
      return;
    }

    let meta = { name: 'application-name',
                 content: target.content };

    let lang;
    let elm;

    for (elm = target;
         !lang && elm && elm.nodeType == target.ELEMENT_NODE;
         elm = elm.parentNode) {
      if (elm.hasAttribute('lang')) {
        lang = elm.getAttribute('lang');
        continue;
      }

      if (elm.hasAttributeNS('http://www.w3.org/XML/1998/namespace', 'lang')) {
        lang = elm.getAttributeNS('http://www.w3.org/XML/1998/namespace', 'lang');
        continue;
      }
    }

    
    if (!lang && elm.nodeType == target.DOCUMENT_NODE) {
      lang = elm.contentLanguage;
    }

    if (lang) {
      meta.lang = lang;
    }

    sendAsyncMsg('metachange', meta);
  },

  _ScrollViewChangeHandler: function(e) {
    e.stopPropagation();
    let detail = {
      state: e.state,
    };
    sendAsyncMsg('scrollviewchange', detail);
  },

  _ClickHandler: function(e) {

    let isHTMLLink = node =>
      ((node instanceof Ci.nsIDOMHTMLAnchorElement && node.href) ||
       (node instanceof Ci.nsIDOMHTMLAreaElement && node.href) ||
        node instanceof Ci.nsIDOMHTMLLinkElement);

    
    
    if ((Services.appinfo.OS == 'Darwin' && e.metaKey) ||
        (Services.appinfo.OS != 'Darwin' && e.ctrlKey) ||
         e.button == 1) {

      let node = e.target;
      while (node && !isHTMLLink(node)) {
        node = node.parentNode;
      }

      if (node) {
        sendAsyncMsg('opentab', {url: node.href});
      }
    }
  },

  _selectionStateChangedHandler: function(e) {
    e.stopPropagation();

    if (!this._isContentWindowCreated) {
      return;
    }

    let boundingClientRect = e.boundingClientRect;

    let isCollapsed = (e.selectedText.length == 0);
    let isMouseUp = (e.states.indexOf('mouseup') == 0);
    let canPaste = this._isCommandEnabled("paste");

    if (this._selectionStateChangedTarget != e.target) {
      
      
      
      
      if(e.states.length == 0 ||
         e.states.indexOf('drag') == 0 ||
         e.states.indexOf('keypress') == 0 ||
         e.states.indexOf('mousedown') == 0) {
        return;
      }

      
      
      if (isCollapsed) {
        if (isMouseUp && canPaste) {
          
          
        } else if (e.states.indexOf('blur') == 0) {
          
        } else if (e.states.indexOf('taponcaret') == 0) {
          
        } else {
          return;
        }
      }
    }

    
    
    
    
    
    if (e.visible && !isCollapsed) {
      this._selectionStateChangedTarget = e.target;
    } else if (canPaste && isCollapsed) {
      this._selectionStateChangedTarget = e.target;
    } else {
      this._selectionStateChangedTarget = null;
    }

    let zoomFactor = content.screen.width / content.innerWidth;

    let detail = {
      rect: {
        width: boundingClientRect ? boundingClientRect.width : 0,
        height: boundingClientRect ? boundingClientRect.height : 0,
        top: boundingClientRect ? boundingClientRect.top : 0,
        bottom: boundingClientRect ? boundingClientRect.bottom : 0,
        left: boundingClientRect ? boundingClientRect.left : 0,
        right: boundingClientRect ? boundingClientRect.right : 0,
      },
      commands: {
        canSelectAll: this._isCommandEnabled("selectall"),
        canCut: this._isCommandEnabled("cut"),
        canCopy: this._isCommandEnabled("copy"),
        canPaste: this._isCommandEnabled("paste"),
      },
      zoomFactor: zoomFactor,
      states: e.states,
      isCollapsed: (e.selectedText.length == 0),
      visible: e.visible,
    };

    
    let currentWindow = e.target.defaultView;
    while (currentWindow.realFrameElement) {
      let currentRect = currentWindow.realFrameElement.getBoundingClientRect();
      detail.rect.top += currentRect.top;
      detail.rect.bottom += currentRect.top;
      detail.rect.left += currentRect.left;
      detail.rect.right += currentRect.left;
      currentWindow = currentWindow.realFrameElement.ownerDocument.defaultView;
    }

    sendAsyncMsg('selectionstatechanged', detail);
  },

  _themeColorChangedHandler: function(eventType, target) {
    let meta = {
      name: 'theme-color',
      content: target.content,
      type: eventType.replace('DOMMeta', '').toLowerCase()
    };
    sendAsyncMsg('metachange', meta);
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
      this._isContentWindowCreated = true;
      
      while (this._pendingSetInputMethodActive.length > 0) {
        this._recvSetInputMethodActive(this._pendingSetInputMethodActive.shift());
      }
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
    let documentURI = 
      docShell.QueryInterface(Ci.nsIWebNavigation).currentURI.spec;
    if ((elem instanceof Ci.nsIDOMHTMLAnchorElement && elem.href) ||
        (elem instanceof Ci.nsIDOMHTMLAreaElement && elem.href)) {
      return {uri: elem.href,
              documentURI: documentURI,
              text: elem.textContent.substring(0, kLongestReturnedString)};
    }
    if (elem instanceof Ci.nsIImageLoadingContent && elem.currentURI) {
      return {uri: elem.currentURI.spec, documentURI: documentURI};
    }
    if (elem instanceof Ci.nsIDOMHTMLImageElement) {
      return {uri: elem.src, documentURI: documentURI};
    }
    if (elem instanceof Ci.nsIDOMHTMLMediaElement) {
      let hasVideo = !(elem.readyState >= elem.HAVE_METADATA &&
                       (elem.videoWidth == 0 || elem.videoHeight == 0));
      return {uri: elem.currentSrc || elem.src,
              hasVideo: hasVideo,
              documentURI: documentURI};
    }
    if (elem instanceof Ci.nsIDOMHTMLInputElement &&
        elem.hasAttribute("name")) {
      
      
      let parent = elem.parentNode;
      while (parent) {
        if (parent instanceof Ci.nsIDOMHTMLFormElement &&
            parent.hasAttribute("action")) {
          let actionHref = docShell.QueryInterface(Ci.nsIWebNavigation)
                                   .currentURI
                                   .resolve(parent.getAttribute("action"));
          let method = parent.hasAttribute("method")
            ? parent.getAttribute("method").toLowerCase()
            : "get";
          return {
            documentURI: documentURI,
            action: actionHref,
            method: method,
            name: elem.getAttribute("name"),
          }
        }
        parent = parent.parentNode;
      }
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
    let mimeType = data.json.args.mimeType;
    let domRequestID = data.json.id;

    let takeScreenshotClosure = function() {
      self._takeScreenshot(maxWidth, maxHeight, mimeType, domRequestID);
    };

    let maxDelayMS = 2000;
    try {
      maxDelayMS = Services.prefs.getIntPref('dom.browserElement.maxScreenshotDelayMS');
    }
    catch(e) {}

    
    
    
    Cc['@mozilla.org/message-loop;1'].getService(Ci.nsIMessageLoop).postIdleTask(
      takeScreenshotClosure, maxDelayMS);
  },

  _recvGetContentDimensions: function(data) {
    debug("Received getContentDimensions message: (" + data.json.id + ")");
    sendAsyncMsg('got-contentdimensions', {
      id: data.json.id,
      successRv: this._getContentDimensions()
    });
  },

  _mozScrollAreaChanged: function(e) {
    let dimensions = this._getContentDimensions();
    sendAsyncMsg('scrollareachanged', {
      width: dimensions.width,
      height: dimensions.height
    });
  },

  _getContentDimensions: function() {
    return {
      width: content.document.body.scrollWidth,
      height: content.document.body.scrollHeight
    }
  },

  




  _takeScreenshot: function(maxWidth, maxHeight, mimeType, domRequestID) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    debug("Taking a screenshot: maxWidth=" + maxWidth +
          ", maxHeight=" + maxHeight +
          ", mimeType=" + mimeType +
          ", domRequestID=" + domRequestID + ".");

    if (!content) {
      
      
      debug("No content yet!");
      return;
    }

    let devicePixelRatio = content.devicePixelRatio;

    let maxPixelWidth = Math.round(maxWidth * devicePixelRatio);
    let maxPixelHeight = Math.round(maxHeight * devicePixelRatio);

    let contentPixelWidth = content.innerWidth * devicePixelRatio;
    let contentPixelHeight = content.innerHeight * devicePixelRatio;

    let scaleWidth = Math.min(1, maxPixelWidth / contentPixelWidth);
    let scaleHeight = Math.min(1, maxPixelHeight / contentPixelHeight);

    let scale = Math.max(scaleWidth, scaleHeight);

    let canvasWidth =
      Math.min(maxPixelWidth, Math.round(contentPixelWidth * scale));
    let canvasHeight =
      Math.min(maxPixelHeight, Math.round(contentPixelHeight * scale));

    let transparent = (mimeType !== 'image/jpeg');

    var canvas = content.document
      .createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    if (!transparent)
      canvas.mozOpaque = true;
    canvas.width = canvasWidth;
    canvas.height = canvasHeight;

    let ctx = canvas.getContext("2d", { willReadFrequently: true });
    ctx.scale(scale * devicePixelRatio, scale * devicePixelRatio);

    let flags = ctx.DRAWWINDOW_DRAW_VIEW |
                ctx.DRAWWINDOW_USE_WIDGET_LAYERS |
                ctx.DRAWWINDOW_DO_NOT_FLUSH |
                ctx.DRAWWINDOW_ASYNC_DECODE_IMAGES;
    ctx.drawWindow(content, 0, 0, content.innerWidth, content.innerHeight,
                   transparent ? "rgba(255,255,255,0)" : "rgb(255,255,255)",
                   flags);

    
    
    
    
    canvas.toBlob(function(blob) {
      sendAsyncMsg('got-screenshot', {
        id: domRequestID,
        successRv: blob
      });
    }, mimeType);
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
    var menuObj = {type: 'menu', items: []};
    this._maybeCopyAttribute(menu, menuObj, 'label');

    for (var i = 0, child; child = menu.children[i++];) {
      if (child.nodeName === 'MENU') {
        menuObj.items.push(this._buildMenuObj(child, idPrefix + i + '_'));
      } else if (child.nodeName === 'MENUITEM') {
        var id = this._ctxCounter + '_' + idPrefix + i;
        var menuitem = {id: id, type: 'menuitem'};
        this._maybeCopyAttribute(child, menuitem, 'label');
        this._maybeCopyAttribute(child, menuitem, 'icon');
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
    if (docShell && docShell.isActive !== visible) {
      docShell.isActive = visible;
      sendAsyncMsg('visibilitychange', {visible: visible});
    }
  },

  _recvSendMouseEvent: function(data) {
    let json = data.json;
    let utils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    utils.sendMouseEventToWindow(json.type, json.x, json.y, json.button,
                                 json.clickCount, json.modifiers);
  },

  _recvSendTouchEvent: function(data) {
    let json = data.json;
    let utils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    utils.sendTouchEventToWindow(json.type, json.identifiers, json.touchesX,
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

  _recvZoom: function(data) {
    docShell.contentViewer.fullZoom = data.json.zoom;
  },

  _recvDoCommand: function(data) {
    if (this._isCommandEnabled(data.json.command)) {
      this._selectionStateChangedTarget = null;
      docShell.doCommand(COMMAND_MAP[data.json.command]);
    }
  },

  _recvSetInputMethodActive: function(data) {
    let msgData = { id: data.json.id };
    if (!this._isContentWindowCreated) {
      if (data.json.args.isActive) {
        
        
        this._pendingSetInputMethodActive.push(data);
        return;
      }
      msgData.successRv = null;
      sendAsyncMsg('got-set-input-method-active', msgData);
      return;
    }
    
    let nav = XPCNativeWrapper.unwrap(content.document.defaultView.navigator);
    if (nav.mozInputMethod) {
      
      new XPCNativeWrapper(nav.mozInputMethod).setActive(data.json.args.isActive);
      msgData.successRv = null;
    } else {
      msgData.errorMsg = 'Cannot access mozInputMethod.';
    }
    sendAsyncMsg('got-set-input-method-active', msgData);
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

        switch (status) {
          case Cr.NS_OK :
          case Cr.NS_BINDING_ABORTED :
            
            
            return;

          
          
          
          case Cr.NS_ERROR_UNKNOWN_PROTOCOL :
            sendAsyncMsg('error', { type: 'unknownProtocolFound' });
            return;
          case Cr.NS_ERROR_FILE_NOT_FOUND :
            sendAsyncMsg('error', { type: 'fileNotFound' });
            return;
          case Cr.NS_ERROR_UNKNOWN_HOST :
            sendAsyncMsg('error', { type: 'dnsNotFound' });
            return;
          case Cr.NS_ERROR_CONNECTION_REFUSED :
            sendAsyncMsg('error', { type: 'connectionFailure' });
            return;
          case Cr.NS_ERROR_NET_INTERRUPT :
            sendAsyncMsg('error', { type: 'netInterrupt' });
            return;
          case Cr.NS_ERROR_NET_TIMEOUT :
            sendAsyncMsg('error', { type: 'netTimeout' });
            return;
          case Cr.NS_ERROR_CSP_FRAME_ANCESTOR_VIOLATION :
            sendAsyncMsg('error', { type: 'cspBlocked' });
            return;
          case Cr.NS_ERROR_PHISHING_URI :
            sendAsyncMsg('error', { type: 'phishingBlocked' });
            return;
          case Cr.NS_ERROR_MALWARE_URI :
            sendAsyncMsg('error', { type: 'malwareBlocked' });
            return;
          case Cr.NS_ERROR_UNWANTED_URI :
            sendAsyncMsg('error', { type: 'unwantedBlocked' });
            return;

          case Cr.NS_ERROR_OFFLINE :
            sendAsyncMsg('error', { type: 'offline' });
            return;
          case Cr.NS_ERROR_MALFORMED_URI :
            sendAsyncMsg('error', { type: 'malformedURI' });
            return;
          case Cr.NS_ERROR_REDIRECT_LOOP :
            sendAsyncMsg('error', { type: 'redirectLoop' });
            return;
          case Cr.NS_ERROR_UNKNOWN_SOCKET_TYPE :
            sendAsyncMsg('error', { type: 'unknownSocketType' });
            return;
          case Cr.NS_ERROR_NET_RESET :
            sendAsyncMsg('error', { type: 'netReset' });
            return;
          case Cr.NS_ERROR_DOCUMENT_NOT_CACHED :
            sendAsyncMsg('error', { type: 'notCached' });
            return;
          case Cr.NS_ERROR_DOCUMENT_IS_PRINTMODE :
            sendAsyncMsg('error', { type: 'isprinting' });
            return;
          case Cr.NS_ERROR_PORT_ACCESS_NOT_ALLOWED :
            sendAsyncMsg('error', { type: 'deniedPortAccess' });
            return;
          case Cr.NS_ERROR_UNKNOWN_PROXY_HOST :
            sendAsyncMsg('error', { type: 'proxyResolveFailure' });
            return;
          case Cr.NS_ERROR_PROXY_CONNECTION_REFUSED :
            sendAsyncMsg('error', { type: 'proxyConnectFailure' });
            return;
          case Cr.NS_ERROR_INVALID_CONTENT_ENCODING :
            sendAsyncMsg('error', { type: 'contentEncodingFailure' });
            return;
          case Cr.NS_ERROR_REMOTE_XUL :
            sendAsyncMsg('error', { type: 'remoteXUL' });
            return;
          case Cr.NS_ERROR_UNSAFE_CONTENT_TYPE :
            sendAsyncMsg('error', { type: 'unsafeContentType' });
            return;
          case Cr.NS_ERROR_CORRUPTED_CONTENT :
            sendAsyncMsg('error', { type: 'corruptedContentError' });
            return;

          default:
            
            
            try {
              let nssErrorsService = Cc['@mozilla.org/nss_errors_service;1']
                                       .getService(Ci.nsINSSErrorsService);
              if (nssErrorsService.getErrorClass(status)
                    == Ci.nsINSSErrorsService.ERROR_CLASS_BAD_CERT) {
                
                
                
                
                let errorPage = null;
                try {
                  errorPage = Services.prefs.getCharPref(CERTIFICATE_ERROR_PAGE_PREF);
                } catch (e) {}

                if (errorPage == 'certerror') {
                  sendAsyncMsg('error', { type: 'certerror' });
                  return;
                }
              }
            } catch (e) {}

            sendAsyncMsg('error', { type: 'other' });
            return;
        }
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

