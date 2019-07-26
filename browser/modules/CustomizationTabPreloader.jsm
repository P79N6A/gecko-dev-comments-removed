



"use strict";

this.EXPORTED_SYMBOLS = ["CustomizationTabPreloader"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const XUL_PAGE = "data:application/vnd.mozilla.xul+xml;charset=utf-8,<window%20id='win'/>";
const CUSTOMIZATION_URL = "about:customizing";



const PRELOADER_INTERVAL_MS = 600;

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

this.CustomizationTabPreloader = {
  uninit: function () {
    CustomizationTabPreloaderInternal.uninit();
  },

  newTab: function (aTab) {
    return CustomizationTabPreloaderInternal.newTab(aTab);
  },

  




  ensurePreloading: function() {
    CustomizationTabPreloaderInternal.ensurePreloading();
  },
};

Object.freeze(CustomizationTabPreloader);

this.CustomizationTabPreloaderInternal = {
  _browser: null,

  uninit: function () {
    HostFrame.destroy();

    if (this._browser) {
      this._browser.destroy();
      this._browser = null;
    }
  },

  newTab: function (aTab) {
    let win = aTab.ownerDocument.defaultView;
    if (win.gBrowser && this._browser) {
      return this._browser.swapWithNewTab(aTab);
    }

    return false;
  },

  ensurePreloading: function () {
    if (!this._browser) {
      this._browser = new HiddenBrowser();
    }
  }
};

function HiddenBrowser() {
  this._createBrowser();
}

HiddenBrowser.prototype = {
  _timer: null,

  get isPreloaded() {
    return this._browser &&
           this._browser.contentDocument &&
           this._browser.contentDocument.readyState === "complete" &&
           this._browser.currentURI.spec === CUSTOMIZATION_URL;
  },

  swapWithNewTab: function (aTab) {
    if (!this.isPreloaded || this._timer) {
      return false;
    }

    let win = aTab.ownerDocument.defaultView;
    let tabbrowser = win.gBrowser;

    if (!tabbrowser) {
      return false;
    }

    
    tabbrowser.swapNewTabWithBrowser(aTab, this._browser);

    
    let mm = aTab.linkedBrowser.messageManager;
    let scripts = win.getGroupMessageManager("browsers").getDelayedFrameScripts();
    Array.forEach(scripts, ([script, runGlobal]) => mm.loadFrameScript(script, true, runGlobal));

    
    this._removeBrowser();

    
    this._timer = createTimer(this, PRELOADER_INTERVAL_MS);

    
    return true;
  },

  observe: function () {
    this._timer = null;

    
    this._createBrowser();
  },

  destroy: function () {
    this._removeBrowser();
    this._timer = clearTimer(this._timer);
  },

  _createBrowser: function () {
    HostFrame.get().then(aFrame => {
      let doc = aFrame.document;
      this._browser = doc.createElementNS(XUL_NS, "browser");
      this._browser.setAttribute("type", "content");
      this._browser.setAttribute("src", CUSTOMIZATION_URL);
      this._browser.style.width = "400px";
      this._browser.style.height = "400px";
      doc.getElementById("win").appendChild(this._browser);
    });
  },

  _removeBrowser: function () {
    if (this._browser) {
      this._browser.remove();
      this._browser = null;
    }
  }
};

let HostFrame = {
  _frame: null,
  _deferred: null,

  get hiddenDOMDocument() {
    return Services.appShell.hiddenDOMWindow.document;
  },

  get isReady() {
    return this.hiddenDOMDocument.readyState === "complete";
  },

  get: function () {
    if (!this._deferred) {
      this._deferred = Promise.defer();
      this._create();
    }

    return this._deferred.promise;
  },

  destroy: function () {
    if (this._frame) {
      if (!Cu.isDeadWrapper(this._frame)) {
        this._frame.removeEventListener("load", this, true);
        this._frame.remove();
      }

      this._frame = null;
      this._deferred = null;
    }
  },

  handleEvent: function () {
    let contentWindow = this._frame.contentWindow;
    if (contentWindow.location.href === XUL_PAGE) {
      this._frame.removeEventListener("load", this, true);
      this._deferred.resolve(contentWindow);
    } else {
      contentWindow.location = XUL_PAGE;
    }
  },

  _create: function () {
    if (this.isReady) {
      let doc = this.hiddenDOMDocument;
      this._frame = doc.createElementNS(HTML_NS, "iframe");
      this._frame.addEventListener("load", this, true);
      doc.documentElement.appendChild(this._frame);
    } else {
      let flags = Ci.nsIThread.DISPATCH_NORMAL;
      Services.tm.currentThread.dispatch(() => this._create(), flags);
    }
  }
};
