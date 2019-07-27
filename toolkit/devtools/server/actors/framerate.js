


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const Services = require("Services");
const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils.js");

const {on, once, off, emit} = events;
const {method, custom, Arg, Option, RetVal} = protocol;




let FramerateActor = exports.FramerateActor = protocol.ActorClass({
  typeName: "framerate",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._contentWin = tabActor.window;
    this._onRefreshDriverTick = this._onRefreshDriverTick.bind(this);
    this._onGlobalCreated = this._onGlobalCreated.bind(this);
    on(this.tabActor, "window-ready", this._onGlobalCreated);
  },
  destroy: function(conn) {
    off(this.tabActor, "window-ready", this._onGlobalCreated);
    protocol.Actor.prototype.destroy.call(this, conn);
    this.stopRecording();
  },

  


  startRecording: method(function() {
    if (this._recording) {
      return;
    }
    this._recording = true;
    this._ticks = [];

    this._startTime = this._contentWin.performance.now();
    this._rafID = this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);
  }, {
  }),

  


  stopRecording: method(function(beginAt = 0, endAt = Number.MAX_SAFE_INTEGER) {
    if (!this._recording) {
      return [];
    }
    let ticks = this.getPendingTicks(beginAt, endAt);
    this.cancelRecording();
    return ticks;
  }, {
    request: {
      beginAt: Arg(0, "nullable:number"),
      endAt: Arg(1, "nullable:number")
    },
    response: { ticks: RetVal("array:number") }
  }),

  


  cancelRecording: method(function() {
    this._contentWin.cancelAnimationFrame(this._rafID);
    this._recording = false;
    this._ticks = null;
    this._rafID = -1;
  }, {
  }),

  


  isRecording: method(function() {
    return !!this._recording;
  }, {
    response: { recording: RetVal("boolean") }
  }),

  


  getPendingTicks: method(function(beginAt = 0, endAt = Number.MAX_SAFE_INTEGER) {
    if (!this._ticks) {
      return [];
    }
    return this._ticks.filter(e => e >= beginAt && e <= endAt);
  }, {
    request: {
      beginAt: Arg(0, "nullable:number"),
      endAt: Arg(1, "nullable:number")
    },
    response: { ticks: RetVal("array:number") }
  }),

  


  _onRefreshDriverTick: function() {
    if (!this._recording) {
      return;
    }
    this._rafID = this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);

    
    let currentTime = this._contentWin.performance.now();

    
    
    this._elapsedTime = currentTime - this._startTime;
    this._ticks.push(this._elapsedTime);
  },

  


  _onGlobalCreated: function (win) {
    if (this._recording) {
      
      
      this._startTime = -this._elapsedTime;
      this._rafID = this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);
    }
  }
});




let FramerateFront = exports.FramerateFront = protocol.FrontClass(FramerateActor, {
  initialize: function(client, { framerateActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: framerateActor });
    this.manage(this);
  }
});
