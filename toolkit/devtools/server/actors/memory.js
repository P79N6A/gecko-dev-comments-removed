



"use strict";

const { Cc, Ci, Cu } = require("chrome");
let protocol = require("devtools/server/protocol");
let { method, RetVal, Arg, types } = protocol;
const { reportException } = require("devtools/toolkit/DevToolsUtils");
loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "StackFrameCache",
                         "devtools/server/actors/utils/stack", true);
















function expectState(expectedState, method) {
  return function(...args) {
    if (this.state !== expectedState) {
      const msg = "Wrong State: Expected '" + expectedState + "', but current "
                + "state is '" + this.state + "'";
      return Promise.reject(new Error(msg));
    }

    return method.apply(this, args);
  };
}

types.addDictType("AllocationsRecordingOptions", {
  
  
  
  probability: "number"
});







let MemoryActor = protocol.ActorClass({
  typeName: "memory",

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
    this.state = "attached";
  }), {
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
  }), {
    request: {},
    response: {
      type: "detached"
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
        nthis._frameCache.initFrames();
      }
      this.dbg.addDebuggees();
    }
  },

  



  takeCensus: method(expectState("attached", function() {
    return this.dbg.memory.takeCensus();
  }), {
    request: {},
    response: RetVal("json")
  }),

  





  startRecordingAllocations: method(expectState("attached", function(options = {}) {
    this._frameCache.initFrames();
    this.dbg.memory.allocationSamplingProbability = options.probability != null
      ? options.probability
      : 1.0;
    this.dbg.memory.trackingAllocationSites = true;
  }), {
    request: {
      options: Arg(0, "nullable:AllocationsRecordingOptions")
    },
    response: {}
  }),

  


  stopRecordingAllocations: method(expectState("attached", function() {
    this.dbg.memory.trackingAllocationSites = false;
    this._clearFrames();
  }), {
    request: {},
    response: {}
  }),

  


























































  getAllocations: method(expectState("attached", function() {
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
  }), {
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
