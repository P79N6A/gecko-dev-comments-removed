



"use strict";

const { Cc, Ci, Cu } = require("chrome");
let protocol = require("devtools/server/protocol");
let { method, RetVal, Arg, types } = protocol;
const { reportException } = require("devtools/toolkit/DevToolsUtils");
loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "StackFrameCache",
                         "devtools/server/actors/utils/stack", true);

















function expectState(expectedState, method, activity) {
  return function(...args) {
    if (this.state !== expectedState) {
      const msg = `Wrong state while ${activity}:` +
                  `Expected '${expectedState}',` +
                  `but current state is '${this.state}'.`;
      return Promise.reject(new Error(msg));
    }

    return method.apply(this, args);
  };
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

  get dbg() {
    if (!this._dbg) {
      this._dbg = this.parent.makeDebugger();
    }
    return this._dbg;
  },

  initialize: function(conn, parent, frameCache = new StackFrameCache()) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.parent = parent;
    this._mgr = Cc["@mozilla.org/memory-reporter-manager;1"]
                  .getService(Ci.nsIMemoryReporterManager);
    this.state = "detached";
    this._dbg = null;
    this._frameCache = frameCache;

    this._onGarbageCollection = data =>
      events.emit(this, "garbage-collection", data);

    this._onWindowReady = this._onWindowReady.bind(this);

    events.on(this.parent, "window-ready", this._onWindowReady);
  },

  destroy: function() {
    events.off(this.parent, "window-ready", this._onWindowReady);

    this._mgr = null;
    if (this.state === "attached") {
      this.detach();
    }
    protocol.Actor.prototype.destroy.call(this);
  },

  






  attach: method(expectState("detached", function() {
    this.dbg.addDebuggees();
    this.dbg.memory.onGarbageCollection = this._onGarbageCollection;
    this.state = "attached";
  },
  `attaching to the debugger`), {
    request: {},
    response: {
      type: "attached"
    }
  }),

  


  detach: method(expectState("attached", function() {
    this._clearDebuggees();
    this.dbg.enabled = false;
    this._dbg = null;
    this.state = "detached";
  },
  `detaching from the debugger`), {
    request: {},
    response: {
      type: "detached"
    }
  }),

  


  getState: method(function() {
    return this.state;
  }, {
    response: {
      state: RetVal(0, "string")
    }
  }),

  _clearDebuggees: function() {
    if (this._dbg) {
      if (this.dbg.memory.trackingAllocationSites) {
        this.dbg.memory.drainAllocationsLog();
      }
      this._clearFrames();
      this.dbg.removeAllDebuggees();
    }
  },

  _clearFrames: function() {
    if (this.dbg.memory.trackingAllocationSites) {
      this._frameCache.clearFrames();
    }
  },

  


  _onWindowReady: function({ isTopLevel }) {
    if (this.state == "attached") {
      if (isTopLevel && this.dbg.memory.trackingAllocationSites) {
        this._clearDebuggees();
        this._frameCache.initFrames();
      }
      this.dbg.addDebuggees();
    }
  },

  



  takeCensus: method(expectState("attached", function() {
    return this.dbg.memory.takeCensus();
  },
  `taking census`), {
    request: {},
    response: RetVal("json")
  }),

  





  startRecordingAllocations: method(expectState("attached", function(options = {}) {
    if (this.dbg.memory.trackingAllocationSites) {
      return Date.now();
    }

    this._frameCache.initFrames();

    this.dbg.memory.allocationSamplingProbability = options.probability != null
      ? options.probability
      : 1.0;
    if (options.maxLogLength != null) {
      this.dbg.memory.maxAllocationsLogLength = options.maxLogLength;
    }
    this.dbg.memory.trackingAllocationSites = true;

    return Date.now();
  },
  `starting recording allocations`), {
    request: {
      options: Arg(0, "nullable:AllocationsRecordingOptions")
    },
    response: {
      
      value: RetVal(0, "nullable:number")
    }
  }),

  


  stopRecordingAllocations: method(expectState("attached", function() {
    this.dbg.memory.trackingAllocationSites = false;
    this._clearFrames();

    return Date.now();
  },
  `stopping recording allocations`), {
    request: {},
    response: {
      
      value: RetVal(0, "nullable:number")
    }
  }),

  



  getAllocationsSettings: method(expectState("attached", function() {
    return {
      maxLogLength: this.dbg.memory.maxAllocationsLogLength,
      probability: this.dbg.memory.allocationSamplingProbability
    };
  },
  `getting allocations settings`), {
    request: {},
    response: {
      options: RetVal(0, "json")
    }
  }),

  


























































  getAllocations: method(expectState("attached", function() {
    if (this.dbg.memory.allocationsLogOverflowed) {
      
      
      
      
      reportException("MemoryActor.prototype.getAllocations",
                      "Warning: allocations log overflowed and lost some data.");
    }

    const allocations = this.dbg.memory.drainAllocationsLog()
    const packet = {
      allocations: [],
      allocationsTimestamps: []
    };

    for (let { frame: stack, timestamp } of allocations) {
      if (stack && Cu.isDeadWrapper(stack)) {
        continue;
      }

      
      let waived = Cu.waiveXrays(stack);

      
      
      
      
      let index = this._frameCache.addFrame(waived);

      packet.allocations.push(index);
      packet.allocationsTimestamps.push(timestamp);
    }

    return this._frameCache.updateFramePacket(packet);
  },
  `getting allocations`), {
    request: {},
    response: RetVal("json")
  }),

  


  forceGarbageCollection: method(function() {
    for (let i = 0; i < 3; i++) {
      Cu.forceGC();
    }
  }, {
    request: {},
    response: {}
  }),

  




  forceCycleCollection: method(function() {
    Cu.forceCC();
  }, {
    request: {},
    response: {}
  }),

  





  measure: method(function() {
    let result = {};

    let jsObjectsSize = {};
    let jsStringsSize = {};
    let jsOtherSize = {};
    let domSize = {};
    let styleSize = {};
    let otherSize = {};
    let totalSize = {};
    let jsMilliseconds = {};
    let nonJSMilliseconds = {};

    try {
      this._mgr.sizeOfTab(this.parent.window, jsObjectsSize, jsStringsSize, jsOtherSize,
                          domSize, styleSize, otherSize, totalSize, jsMilliseconds, nonJSMilliseconds);
      result.total = totalSize.value;
      result.domSize = domSize.value;
      result.styleSize = styleSize.value;
      result.jsObjectsSize = jsObjectsSize.value;
      result.jsStringsSize = jsStringsSize.value;
      result.jsOtherSize = jsOtherSize.value;
      result.otherSize = otherSize.value;
      result.jsMilliseconds = jsMilliseconds.value.toFixed(1);
      result.nonJSMilliseconds = nonJSMilliseconds.value.toFixed(1);
    } catch (e) {
      reportException("MemoryActor.prototype.measure", e);
    }

    return result;
  }, {
    request: {},
    response: RetVal("json"),
  }),

  residentUnique: method(function() {
    return this._mgr.residentUnique;
  }, {
    request: {},
    response: { value: RetVal("number") }
  })
});

exports.MemoryActor = MemoryActor;

exports.MemoryFront = protocol.FrontClass(MemoryActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.memoryActor;
    this.manage(this);
  }
});
