




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

this.EXPORTED_SYMBOLS = ["NetMonitorPanel"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");

function NetMonitorPanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;

  this._view = this.panelWin.NetMonitorView;
  this._controller = this.panelWin.NetMonitorController;
  this._controller._target = this.target;

  EventEmitter.decorate(this);
}

NetMonitorPanel.prototype = {
  





  open: function NetMonitorPanel_open() {
    let promise;

    
    if (!this.target.isRemote) {
      promise = this.target.makeRemote();
    } else {
      promise = Promise.resolve(this.target);
    }

    return promise
      .then(() => this._controller.startupNetMonitor())
      .then(() => this._controller.connect())
      .then(() => {
        this.isReady = true;
        this.emit("ready");
        return this;
      })
      .then(null, function onError(aReason) {
        Cu.reportError("NetMonitorPanel open failed. " +
                       reason.error + ": " + reason.message);
      });
  },

  
  get target() this._toolbox.target,

  destroy: function() {
    this._controller.shutdownNetMonitor().then(() => this.emit("destroyed"));
  }
};
