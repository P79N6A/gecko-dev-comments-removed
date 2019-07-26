


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const Services = require("Services");
const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils.js");

const {on, once, off, emit} = events;
const {method, custom, Arg, Option, RetVal} = protocol;

exports.register = function(handle) {
  handle.addTabActor(FramerateActor, "framerateActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(FramerateActor);
};




let FramerateActor = exports.FramerateActor = protocol.ActorClass({
  typeName: "framerate",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._contentWin = tabActor.window;
    this._onRefreshDriverTick = this._onRefreshDriverTick.bind(this);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  


  startRecording: method(function() {
    if (this._recording) {
      return;
    }
    this._recording = true;
    this._ticks = [];

    this._startTime = this._contentWin.performance.now();
    this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);
  }, {
  }),

  


  stopRecording: method(function() {
    if (!this._recording) {
      return [];
    }
    this._recording = false;

    
    let ticks = this._ticks;
    this._ticks = null;
    return ticks;
  }, {
    response: { timeline: RetVal("array:number") }
  }),

  


  _onRefreshDriverTick: function() {
    if (!this._recording) {
      return;
    }
    this._contentWin.requestAnimationFrame(this._onRefreshDriverTick);

    
    let currentTime = this._contentWin.performance.now();
    let elapsedTime = currentTime - this._startTime;
    this._ticks.push(elapsedTime);
  }
});




let FramerateFront = exports.FramerateFront = protocol.FrontClass(FramerateActor, {
  initialize: function(client, { framerateActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: framerateActor });
    this.manage(this);
  },

  











  plotFPS: function(ticks, interval = 100) {
    let timeline = [];
    let totalTicks = ticks.length;

    
    
    if (totalTicks == 0) {
      timeline.push({ delta: 0, value: 0 });
      timeline.push({ delta: interval, value: 0 });
      return timeline;
    }

    let frameCount = 0;
    let prevTime = ticks[0];

    for (let i = 1; i < totalTicks; i++) {
      let currTime = ticks[i];
      frameCount++;

      let elapsedTime = currTime - prevTime;
      if (elapsedTime < interval) {
        continue;
      }

      let framerate = 1000 / (elapsedTime / frameCount);
      timeline.push({ delta: prevTime, value: framerate });
      timeline.push({ delta: currTime, value: framerate });

      frameCount = 0;
      prevTime = currTime;
    }

    return timeline;
  }
});
