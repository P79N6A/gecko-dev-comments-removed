




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
    this._onRecordingStartOrStop = this._onRecordingStartOrStop.bind(this);

    
    
    this._connection = this.panelWin.gToolbox.getPerformanceActorsConnection();
    
    
    yield this._connection.open();

    this.panelWin.gFront = new PerformanceFront(this._connection);
    this.panelWin.gFront.on("recording-started", this._onRecordingStartOrStop);
    this.panelWin.gFront.on("recording-stopped", this._onRecordingStartOrStop);

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

    this.panelWin.gFront.off("recording-started", this._onRecordingStartOrStop);
    this.panelWin.gFront.off("recording-stopped", this._onRecordingStartOrStop);
    yield this.panelWin.shutdownPerformance();
    this.emit("destroyed");
    this._destroyed = true;
  }),

  _onRecordingStartOrStop: function () {
    let front = this.panelWin.gFront;
    if (front.isRecording()) {
      this._toolbox.highlightTool("performance");
    } else {
      this._toolbox.unhighlightTool("performance");
    }
  }
};
