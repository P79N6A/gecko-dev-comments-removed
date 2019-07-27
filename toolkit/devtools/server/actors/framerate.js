


"use strict";

const protocol = require("devtools/server/protocol");
const { actorBridge } = require("devtools/server/actors/utils/actor-utils");
const { method, custom, Arg, Option, RetVal } = protocol;
const { on, once, off, emit } = require("sdk/event/core");
const { Framerate } = require("devtools/toolkit/shared/framerate");







let FramerateActor = exports.FramerateActor = protocol.ActorClass({
  typeName: "framerate",
  initialize: function (conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.bridge = new Framerate(tabActor);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.bridge.destroy();
  },

  startRecording: actorBridge("startRecording", {}),

  stopRecording: actorBridge("stopRecording", {
    request: {
      beginAt: Arg(0, "nullable:number"),
      endAt: Arg(1, "nullable:number")
    },
    response: { ticks: RetVal("array:number") }
  }),

  cancelRecording: actorBridge("cancelRecording"),

  isRecording: actorBridge("isRecording", {
    response: { recording: RetVal("boolean") }
  }),

  getPendingTicks: actorBridge("getPendingTicks", {
    request: {
      beginAt: Arg(0, "nullable:number"),
      endAt: Arg(1, "nullable:number")
    },
    response: { ticks: RetVal("array:number") }
  }),
});




let FramerateFront = exports.FramerateFront = protocol.FrontClass(FramerateActor, {
  initialize: function(client, { framerateActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: framerateActor });
    this.manage(this);
  }
});
