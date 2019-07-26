




"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const promise = require("sdk/core/promise");
const EventEmitter = require("devtools/shared/event-emitter");

function DebuggerPanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;
  this._destroyer = null;

  this._view = this.panelWin.DebuggerView;
  this._controller = this.panelWin.DebuggerController;
  this._view._hostType = this._toolbox.hostType;
  this._controller._target = this.target;

  this.handleHostChanged = this.handleHostChanged.bind(this);
  this.highlightWhenPaused = this.highlightWhenPaused.bind(this);
  this.unhighlightWhenResumed = this.unhighlightWhenResumed.bind(this);

  EventEmitter.decorate(this);
};

exports.DebuggerPanel = DebuggerPanel;

DebuggerPanel.prototype = {
  





  open: function() {
    let targetPromise;

    
    if (!this.target.isRemote) {
      targetPromise = this.target.makeRemote();
    } else {
      targetPromise = promise.resolve(this.target);
    }

    return targetPromise
      .then(() => this._controller.startupDebugger())
      .then(() => this._controller.connect())
      .then(() => {
        this._toolbox.on("host-changed", this.handleHostChanged);
        this.target.on("thread-paused", this.highlightWhenPaused);
        this.target.on("thread-resumed", this.unhighlightWhenResumed);
        this.isReady = true;
        this.emit("ready");
        return this;
      })
      .then(null, function onError(aReason) {
        Cu.reportError("DebuggerPanel open failed. " +
                       aReason.error + ": " + aReason.message);
      });
  },

  

  get target() this._toolbox.target,

  destroy: function() {
    
    if (this._destroyer) {
      return this._destroyer;
    }

    this.target.off("thread-paused", this.highlightWhenPaused);
    this.target.off("thread-resumed", this.unhighlightWhenResumed);

    return this._destroyer = this._controller.shutdownDebugger().then(() => {
      this.emit("destroyed");
    });
  },

  

  addBreakpoint: function(aLocation, aOptions) {
    return this._controller.Breakpoints.addBreakpoint(aLocation, aOptions);
  },

  removeBreakpoint: function(aLocation) {
    return this._controller.Breakpoints.removeBreakpoint(aLocation);
  },

  handleHostChanged: function() {
    this._view.handleHostChanged(this._toolbox.hostType);
  },

  highlightWhenPaused: function() {
    this._toolbox.highlightTool("jsdebugger");

    
    
    this._toolbox.raise();
  },

  unhighlightWhenResumed: function() {
    this._toolbox.unhighlightTool("jsdebugger");
  }
};
