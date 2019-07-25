



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
let Cr = Components.results;

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
  sendAsyncMessage('browser-element-api:' + msg, data);
}

function sendSyncMsg(msg, data) {
  return sendSyncMessage('browser-element-api:' + msg, data);
}












var global = this;

function BrowserElementChild() {
  
  this._windowIDDict = {};

  this._init();
};

BrowserElementChild.prototype = {
  _init: function() {
    debug("Starting up.");
    sendAsyncMsg("hello");

    BrowserElementPromptService.mapWindowToBrowserElementChild(content, this);

    docShell.isBrowserFrame = true;
    docShell.QueryInterface(Ci.nsIWebProgress)
            .addProgressListener(this._progressListener,
                                 Ci.nsIWebProgress.NOTIFY_LOCATION |
                                 Ci.nsIWebProgress.NOTIFY_SECURITY |
                                 Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);

    
    var securityUI = Cc['@mozilla.org/secure_browser_ui;1']
                       .createInstance(Ci.nsISecureBrowserUI);
    securityUI.init(content);

    
    
    
    
    
    
    
    
    
    let appManifestURL = sendSyncMsg('get-mozapp-manifest-url')[0];
    let windowUtils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                             .getInterface(Ci.nsIDOMWindowUtils);

    if (!!appManifestURL) {
      windowUtils.setIsApp(true);
      windowUtils.setApp(appManifestURL);
    } else {
      windowUtils.setIsApp(false);
    }

    addEventListener('DOMTitleChanged',
                     this._titleChangedHandler.bind(this),
                      true,
                      false);

    addEventListener('DOMLinkAdded',
                     this._iconChangedHandler.bind(this),
                      true,
                      false);

    var self = this;
    function addMsgListener(msg, handler) {
      addMessageListener('browser-element-api:' + msg, handler.bind(self));
    }

    addMsgListener("get-screenshot", this._recvGetScreenshot);
    addMsgListener("set-visible", this._recvSetVisible);
    addMsgListener("unblock-modal-prompt", this._recvStopWaiting);

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
                               this._closeHandler.bind(this),
                                false);
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
        args.promptType == 'confirm') {
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

    
    
    
    
    
    var modalStateWin = utils.enterModalStateWithWindow();

    
    
    if (!win.modalDepth) {
      win.modalDepth = 0;
    }
    win.modalDepth++;
    let origModalDepth = win.modalDepth;

    let thread = Services.tm.currentThread;
    debug("Nested event loop - begin");
    while (win.modalDepth == origModalDepth) {
      
      
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

    utils.leaveModalStateWithWindow(modalStateWin);

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

  _titleChangedHandler: function(e) {
    debug("Got titlechanged: (" + e.target.title + ")");
    var win = e.target.defaultView;

    
    
    if (win == content) {
      sendAsyncMsg('titlechange', e.target.title);
    }
    else {
      debug("Not top level!");
    }
  },

  _iconChangedHandler: function(e) {
    debug("Got iconchanged: (" + e.target.href + ")");
    var hasIcon = e.target.rel.split(' ').some(function(x) {
      return x.toLowerCase() === 'icon';
    });

    if (hasIcon) {
      var win = e.target.ownerDocument.defaultView;
      
      
      if (win == content) {
        sendAsyncMsg('iconchange', e.target.href);
      }
      else {
        debug("Not top level!");
      }
    }
  },

  _closeHandler: function(e) {
    let win = e.target;
    if (win != content || e.defaultPrevented) {
      return;
    }

    debug("Closing window " + win);
    sendAsyncMsg('close');

    
    e.preventDefault();
  },

  _recvGetScreenshot: function(data) {
    debug("Received getScreenshot message: (" + data.json.id + ")");
    var canvas = content.document
      .createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    var ctx = canvas.getContext("2d");
    canvas.mozOpaque = true;
    canvas.height = content.innerHeight;
    canvas.width = content.innerWidth;
    ctx.drawWindow(content, 0, 0, content.innerWidth,
                   content.innerHeight, "rgb(255,255,255)");
    sendAsyncMsg('got-screenshot', {
      id: data.json.id,
      screenshot: canvas.toDataURL("image/png")
    });
  },

  _recvSetVisible: function(data) {
    debug("Received setVisible message: (" + data.json.visible + ")");
    if (docShell.isActive !== data.json.visible) {
      docShell.isActive = data.json.visible;
    }
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

      sendAsyncMsg('locationchange', location.spec);
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
        sendAsyncMsg('loadend');
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

      sendAsyncMsg('securitychange', {state: stateDesc, extendedValidation: isEV});
    },

    onStatusChange: function(webProgress, request, status, message) {},
    onProgressChange: function(webProgress, request, curSelfProgress,
                               maxSelfProgress, curTotalProgress, maxTotalProgress) {},
  },
};

var api = new BrowserElementChild();
