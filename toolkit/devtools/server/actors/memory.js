



"use strict";

const protocol = require("devtools/server/protocol");
const { method, RetVal, Arg, types } = protocol;
const { Memory } = require("devtools/toolkit/shared/memory");
const { actorBridge } = require("devtools/server/actors/common");
loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "StackFrameCache",
                         "devtools/server/actors/utils/stack", true);

types.addDictType("AllocationsRecordingOptions", {
  
  
  
  probability: "number",

  
  
  
  maxLogLength: "number"
});












let MemoryActor = exports.MemoryActor = protocol.ActorClass({
  typeName: "memory",

  




  events: {
    
    
    
    "garbage-collection": {
      type: "garbage-collection",
      data: Arg(0, "json"),
    },

    
    
    "allocations": {
      type: "allocations",
      data: Arg(0, "json"),
    },
  },

  initialize: function(conn, parent, frameCache = new StackFrameCache()) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this._onGarbageCollection = this._onGarbageCollection.bind(this);
    this._onAllocations = this._onAllocations.bind(this);
    this.bridge = new Memory(parent, frameCache);
    this.bridge.on("garbage-collection", this._onGarbageCollection);
    this.bridge.on("allocations", this._onAllocations);
  },

  destroy: function() {
    this.bridge.off("garbage-collection", this._onGarbageCollection);
    this.bridge.off("allocations", this._onAllocations);
    this.bridge.destroy();
    protocol.Actor.prototype.destroy.call(this);
  },

  attach: actorBridge("attach", {
    request: {},
    response: {
      type: "attached"
    }
  }),

  detach: actorBridge("detach", {
    request: {},
    response: {
      type: "detached"
    }
  }),

  getState: actorBridge("getState", {
    response: {
      state: RetVal(0, "string")
    }
  }),

  takeCensus: actorBridge("takeCensus", {
    request: {},
    response: RetVal("json")
  }),

  startRecordingAllocations: actorBridge("startRecordingAllocations", {
    request: {
      options: Arg(0, "nullable:AllocationsRecordingOptions")
    },
    response: {
      
      value: RetVal(0, "nullable:number")
    }
  }),

  stopRecordingAllocations: actorBridge("stopRecordingAllocations", {
    request: {},
    response: {
      
      value: RetVal(0, "nullable:number")
    }
  }),

  getAllocationsSettings: actorBridge("getAllocationsSettings", {
    request: {},
    response: {
      options: RetVal(0, "json")
    }
  }),

  getAllocations: actorBridge("getAllocations", {
    request: {},
    response: RetVal("json")
  }),

  forceGarbageCollection: actorBridge("forceGarbageCollection", {
    request: {},
    response: {}
  }),

  forceCycleCollection: actorBridge("forceCycleCollection", {
    request: {},
    response: {}
  }),

  measure: actorBridge("measure", {
    request: {},
    response: RetVal("json"),
  }),

  residentUnique: actorBridge("residentUnique", {
    request: {},
    response: { value: RetVal("number") }
  }),

  _onGarbageCollection: function (data) {
    events.emit(this, "garbage-collection", data);
  },

  _onAllocations: function (data) {
    events.emit(this, "allocations", data);
  },
});

exports.MemoryFront = protocol.FrontClass(MemoryActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.memoryActor;
    this.manage(this);
  }
});
