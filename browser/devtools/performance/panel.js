




"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const { PerformanceFront } = require("devtools/performance/front");

Cu.import("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

function PerformancePanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;

  EventEmitter.decorate(this);
}

exports.PerformancePanel = PerformancePanel;

PerformancePanel.prototype = {
  






  open: Task.async(function*() {
    this.panelWin.gToolbox = this._toolbox;
    this.panelWin.gTarget = this.target;

    
    
    this._connection = this.panelWin.gToolbox.getPerformanceActorsConnection();
    
    
    yield this._connection.open();

    this.panelWin.gFront = new PerformanceFront(this._connection);

    yield this.panelWin.startupPerformance();

    this.isReady = true;
    this.emit("ready");
    return this;
  }),

  

  get target() this._toolbox.target,

  destroy: Task.async(function*() {
    
    if (this._destroyed) {
      return;
    }

    yield this.panelWin.shutdownPerformance();
    this.emit("destroyed");
    this._destroyed = true;
  })
};
