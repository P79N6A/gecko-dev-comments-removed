




"use strict";

const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ["DebuggerPanel"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

function DebuggerPanel(iframeWindow, toolbox) {
  this._toolbox = toolbox;
  this._controller = iframeWindow.DebuggerController;
  this._view = iframeWindow.DebuggerView;
  this._controller._target = this.target;
  this._bkp = this._controller.Breakpoints;
  this.panelWin = iframeWindow;

  this._ensureOnlyOneRunningDebugger();
  if (!this.target.isRemote) {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }
  }

  let onDebuggerLoaded = function () {
    iframeWindow.removeEventListener("Debugger:Loaded", onDebuggerLoaded, true);
    this.setReady();
  }.bind(this);

  let onDebuggerConnected = function () {
    iframeWindow.removeEventListener("Debugger:Connected",
      onDebuggerConnected, true);
    this.emit("connected");
  }.bind(this);

  iframeWindow.addEventListener("Debugger:Loaded", onDebuggerLoaded, true);
  iframeWindow.addEventListener("Debugger:Connected",
    onDebuggerConnected, true);

  new EventEmitter(this);
}

DebuggerPanel.prototype = {
  
  get target() this._toolbox.target,

  get isReady() this._isReady,

  setReady: function() {
    this._isReady = true;
    this.emit("ready");
  },

  destroy: function() {
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
