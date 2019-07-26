



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserNewTabPreloader"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const XUL_PAGE = "data:application/vnd.mozilla.xul+xml;charset=utf-8,<window id='win'/>";
const NEWTAB_URL = "about:newtab";
const PREF_BRANCH = "browser.newtab.";



const PRELOADER_INTERVAL_MS = 3000;


const PRELOADER_INIT_DELAY_MS = 5000;



const PRELOADER_UPDATE_DELAY_MS = 3000;

const TOPIC_TIMER_CALLBACK = "timer-callback";
const TOPIC_DELAYED_STARTUP = "browser-delayed-startup-finished";
const TOPIC_XUL_WINDOW_CLOSED = "xul-window-destroyed";

function createTimer(obj, delay) {
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.init(obj, delay, Ci.nsITimer.TYPE_ONE_SHOT);
  return timer;
}

function clearTimer(timer) {
  if (timer) {
    timer.cancel();
  }
  return null;
}

this.BrowserNewTabPreloader = {
  init: function Preloader_init() {
    Initializer.start();
  },

  uninit: function Preloader_uninit() {
    Initializer.stop();
    HostFrame.destroy();
    Preferences.uninit();
    HiddenBrowsers.uninit();
  },

  newTab: function Preloader_newTab(aTab) {
    let win = aTab.ownerDocument.defaultView;
    if (win.gBrowser) {
      let utils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils);

      let {width, height} = utils.getBoundsWithoutFlushing(win.gBrowser);
      let hiddenBrowser = HiddenBrowsers.get(width, height)
      if (hiddenBrowser) {
        return hiddenBrowser.swapWithNewTab(aTab);
      }
    }

    return false;
  }
};

Object.freeze(BrowserNewTabPreloader);

let Initializer = {
  _timer: null,
  _observing: false,

  start: function Initializer_start() {
    Services.obs.addObserver(this, TOPIC_DELAYED_STARTUP, false);
    this._observing = true;
  },

  stop: function Initializer_stop() {
    this._timer = clearTimer(this._timer);

    if (this._observing) {
      Services.obs.removeObserver(this, TOPIC_DELAYED_STARTUP);
      this._observing = false;
    }
  },

  observe: function Initializer_observe(aSubject, aTopic, aData) {
    if (aTopic == TOPIC_DELAYED_STARTUP) {
      Services.obs.removeObserver(this, TOPIC_DELAYED_STARTUP);
      this._observing = false;
      this._startTimer();
    } else if (aTopic == TOPIC_TIMER_CALLBACK) {
      this._timer = null;
      this._startPreloader();
    }
  },

  _startTimer: function Initializer_startTimer() {
    this._timer = createTimer(this, PRELOADER_INIT_DELAY_MS);
  },

  _startPreloader: function Initializer_startPreloader() {
    Preferences.init();
    if (Preferences.enabled) {
      HiddenBrowsers.init();
    }
  }
};

let Preferences = {
  _enabled: null,
  _branch: null,

  get enabled() {
    if (this._enabled === null) {
      this._enabled = this._branch.getBoolPref("preload") &&
                      !this._branch.prefHasUserValue("url");
    }

    return this._enabled;
  },

  init: function Preferences_init() {
    this._branch = Services.prefs.getBranch(PREF_BRANCH);
    this._branch.addObserver("", this, false);
  },

  uninit: function Preferences_uninit() {
    if (this._branch) {
      this._branch.removeObserver("", this);
      this._branch = null;
    }
  },

  observe: function Preferences_observe() {
    let prevEnabled = this._enabled;
    this._enabled = null;

    if (prevEnabled && !this.enabled) {
      HiddenBrowsers.uninit();
    } else if (!prevEnabled && this.enabled) {
      HiddenBrowsers.init();
    }
  },
};

let HiddenBrowsers = {
  _browsers: null,
  _updateTimer: null,

  _topics: [
    TOPIC_DELAYED_STARTUP,
    TOPIC_XUL_WINDOW_CLOSED
  ],

  init: function () {
    this._browsers = new Map();
    this._updateBrowserSizes();
    this._topics.forEach(t => Services.obs.addObserver(this, t, false));
  },

  uninit: function () {
    if (this._browsers) {
      this._topics.forEach(t => Services.obs.removeObserver(this, t, false));
      this._updateTimer = clearTimer(this._updateTimer);

      for (let [key, browser] of this._browsers) {
        browser.destroy();
      }
      this._browsers = null;
    }
  },

  get: function (width, height) {
    
    if (!this._browsers) {
      return null;
    }

    let key = width + "x" + height;
    if (!this._browsers.has(key)) {
      
      this._updateBrowserSizes();
    }

    
    if (this._browsers.has(key)) {
      return this._browsers.get(key);
    }

    
    Cu.reportError("NewTabPreloader: no matching browser found after updating");
    for (let [size, browser] of this._browsers) {
      return browser;
    }

    
    Cu.reportError("NewTabPreloader: not even a single browser was found?");
    return null;
  },

  observe: function (subject, topic, data) {
    if (topic === TOPIC_TIMER_CALLBACK) {
      this._updateTimer = null;
      this._updateBrowserSizes();
    } else {
      this._updateTimer = clearTimer(this._updateTimer);
      this._updateTimer = createTimer(this, PRELOADER_UPDATE_DELAY_MS);
    }
  },

  _updateBrowserSizes: function () {
    let sizes = this._collectTabBrowserSizes();
    let toRemove = [];

    
    
    for (let [key, browser] of this._browsers) {
      if (sizes.has(key)) {
        
        sizes.delete(key);
      } else {
        
        toRemove.push(browser);
        this._browsers.delete(key);
      }
    }

    
    for (let [key, {width, height}] of sizes) {
      let browser;
      if (toRemove.length) {
        
        
        browser = toRemove.shift();
        browser.resize(width, height);
      } else {
        
        browser = new HiddenBrowser(width, height);
      }

      this._browsers.set(key, browser);
    }

    
    toRemove.forEach(b => b.destroy());
  },

  _collectTabBrowserSizes: function () {
    let sizes = new Map();

    function tabBrowsers() {
      let wins = Services.ww.getWindowEnumerator("navigator:browser");
      while (wins.hasMoreElements()) {
        let win = wins.getNext();
        if (win.gBrowser) {
          yield win.gBrowser;
        }
      }
    }

    
    for (let {boxObject: {width, height}} of tabBrowsers()) {
      if (width > 0 && height > 0) {
        let key = width + "x" + height;
        if (!sizes.has(key)) {
          sizes.set(key, {width: width, height: height});
        }
      }
    }

    return sizes;
  }
};

function HiddenBrowser(width, height) {
  this.resize(width, height);

  HostFrame.get(aFrame => {
    let doc = aFrame.document;
    this._browser = doc.createElementNS(XUL_NS, "browser");
    this._browser.setAttribute("type", "content");
    doc.getElementById("win").appendChild(this._browser);
    this.preload();
  });
}

HiddenBrowser.prototype = {
  _width: null,
  _height: null,
  _timer: null,

  get isPreloaded() {
    return this._browser &&
           this._browser.contentDocument &&
           this._browser.contentDocument.readyState === "complete" &&
           this._browser.currentURI.spec === NEWTAB_URL;
  },

  swapWithNewTab: function (aTab) {
    if (!this.isPreloaded || this._timer) {
      return false;
    }

    let tabbrowser = aTab.ownerDocument.defaultView.gBrowser;
    if (!tabbrowser) {
      return false;
    }

    
    tabbrowser.swapNewTabWithBrowser(aTab, this._browser);

    
    this._timer = createTimer(this, PRELOADER_INTERVAL_MS);

    
    return true;
  },

  observe: function () {
    this._timer = null;
    this.preload();
  },

  preload: function () {
    if (!this._browser) {
      return;
    }

    
    this.resize(this._width, this._height);

    
    this._browser.loadURI(NEWTAB_URL);
  },

  resize: function (width, height) {
    if (this._browser) {
      this._browser.style.width = width + "px";
      this._browser.style.height = height + "px";
    } else {
      this._width = width;
      this._height = height;
    }
  },

  destroy: function () {
    if (this._browser) {
      this._browser.remove();
      this._browser = null;
    }

    this._timer = clearTimer(this._timer);
  }
};

let HostFrame = {
  _frame: null,
  _loading: false,

  get hiddenDOMDocument() {
    return Services.appShell.hiddenDOMWindow.document;
  },

  get isReady() {
    return this.hiddenDOMDocument.readyState === "complete";
  },

  get: function (callback) {
    if (this._frame) {
      callback(this._frame);
    } else if (this.isReady && !this._loading) {
      this._create(callback);
      this._loading = true;
    } else {
      Services.tm.currentThread.dispatch(() => HostFrame.get(callback),
                                         Ci.nsIThread.DISPATCH_NORMAL);
    }
  },

  destroy: function () {
    this._frame = null;
  },

  _create: function (callback) {
    let doc = this.hiddenDOMDocument;
    let iframe = doc.createElementNS(HTML_NS, "iframe");
    doc.documentElement.appendChild(iframe);

    let frame = iframe.contentWindow;
    let docShell = frame.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDocShell);

    docShell.createAboutBlankContentViewer(null);
    frame.location = XUL_PAGE;

    let eventHandler = docShell.chromeEventHandler;
    eventHandler.addEventListener("DOMContentLoaded", function onLoad() {
      eventHandler.removeEventListener("DOMContentLoaded", onLoad, false);
      callback(HostFrame._frame = frame);
    }, false);
  }
};
