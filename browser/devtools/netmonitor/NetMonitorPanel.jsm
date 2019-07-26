




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

this.EXPORTED_SYMBOLS = ["NetMonitorPanel"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");

this.NetMonitorPanel = function NetMonitorPanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;
  this._destroyer = null;

  this._view = this.panelWin.NetMonitorView;
  this._controller = this.panelWin.NetMonitorController;
  this._controller._target = this.target;

  EventEmitter.decorate(this);
};

NetMonitorPanel.prototype = {
  





  open: function() {
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
                       aReason.error + ": " + aReason.message);
      });
  },

  

  get target() this._toolbox.target,

  destroy: function() {
    
    if (this._destroyer) {
      return this._destroyer;
    }

    return this._destroyer = this._controller.shutdownNetMonitor().then(() => {
      this.emit("destroyed");
    });
  }
};
