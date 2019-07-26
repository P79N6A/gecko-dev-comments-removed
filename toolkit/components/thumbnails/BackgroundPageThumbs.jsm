



const EXPORTED_SYMBOLS = [
  "BackgroundPageThumbs",
];

const DEFAULT_CAPTURE_TIMEOUT = 30000; 
const DESTROY_BROWSER_TIMEOUT = 60000; 
const FRAME_SCRIPT_URL = "chrome://global/content/backgroundPageThumbsContent.js";

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const HTML_NS = "http://www.w3.org/1999/xhtml";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/PageThumbs.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

const BackgroundPageThumbs = {

  















  capture: function (url, options={}) {
    if (isPrivateBrowsingActive()) {
      
      
      
      
      
      
      if (options.onDone)
        Services.tm.mainThread.dispatch(options.onDone.bind(options, url), 0);
      return;
    }
    this._captureQueue = this._captureQueue || [];
    this._capturesByURL = this._capturesByURL || new Map();
    
    
    let existing = this._capturesByURL.get(url);
    if (existing) {
      if (options.onDone)
        existing.doneCallbacks.push(options.onDone);
      
      return;
    }
    let cap = new Capture(url, this._onCaptureOrTimeout.bind(this), options);
    this._captureQueue.push(cap);
    this._capturesByURL.set(url, cap);
    this._processCaptureQueue();
  },

  






  _ensureParentWindowReady: function () {
    if (this._parentWin)
      
      return true;
    if (this._startedParentWinInit)
      
      return false;

    this._startedParentWinInit = true;

    PrivateBrowsingUtils.whenHiddenPrivateWindowReady(function (parentWin) {
      parentWin.addEventListener("unload", function (event) {
        if (event.target == parentWin.document)
          this._destroy();
      }.bind(this), true);

      if (canHostBrowser(parentWin)) {
        this._parentWin = parentWin;
        this._processCaptureQueue();
        return;
      }

      
      
      
      let iframe = parentWin.document.createElementNS(HTML_NS, "iframe");
      iframe.setAttribute("src", "chrome://global/content/mozilla.xhtml");
      let onLoad = function onLoadFn() {
        iframe.removeEventListener("load", onLoad, true);
        this._parentWin = iframe.contentWindow;
        this._processCaptureQueue();
      }.bind(this);
      iframe.addEventListener("load", onLoad, true);
      parentWin.document.documentElement.appendChild(iframe);
      this._hostIframe = iframe;
    }.bind(this));

    return false;
  },

  



  _destroy: function () {
    if (this._captureQueue)
      this._captureQueue.forEach(cap => cap.destroy());
    this._destroyBrowser();
    if (this._hostIframe)
      this._hostIframe.remove();
    delete this._captureQueue;
    delete this._hostIframe;
    delete this._startedParentWinInit;
    delete this._parentWin;
  },

  


  _ensureBrowser: function () {
    if (this._thumbBrowser)
      return;

    let browser = this._parentWin.document.createElementNS(XUL_NS, "browser");
    browser.setAttribute("type", "content");
    browser.setAttribute("remote", "true");
    browser.setAttribute("privatebrowsing", "true");

    
    
    
    let width = {};
    let height = {};
    Cc["@mozilla.org/gfx/screenmanager;1"].
      getService(Ci.nsIScreenManager).
      primaryScreen.
      GetRectDisplayPix({}, {}, width, height);
    browser.style.width = width.value + "px";
    browser.style.height = height.value + "px";

    this._parentWin.document.documentElement.appendChild(browser);

    browser.messageManager.loadFrameScript(FRAME_SCRIPT_URL, false);
    this._thumbBrowser = browser;
  },

  _destroyBrowser: function () {
    if (!this._thumbBrowser)
      return;
    this._thumbBrowser.remove();
    delete this._thumbBrowser;
  },

  



  _processCaptureQueue: function () {
    if (!this._captureQueue.length ||
        this._captureQueue[0].pending ||
        !this._ensureParentWindowReady())
      return;

    
    this._ensureBrowser();
    this._captureQueue[0].start(this._thumbBrowser.messageManager);
    if (this._destroyBrowserTimer) {
      this._destroyBrowserTimer.cancel();
      delete this._destroyBrowserTimer;
    }
  },

  


  _onCaptureOrTimeout: function (capture) {
    
    
    if (capture !== this._captureQueue[0])
      throw new Error("The capture should be at the head of the queue.");
    this._captureQueue.shift();
    this._capturesByURL.delete(capture.url);

    
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this._destroyBrowser.bind(this),
                           this._destroyBrowserTimeout,
                           Ci.nsITimer.TYPE_ONE_SHOT);
    this._destroyBrowserTimer = timer;

    this._processCaptureQueue();
  },

  _destroyBrowserTimeout: DESTROY_BROWSER_TIMEOUT,
};









function Capture(url, captureCallback, options) {
  this.url = url;
  this.captureCallback = captureCallback;
  this.options = options;
  this.id = Capture.nextID++;
  this.doneCallbacks = [];
  if (options.onDone)
    this.doneCallbacks.push(options.onDone);
}

Capture.prototype = {

  get pending() {
    return !!this._msgMan;
  },

  




  start: function (messageManager) {
    let timeout = typeof(this.options.timeout) == "number" ? this.options.timeout :
                  DEFAULT_CAPTURE_TIMEOUT;
    this._timeoutTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timeoutTimer.initWithCallback(this, timeout, Ci.nsITimer.TYPE_ONE_SHOT);

    this._msgMan = messageManager;
    this._msgMan.sendAsyncMessage("BackgroundPageThumbs:capture",
                                  { id: this.id, url: this.url });
    this._msgMan.addMessageListener("BackgroundPageThumbs:didCapture", this);
  },

  




  destroy: function () {
    
    
    if (this._timeoutTimer) {
      this._timeoutTimer.cancel();
      delete this._timeoutTimer;
    }
    if (this._msgMan) {
      this._msgMan.removeMessageListener("BackgroundPageThumbs:didCapture",
                                         this);
      delete this._msgMan;
    }
    delete this.captureCallback;
  },

  
  receiveMessage: function (msg) {
    
    
    if (msg.json.id == this.id)
      this._done(msg.json);
  },

  
  notify: function () {
    this._done(null);
  },

  _done: function (data) {
    
    
    

    this.captureCallback(this);
    this.destroy();

    let callOnDones = function callOnDonesFn() {
      for (let callback of this.doneCallbacks) {
        try {
          callback.call(this.options, this.url);
        }
        catch (err) {
          Cu.reportError(err);
        }
      }
    }.bind(this);

    if (!data) {
      callOnDones();
      return;
    }
    PageThumbs._store(this.url, data.finalURL, data.imageData).then(callOnDones);
  },
};

Capture.nextID = 0;







function canHostBrowser(win) {
  
  
  
  
  
  
  let principal = win.document.nodePrincipal;
  if (!Services.scriptSecurityManager.isSystemPrincipal(principal))
    return false;
  let permResult = Services.perms.testPermissionFromPrincipal(principal,
                                                              "allowXULXBL");
  return permResult == Ci.nsIPermissionManager.ALLOW_ACTION;
}




function isPrivateBrowsingActive() {
  let wins = Services.ww.getWindowEnumerator();
  while (wins.hasMoreElements())
    if (PrivateBrowsingUtils.isWindowPrivate(wins.getNext()))
      return true;
  return false;
}
