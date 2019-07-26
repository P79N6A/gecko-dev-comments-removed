



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ "switchToFloatingScrollbars", "switchToNativeScrollbars" ];

Cu.import("resource://gre/modules/Services.jsm");

let URL = Services.io.newURI("chrome://browser/skin/devtools/floating-scrollbars.css", null, null);

let trackedTabs = new WeakMap();







this.switchToFloatingScrollbars = function switchToFloatingScrollbars(aTab) {
  let mgr = trackedTabs.get(aTab);
  if (!mgr) {
    mgr = new ScrollbarManager(aTab);
  }
  mgr.switchToFloating();
}







this.switchToNativeScrollbars = function switchToNativeScrollbars(aTab) {
  let mgr = trackedTabs.get(aTab);
  if (mgr) {
    mgr.reset();
  }
}

function ScrollbarManager(aTab) {
  trackedTabs.set(aTab, this);

  this.attachedTab = aTab;
  this.attachedBrowser = aTab.linkedBrowser;

  this.reset = this.reset.bind(this);
  this.switchToFloating = this.switchToFloating.bind(this);

  this.attachedTab.addEventListener("TabClose", this.reset, true);
  this.attachedBrowser.addEventListener("DOMContentLoaded", this.switchToFloating, true);
}

ScrollbarManager.prototype = {
  get win() {
    return this.attachedBrowser.contentWindow;
  },

  


  switchToFloating: function() {
    let windows = this.getInnerWindows(this.win);
    windows.forEach(this.injectStyleSheet);
    this.forceStyle();
  },


  


  reset: function() {
    let windows = this.getInnerWindows(this.win);
    windows.forEach(this.removeStyleSheet);
    this.forceStyle(this.attachedBrowser);
    this.attachedBrowser.removeEventListener("DOMContentLoaded", this.switchToFloating, true);
    this.attachedTab.removeEventListener("TabClose", this.reset, true);
    trackedTabs.delete(this.attachedTab);
  },

  


  forceStyle: function() {
    let parentWindow = this.attachedBrowser.ownerDocument.defaultView;
    let display = parentWindow.getComputedStyle(this.attachedBrowser).display; 
    this.attachedBrowser.style.display = "none";
    parentWindow.getComputedStyle(this.attachedBrowser).display; 
    this.attachedBrowser.style.display = display; 
  },

  


  getInnerWindows: function(win) {
    let iframes = win.document.querySelectorAll("iframe");
    let innerWindows = [];
    for (let iframe of iframes) {
      innerWindows = innerWindows.concat(this.getInnerWindows(iframe.contentWindow));
    }
    return [win].concat(innerWindows);
  },

  


  injectStyleSheet: function(win) {
    let winUtils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    try {
      winUtils.loadSheet(URL, win.AGENT_SHEET);
    }catch(e) {}
  },

  


  removeStyleSheet: function(win) {
    let winUtils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    try {
      winUtils.removeSheet(URL, win.AGENT_SHEET);
    }catch(e) {}
  },
}
