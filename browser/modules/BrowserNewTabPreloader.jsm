



"use strict";

let EXPORTED_SYMBOLS = ["BrowserNewTabPreloader"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const PREF_BRANCH = "browser.newtab.";

let BrowserNewTabPreloader =  {
  init: function Preloader_init() {
    Preferences.init();

    if (Preferences.enabled) {
      HiddenBrowser.create();
    }
  },

  uninit: function Preloader_uninit() {
    HostFrame.destroy();
    Preferences.uninit();
    HiddenBrowser.destroy();
  },

  newTab: function Preloader_newTab(aTab) {
    HiddenBrowser.swapWithNewTab(aTab);
  }
};

Object.freeze(BrowserNewTabPreloader);

let Preferences = {
  _enabled: null,
  _branch: null,
  _url: null,

  get enabled() {
    if (this._enabled === null) {
      this._enabled = this._branch.getBoolPref("preload") &&
                      !this._branch.prefHasUserValue("url") &&
                      this.url && this.url != "about:blank";
    }

    return this._enabled;
  },

  get url() {
    if (this._url === null) {
      this._url = this._branch.getCharPref("url");
    }

    return this._url;
  },

  init: function Preferences_init() {
    this._branch = Services.prefs.getBranch(PREF_BRANCH);
    this._branch.addObserver("", this, false);
  },

  uninit: function Preferences_uninit() {
    this._branch.removeObserver("", this);
    this._branch = null;
  },

  observe: function Preferences_observe(aSubject, aTopic, aData) {
    let {url, enabled} = this;
    this._url = this._enabled = null;

    if (enabled && !this.enabled) {
      HiddenBrowser.destroy();
    } else if (!enabled && this.enabled) {
      HiddenBrowser.create();
    } else if (this._browser && url != this.url) {
      HiddenBrowser.update(this.url);
    }
  },
};

let HiddenBrowser = {
  get isPreloaded() {
    return this._browser &&
           this._browser.contentDocument &&
           this._browser.contentDocument.readyState == "complete" &&
           this._browser.currentURI.spec == Preferences.url;
  },

  swapWithNewTab: function HiddenBrowser_swapWithNewTab(aTab) {
    if (this.isPreloaded) {
      let tabbrowser = aTab.ownerDocument.defaultView.gBrowser;
      if (tabbrowser) {
        tabbrowser.swapNewTabWithBrowser(aTab, this._browser);
      }
    }
  },

  create: function HiddenBrowser_create() {
    HostFrame.getFrame(function (aFrame) {
      let doc = aFrame.document;
      this._browser = doc.createElementNS(XUL_NS, "browser");
      this._browser.setAttribute("type", "content");
      this._browser.setAttribute("src", Preferences.url);
      doc.documentElement.appendChild(this._browser);
    }.bind(this));
  },

  update: function HiddenBrowser_update(aURL) {
    this._browser.setAttribute("src", aURL);
  },

  destroy: function HiddenBrowser_destroy() {
    if (this._browser) {
      this._browser.parentNode.removeChild(this._browser);
      this._browser = null;
    }
  }
};

let HostFrame = {
  _listener: null,
  _privilegedFrame: null,

  _privilegedContentTypes: {
    "application/vnd.mozilla.xul+xml": true,
    "application/xhtml+xml": true
  },

  get _frame() {
    delete this._frame;
    return this._frame = Services.appShell.hiddenDOMWindow;
  },

  get _isReady() {
    let readyState = this._frame.document.readyState;
    return (readyState == "complete" || readyState == "interactive");
  },

  get _isPrivileged() {
    return (this._frame.location.protocol == "chrome:" &&
            this._frame.document.contentType in this._privilegedContentTypes);
  },

  getFrame: function HostFrame_getFrame(aCallback) {
    if (this._isReady && !this._isPrivileged) {
      this._createPrivilegedFrame();
    }

    if (this._isReady) {
      aCallback(this._frame);
    } else {
      this._waitUntilLoaded(aCallback);
    }
  },

  destroy: function HostFrame_destroy() {
    delete this._frame;
    this._listener = null;
  },

  _createPrivilegedFrame: function HostFrame_createPrivilegedFrame() {
    let doc = this._frame.document;
    let iframe = doc.createElement("iframe");
    iframe.setAttribute("src", "chrome://browser/content/newtab/preload.xhtml");
    doc.documentElement.appendChild(iframe);
    this._frame = iframe.contentWindow;
  },

  _waitUntilLoaded: function HostFrame_waitUntilLoaded(aCallback) {
    this._listener = new HiddenWindowLoadListener(this._frame, function () {
      HostFrame.getFrame(aCallback);
    });
  }
};


function HiddenWindowLoadListener(aWindow, aCallback) {
  this._window = aWindow;
  this._callback = aCallback;

  let docShell = Services.appShell.hiddenWindow.docShell;
  this._webProgress = docShell.QueryInterface(Ci.nsIWebProgress);
  this._webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_ALL);
}

HiddenWindowLoadListener.prototype = {
  _window: null,
  _callback: null,
  _webProgress: null,

  _destroy: function HiddenWindowLoadListener_destroy() {
    this._webProgress.removeProgressListener(this);
    this._window = null;
    this._callback = null;
    this._webProgress = null;
  },

  onStateChange:
  function HiddenWindowLoadListener_onStateChange(aWebProgress, aRequest,
                                                  aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW &&
        this._window == aWebProgress.DOMWindow) {
      this._callback();
      this._destroy();
    }
  },

  onStatusChange: function () {},
  onLocationChange: function () {},
  onProgressChange: function () {},
  onSecurityChange: function () {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};
