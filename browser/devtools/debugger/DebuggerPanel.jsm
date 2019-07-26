




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

this.EXPORTED_SYMBOLS = ["DebuggerPanel"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
    "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

function DebuggerPanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;

  this._controller = this.panelWin.DebuggerController;
  this._view = this.panelWin.DebuggerView;
  this._controller._target = this.target;
  this._bkp = this._controller.Breakpoints;

  EventEmitter.decorate(this);
}

DebuggerPanel.prototype = {
  


  open: function DebuggerPanel_open() {
    let deferred = Promise.defer();

    this._ensureOnlyOneRunningDebugger();

    let onDebuggerLoaded = function () {
      this.panelWin.removeEventListener("Debugger:Loaded",
                                        onDebuggerLoaded, true);
      this._isReady = true;
      this.emit("ready");
      deferred.resolve(this);
    }.bind(this);

    let onDebuggerConnected = function () {
      this.panelWin.removeEventListener("Debugger:Connected",
                                        onDebuggerConnected, true);
      this.emit("connected");
    }.bind(this);

    this.panelWin.addEventListener("Debugger:Loaded", onDebuggerLoaded, true);
    this.panelWin.addEventListener("Debugger:Connected",
                                   onDebuggerConnected, true);

    
    if (this.target.isRemote) {
      this.panelWin._remoteFlag = true;
      return deferred.promise;
    }

    
    return this.target.makeRemote().then(function success() {
      return deferred.promise;
    });
  },

  
  get target() this._toolbox.target,

  get isReady() this._isReady,

  destroy: function() {
    this.emit("destroyed");
    return Promise.resolve(null);
  },

  

  addBreakpoint: function() {
    this._bkp.addBreakpoint.apply(this._bkp, arguments);
  },

  removeBreakpoint: function() {
    this._bkp.removeBreakpoint.apply(this._bkp, arguments);
  },

  getBreakpoint: function() {
    return this._bkp.getBreakpoint.apply(this._bkp, arguments);
  },

  getAllBreakpoints: function() {
    return this._bkp.store;
  },

  

  _ensureOnlyOneRunningDebugger: function() {
    
  },
};
