


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
    this._chromeWin = getChromeWin(tabActor.window);
    this._onRefreshDriverTick = this._onRefreshDriverTick.bind(this);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.stopRecording();
  },

  


  startRecording: method(function() {
    if (this._recording) {
      return;
    }
    this._recording = true;
    this._ticks = [];

    this._startTime = this._chromeWin.performance.now();
    this._rafID = this._chromeWin.requestAnimationFrame(this._onRefreshDriverTick);
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
    this._chromeWin.cancelAnimationFrame(this._rafID);
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
    this._rafID = this._chromeWin.requestAnimationFrame(this._onRefreshDriverTick);

    
    let currentTime = this._chromeWin.performance.now();
    let elapsedTime = currentTime - this._startTime;
    this._ticks.push(elapsedTime);
  }
});




let FramerateFront = exports.FramerateFront = protocol.FrontClass(FramerateActor, {
  initialize: function(client, { framerateActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: framerateActor });
    this.manage(this);
  }
});















FramerateFront.plotFPS = function(ticks, interval = 100, clamp = 60) {
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

    let framerate = Math.min(1000 / (elapsedTime / frameCount), clamp);
    timeline.push({ delta: prevTime, value: framerate });
    timeline.push({ delta: currTime, value: framerate });

    frameCount = 0;
    prevTime = currTime;
  }

  return timeline;
};









function getChromeWin(innerWin) {
  return innerWin
    .QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation)
    .QueryInterface(Ci.nsIDocShellTreeItem).rootTreeItem
    .QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
}
