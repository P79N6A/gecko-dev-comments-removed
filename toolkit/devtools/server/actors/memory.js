



"use strict";

const protocol = require("devtools/server/protocol");
const { method, RetVal, Arg, types } = protocol;
const { MemoryBridge } = require("./utils/memory-bridge");
loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "StackFrameCache",
                         "devtools/server/actors/utils/stack", true);






function linkBridge (methodName, definition) {
  return method(function () {
    return this.bridge[methodName].apply(this.bridge, arguments);
  }, definition);
}

types.addDictType("AllocationsRecordingOptions", {
  
  
  
  probability: "number",

  
  
  
  maxLogLength: "number"
});







let MemoryActor = protocol.ActorClass({
  typeName: "memory",

  




  events: {
    
    
    
    "garbage-collection": {
      type: "garbage-collection",
      data: Arg(0, "json"),
    },
  },

  initialize: function(conn, parent, frameCache = new StackFrameCache()) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this._onGarbageCollection = this._onGarbageCollection.bind(this);
    this.bridge = new MemoryBridge(parent, frameCache);
    this.bridge.on("garbage-collection", this._onGarbageCollection);
  },

  destroy: function() {
    this.bridge.off("garbage-collection", this._onGarbageCollection);
    this.bridge.destroy();
    protocol.Actor.prototype.destroy.call(this);
  },

  






  attach: linkBridge("attach", {
    request: {},
    response: {
      type: "attached"
    }
  }),

  


  detach: linkBridge("detach", {
    request: {},
    response: {
      type: "detached"
    }
  }),

  


  getState: linkBridge("getState", {
    response: {
      state: RetVal(0, "string")
    }
  }),

  



  takeCensus: linkBridge("takeCensus", {
    request: {},
    response: RetVal("json")
  }),

  





  startRecordingAllocations: linkBridge("startRecordingAllocations", {
    request: {
      options: Arg(0, "nullable:AllocationsRecordingOptions")
    },
    response: {
      
      value: RetVal(0, "nullable:number")
    }
  }),

  


  stopRecordingAllocations: linkBridge("stopRecordingAllocations", {
    request: {},
    response: {
      
      value: RetVal(0, "nullable:number")
    }
  }),

  



  getAllocationsSettings: linkBridge("getAllocationsSettings", {
    request: {},
    response: {
      options: RetVal(0, "json")
    }
  }),

  getAllocations: linkBridge("getAllocations", {
    request: {},
    response: RetVal("json")
  }),

  


  forceGarbageCollection: linkBridge("forceGarbageCollection", {
    request: {},
    response: {}
  }),

  




  forceCycleCollection: linkBridge("forceCycleCollection", {
    request: {},
    response: {}
  }),

  





  measure: linkBridge("measure", {
    request: {},
    response: RetVal("json"),
  }),

  residentUnique: linkBridge("residentUnique", {
    request: {},
    response: { value: RetVal("number") }
  }),

  



  _onGarbageCollection: function (data) {
    events.emit(this, "garbage-collection", data);
  },
});

exports.MemoryActor = MemoryActor;

exports.MemoryFront = protocol.FrontClass(MemoryActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.memoryActor;
    this.manage(this);
  }
});
