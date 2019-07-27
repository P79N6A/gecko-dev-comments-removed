


"use strict";














const protocol = require("devtools/server/protocol");
const { method, Arg, RetVal, Option } = protocol;
const events = require("sdk/event/core");
const { Timeline } = require("devtools/toolkit/shared/timeline");
const { actorBridge } = require("devtools/server/actors/common");









protocol.types.addType("array-of-numbers-as-strings", {
  write: (v) => v.join(","),
  
  read: (v) => typeof v === "string" ? v.split(",") : v
});




let TimelineActor = exports.TimelineActor = protocol.ActorClass({
  typeName: "timeline",

  events: {
    




    "markers" : {
      type: "markers",
      markers: Arg(0, "json"),
      endTime: Arg(1, "number")
    },

    




    "memory" : {
      type: "memory",
      delta: Arg(0, "number"),
      measurement: Arg(1, "json")
    },

    




    "ticks" : {
      type: "ticks",
      delta: Arg(0, "number"),
      timestamps: Arg(1, "array-of-numbers-as-strings")
    },

    




    "frames" : {
      type: "frames",
      delta: Arg(0, "number"),
      frames: Arg(1, "json")
    }
  },

  


  initialize: function (conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this.bridge = new Timeline(tabActor);

    this._onTimelineEvent = this._onTimelineEvent.bind(this);
    events.on(this.bridge, "*", this._onTimelineEvent);
  },

  




  disconnect: function() {
    this.destroy();
  },

  


  destroy: function () {
    events.off(this.bridge, "*", this._onTimelineEvent);
    this.bridge.destroy();
    this.bridge = null;
    this.tabActor = null;
    protocol.Actor.prototype.destroy.call(this);
  },

  



  _onTimelineEvent: function (eventName, ...args) {
    if (this.events[eventName]) {
      events.emit(this, eventName, ...args);
    }
  },

  isRecording: actorBridge("isRecording", {
    request: {},
    response: {
      value: RetVal("boolean")
    }
  }),

  start: actorBridge("start", {
    request: {
      withMemory: Option(0, "boolean"),
      withTicks: Option(0, "boolean")
    },
    response: {
      value: RetVal("number")
    }
  }),

  stop: actorBridge("stop", {
    response: {
      
      
      value: RetVal("nullable:number")
    }
  }),
});

exports.TimelineFront = protocol.FrontClass(TimelineActor, {
  initialize: function(client, {timelineActor}) {
    protocol.Front.prototype.initialize.call(this, client, {actor: timelineActor});
    this.manage(this);
  },
  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },
});
