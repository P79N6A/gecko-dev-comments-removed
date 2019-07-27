



"use strict";

this.EXPORTED_SYMBOLS = ["CustomizationTabPreloader"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HiddenFrame",
  "resource:///modules/HiddenFrame.jsm");

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
  _hiddenFrame: null,

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
    if (this._hiddenFrame) {
      this._hiddenFrame.destroy();
      this._hiddenFrame = null;
    }
    this._timer = clearTimer(this._timer);
  },

  _createBrowser: function () {
    if (!this._hiddenFrame) {
      this._hiddenFrame = new HiddenFrame();
    }

    this._hiddenFrame.get().then(aFrame => {
      let doc = aFrame.document;
      this._browser = doc.createElementNS(XUL_NS, "browser");
      this._browser.permanentKey = {};
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
