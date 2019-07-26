










const EXPORTED_SYMBOLS = [
  "BackgroundPageThumbs",
];

const DEFAULT_CAPTURE_TIMEOUT = 30000; 
const DESTROY_BROWSER_TIMEOUT = 60000; 
const FRAME_SCRIPT_URL = "chrome://global/content/backgroundPageThumbsContent.js";




const MAX_THUMBNAIL_AGE_SECS = 172800; 

const TELEMETRY_HISTOGRAM_ID_PREFIX = "FX_THUMBNAILS_BG_";


const TEL_CAPTURE_DONE_OK = 0;
const TEL_CAPTURE_DONE_TIMEOUT = 1;


const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const HTML_NS = "http://www.w3.org/1999/xhtml";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/PageThumbs.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const BackgroundPageThumbs = {

  




















  capture: function (url, options={}) {
    if (!PageThumbs._prefEnabled()) {
      if (options.onDone)
        schedule(() => options.onDone(null));
      return;
    }
    this._captureQueue = this._captureQueue || [];
    this._capturesByURL = this._capturesByURL || new Map();

    tel("QUEUE_SIZE_ON_CAPTURE", this._captureQueue.length);

    
    
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

  observe: function(subject, topic, data) {
    
    if (this._thumbBrowser) {
      let frameLoader = subject.QueryInterface(Ci.nsIFrameLoader);
      if (this._thumbBrowser.messageManager == frameLoader.messageManager) {
        Cu.reportError("BackgroundThumbnails remote process crashed - recovering");
        this._destroyBrowser();
        let curCapture = this._captureQueue.length ? this._captureQueue[0] : null;
        
        
        
        
        
        if (curCapture && curCapture.pending) {
          curCapture._done(null);
          
        }
        
        
        
      }
    }
  },

  















  captureIfStale: function PageThumbs_captureIfStale(url, options={}) {
    PageThumbsStorage.isFileRecentForURL(url, MAX_THUMBNAIL_AGE_SECS).then(
      result => {
        if (result.ok) {
          if (options.onDone)
            options.onDone(url);
          return;
        }
        this.capture(url, options);
      }, err => {
        if (options.onDone)
          options.onDone(url);
      });
  },

  






  _ensureParentWindowReady: function () {
    if (this._parentWin)
      
      return true;
    if (this._startedParentWinInit)
      
      return false;

    this._startedParentWinInit = true;

    
    
    
    let hostWindow = Services.appShell.hiddenDOMWindow;
    let iframe = hostWindow.document.createElementNS(HTML_NS, "iframe");
    iframe.setAttribute("src", "chrome://global/content/mozilla.xhtml");
    let onLoad = function onLoadFn() {
      iframe.removeEventListener("load", onLoad, true);
      this._parentWin = iframe.contentWindow;
      this._processCaptureQueue();
    }.bind(this);
    iframe.addEventListener("load", onLoad, true);
    hostWindow.document.documentElement.appendChild(iframe);
    this._hostIframe = iframe;

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

    
    
    
    let [swidth, sheight] = [{}, {}];
    Cc["@mozilla.org/gfx/screenmanager;1"].
      getService(Ci.nsIScreenManager).
      primaryScreen.
      GetRectDisplayPix({}, {}, swidth, sheight);
    let bwidth = Math.min(1024, swidth.value);
    
    
    browser.style.width = bwidth + "px";
    browser.style.height = (bwidth * sheight.value / swidth.value) + "px";

    this._parentWin.document.documentElement.appendChild(browser);

    browser.messageManager.loadFrameScript(FRAME_SCRIPT_URL, false);
    this._thumbBrowser = browser;
    
    
    
    
    
    Services.obs.addObserver(this, "oop-frameloader-crashed", false);
  },

  _destroyBrowser: function () {
    if (!this._thumbBrowser)
      return;
    Services.obs.removeObserver(this, "oop-frameloader-crashed");
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
  this.creationDate = new Date();
  this.doneCallbacks = [];
  if (options.onDone)
    this.doneCallbacks.push(options.onDone);
}

Capture.prototype = {

  get pending() {
    return !!this._msgMan;
  },

  




  start: function (messageManager) {
    this.startDate = new Date();
    tel("CAPTURE_QUEUE_TIME_MS", this.startDate - this.creationDate);

    
    let timeout = typeof(this.options.timeout) == "number" ?
                  this.options.timeout :
                  DEFAULT_CAPTURE_TIMEOUT;
    this._timeoutTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timeoutTimer.initWithCallback(this, timeout,
                                        Ci.nsITimer.TYPE_ONE_SHOT);

    
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
    Services.ww.unregisterNotification(this);
  },

  
  receiveMessage: function (msg) {
    tel("CAPTURE_DONE_REASON", TEL_CAPTURE_DONE_OK);
    tel("CAPTURE_SERVICE_TIME_MS", new Date() - this.startDate);

    
    
    if (msg.json.id == this.id)
      this._done(msg.json);
  },

  
  notify: function () {
    tel("CAPTURE_DONE_REASON", TEL_CAPTURE_DONE_TIMEOUT);
    this._done(null);
  },

  _done: function (data) {
    
    
    

    this.captureCallback(this);
    this.destroy();

    if (data && data.telemetry) {
      
      for (let id in data.telemetry) {
        tel(id, data.telemetry[id]);
      }
    }

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

    PageThumbs._store(this.url, data.finalURL, data.imageData, data.wasErrorResponse)
              .then(callOnDones);
  },
};

Capture.nextID = 0;







function tel(histogramID, value) {
  let id = TELEMETRY_HISTOGRAM_ID_PREFIX + histogramID;
  Services.telemetry.getHistogramById(id).add(value);
}

function schedule(callback) {
  Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
}
