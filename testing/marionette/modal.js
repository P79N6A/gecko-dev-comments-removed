



"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["modal"];

let isFirefox = () => Services.appinfo.name == "Firefox";

this.modal = {};
modal = {
  COMMON_DIALOG_LOADED: "common-dialog-loaded",
  TABMODAL_DIALOG_LOADED: "tabmodal-dialog-loaded",
  handlers: {
    "common-dialog-loaded": new Set(),
    "tabmodal-dialog-loaded": new Set()
  }
};
















modal.addHandler = function(handler) {
  if (!isFirefox()) {
    return;
  }

  Object.keys(this.handlers).map(topic => {
    this.handlers[topic].add(handler);
    Services.obs.addObserver(handler, topic, false);
  });
};










modal.removeHandler = function(toRemove) {
  if (!isFirefox()) {
    return;
  }

  for (let topic of Object.keys(this.handlers)) {
    let handlers = this.handlers[topic];
    for (let handler of handlers) {
      if (handler == toRemove) {
        Services.obs.removeObserver(handler, topic);
        handlers.delete(handler);
      }
    }
  }
};









modal.Dialog = function(curBrowserFn, winRef=null) {
  Object.defineProperty(this, "curBrowser", {
    get() { return curBrowserFn(); }
  });
  this.win_ = winRef;
};





Object.defineProperty(modal.Dialog.prototype, "window", {
  get() {
    if (this.win_ !== null) {
      let win = this.win_.get();
      if (win && win.parent)
        return win;
    }
    return null;
  }
});

Object.defineProperty(modal.Dialog.prototype, "ui", {
  get() {
    let win = this.window;
    if (win)
      return win.Dialog.ui;
    return this.curBrowser.getTabModalUI();
  }
});
