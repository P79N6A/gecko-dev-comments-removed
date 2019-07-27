




"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

Cu.import("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

loader.lazyRequireGetter(this, "TimelineFront",
  "devtools/server/actors/timeline", true);

function TimelinePanel(iframeWindow, toolbox) {
  this.panelWin = iframeWindow;
  this._toolbox = toolbox;

  EventEmitter.decorate(this);
};

exports.TimelinePanel = TimelinePanel;

TimelinePanel.prototype = {
  





  open: Task.async(function*() {
    
    yield this.target.makeRemote();

    this.panelWin.gToolbox = this._toolbox;
    this.panelWin.gTarget = this.target;
    this.panelWin.gFront = new TimelineFront(this.target.client, this.target.form);
    yield this.panelWin.startupTimeline();

    this.isReady = true;
    this.emit("ready");
    return this;
  }),

  

  get target() this._toolbox.target,

  destroy: Task.async(function*() {
    
    if (this._destroyed) {
      return;
    }

    yield this.panelWin.shutdownTimeline();
    this.emit("destroyed");
    this._destroyed = true;
  })
};
