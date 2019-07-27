




"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

loader.lazyRequireGetter(this, "getProfilerConnection",
  "devtools/profiler/shared", true);
loader.lazyRequireGetter(this, "ProfilerFront",
  "devtools/profiler/shared", true);

function ProfilerPanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;

  EventEmitter.decorate(this);
}

exports.ProfilerPanel = ProfilerPanel;

ProfilerPanel.prototype = {
  





  open: Task.async(function*() {
    let connection = getProfilerConnection(this._toolbox);
    yield connection.open();

    this.panelWin.gToolbox = this._toolbox;
    this.panelWin.gTarget = this.target;
    this.panelWin.gFront = new ProfilerFront(connection);
    yield this.panelWin.startupProfiler();

    this.isReady = true;
    this.emit("ready");
    return this;
  }),

  

  get target() this._toolbox.target,

  destroy: Task.async(function*() {
    
    if (this._destroyed) {
      return;
    }

    yield this.panelWin.shutdownProfiler();
    this.emit("destroyed");
    this._destroyed = true;
  })
};
