



"use strict";

this.EXPORTED_SYMBOLS = ["SelfSupportBackend"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HiddenFrame",
  "resource:///modules/HiddenFrame.jsm");


const PREF_ENABLED = "browser.selfsupport.enabled";

const PREF_URL = "browser.selfsupport.url";

const PREF_FHR_ENABLED = "datareporting.healthreport.service.enabled";

const PREF_UITOUR_ENABLED = "browser.uitour.enabled";



const RETRY_INTERVAL_MS = 30000;

const MAX_RETRIES = 5;

const STARTUP_DELAY_MS = 5000;

const LOGGER_NAME = "Browser.SelfSupportBackend";
const PREF_BRANCH_LOG = "browser.selfsupport.log.";
const PREF_LOG_LEVEL = PREF_BRANCH_LOG + "level";
const PREF_LOG_DUMP = PREF_BRANCH_LOG + "dump";

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const UITOUR_FRAME_SCRIPT = "chrome://browser/content/content-UITour.js";

let gLogAppenderDump = null;

this.SelfSupportBackend = Object.freeze({
  init: function () {
    SelfSupportBackendInternal.init();
  },

  uninit: function () {
    SelfSupportBackendInternal.uninit();
  },
});

let SelfSupportBackendInternal = {
  
  _browser: null,
  
  _delayedLoadTimerId: null,
  
  _frame: null,
  _log: null,
  _progressListener: null,

  


  init: function () {
    this._configureLogging();

    this._log.trace("init");

    Preferences.observe(PREF_BRANCH_LOG, this._configureLogging, this);

    
    let fhrEnabled = Preferences.get(PREF_FHR_ENABLED, false);
    if (!fhrEnabled) {
      this._log.config("init - Disabling SelfSupport because Health Report is disabled.");
      return;
    }

    
    let uiTourEnabled = Preferences.get(PREF_UITOUR_ENABLED, false);
    if (!uiTourEnabled) {
      this._log.config("init - Disabling SelfSupport because UITour is disabled.");
      return;
    }

    
    if (!Preferences.get(PREF_ENABLED, true)) {
      this._log.config("init - SelfSupport is disabled.");
      return;
    }

    Services.obs.addObserver(this, "sessionstore-windows-restored", false);
  },

  


  uninit: function () {
    this._log.trace("uninit");

    Preferences.ignore(PREF_BRANCH_LOG, this._configureLogging, this);

    
    clearTimeout(this._delayedLoadTimerId);

    
    if (this._browser !== null) {
      if (this._browser.contentWindow) {
        this._browser.contentWindow.removeEventListener("DOMWindowClose", this, true);
      }

      if (this._progressListener) {
        this._browser.removeProgressListener(this._progressListener);
        this._progressListener.destroy();
        this._progressListener = null;
      }

      this._browser.remove();
      this._browser = null;
    }

    if (this._frame) {
      this._frame.destroy();
      this._frame = null;
    }
  },

  



  observe: function (aSubject, aTopic, aData) {
    this._log.trace("observe - Topic " + aTopic);

    if (aTopic === "sessionstore-windows-restored") {
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      this._delayedLoadTimerId = setTimeout(this._loadSelfSupport.bind(this), STARTUP_DELAY_MS);
    }
  },

  


  _configureLogging: function() {
    if (!this._log) {
      this._log = Log.repository.getLogger(LOGGER_NAME);

      
      let consoleAppender = new Log.ConsoleAppender(new Log.BasicFormatter());
      this._log.addAppender(consoleAppender);
    }

    
    this._log.level = Log.Level[Preferences.get(PREF_LOG_LEVEL, "Warn")];

    
    let logDumping = Preferences.get(PREF_LOG_DUMP, false);
    if (logDumping != !!gLogAppenderDump) {
      if (logDumping) {
        gLogAppenderDump = new Log.DumpAppender(new Log.BasicFormatter());
        this._log.addAppender(gLogAppenderDump);
      } else {
        this._log.removeAppender(gLogAppenderDump);
        gLogAppenderDump = null;
      }
    }
  },

  



  _makeHiddenBrowser: function(aURL) {
    this._frame = new HiddenFrame();
    return this._frame.get().then(aFrame => {
      let doc = aFrame.document;

      this._browser = doc.createElementNS(XUL_NS, "browser");
      this._browser.setAttribute("type", "content");
      this._browser.setAttribute("disableglobalhistory", "true");
      this._browser.setAttribute("src", aURL);

      doc.documentElement.appendChild(this._browser);
    });
  },

  handleEvent: function(aEvent) {
    this._log.trace("handleEvent - aEvent.type " + aEvent.type + ", Trusted " + aEvent.isTrusted);

    if (aEvent.type === "DOMWindowClose") {
      let window = this._browser.contentDocument.defaultView;
      let target = aEvent.target;

      if (target == window) {
        
        
        aEvent.preventDefault();

        this.uninit();
      }
    }
  },

  


  _pageSuccessCallback: function() {
    this._log.debug("_pageSuccessCallback - Page correctly loaded.");
    this._browser.removeProgressListener(this._progressListener);
    this._progressListener.destroy();
    this._progressListener = null;

    
    this._browser.contentWindow.addEventListener("DOMWindowClose", this, true);
  },

  


  _pageLoadErrorCallback: function() {
    this._log.info("_pageLoadErrorCallback - Too many failed load attempts. Giving up.");
    this.uninit();
  },

  




  _loadSelfSupport: function() {
    
    let unformattedURL = Preferences.get(PREF_URL, null);
    let url = Services.urlFormatter.formatURL(unformattedURL);
    if (!url.startsWith("https:")) {
      this._log.error("_loadSelfSupport - Non HTTPS URL provided: " + url);
      return;
    }

    this._log.config("_loadSelfSupport - URL " + url);

    
    this._makeHiddenBrowser(url).then(() => {
      
      this._browser.messageManager.loadFrameScript(UITOUR_FRAME_SCRIPT, true);

      
      
      const webFlags = Ci.nsIWebProgress.NOTIFY_STATE_WINDOW |
                       Ci.nsIWebProgress.NOTIFY_STATE_REQUEST |
                       Ci.nsIWebProgress.NOTIFY_LOCATION;

      this._progressListener = new ProgressListener(() => this._pageLoadErrorCallback(),
                                                    () => this._pageSuccessCallback());

      this._browser.addProgressListener(this._progressListener, webFlags);
    });
  }
};










function ProgressListener(aLoadErrorCallback, aLoadSuccessCallback) {
  this._loadErrorCallback = aLoadErrorCallback;
  this._loadSuccessCallback = aLoadSuccessCallback;
  
  this._loadAttempts = 0;
  this._log = Log.repository.getLogger(LOGGER_NAME);
  
  this._reloadTimerId = null;
}

ProgressListener.prototype = {
  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {
    if (aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE) {
      this._log.warn("onLocationChange - There was a problem fetching the SelfSupport URL (attempt " +
                     this._loadAttempts + ").");

      
      this._loadAttempts++;
      if (this._loadAttempts > MAX_RETRIES) {
        this._loadErrorCallback();
        return;
      }

      
      
      
      this._reloadTimerId = setTimeout(() => {
        this._log.debug("onLocationChange - Reloading SelfSupport URL in the hidden browser.");
        aWebProgress.DOMWindow.location.reload();
      }, RETRY_INTERVAL_MS * this._loadAttempts);
    }
  },

  onStateChange: function (aWebProgress, aRequest, aFlags, aStatus) {
    if (aFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        aFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
        aFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW &&
        Components.isSuccessCode(aStatus)) {
      this._loadSuccessCallback();
    }
  },

  destroy: function () {
    
    clearTimeout(this._reloadTimerId);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference]),
};
