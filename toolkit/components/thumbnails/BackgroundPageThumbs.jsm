



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
    let cap = new Capture(url, this._onCaptureOrTimeout.bind(this), options);
    this._captureQueue = this._captureQueue || [];
    this._captureQueue.push(cap);
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
    
    
    
    let idx = this._captureQueue.indexOf(capture);
    if (idx < 0)
      throw new Error("The capture should be in the queue.");
    this._captureQueue.splice(idx, 1);

    
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

  
  
  let timeout = typeof(options.timeout) == "number" ? options.timeout :
                DEFAULT_CAPTURE_TIMEOUT;
  this._timeoutTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._timeoutTimer.initWithCallback(this, timeout, Ci.nsITimer.TYPE_ONE_SHOT);
}

Capture.prototype = {

  get pending() {
    return !!this._msgMan;
  },

  




  start: function (messageManager) {
    this._msgMan = messageManager;
    this._msgMan.sendAsyncMessage("BackgroundPageThumbs:capture",
                                  { id: this.id, url: this.url });
    this._msgMan.addMessageListener("BackgroundPageThumbs:didCapture", this);
  },

  







  destroy: function () {
    this._timeoutTimer.cancel();
    delete this._timeoutTimer;
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

    let callOnDone = function callOnDoneFn() {
      if (!("onDone" in this.options))
        return;
      try {
        this.options.onDone(this.url);
      }
      catch (err) {
        Cu.reportError(err);
      }
    }.bind(this);

    if (!data) {
      callOnDone();
      return;
    }
    PageThumbs._store(this.url, data.finalURL, data.imageData).then(callOnDone);
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
