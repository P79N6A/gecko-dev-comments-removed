



"use strict";

const { Cc, Ci, Cu } = require("chrome");
let protocol = require("devtools/server/protocol");
let { method, RetVal, Arg, types } = protocol;
const { reportException } = require("devtools/toolkit/DevToolsUtils");
loader.lazyRequireGetter(this, "events", "sdk/event/core");
















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

  initialize: function(conn, parent) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.parent = parent;
    this._mgr = Cc["@mozilla.org/memory-reporter-manager;1"]
                  .getService(Ci.nsIMemoryReporterManager);
    this.state = "detached";
    this._dbg = null;
    this._framesToCounts = null;
    this._framesToIndices = null;
    this._framesToForms = null;

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

  _initFrames: function() {
    if (this._framesToCounts) {
      
      return;
    }

    this._framesToCounts = new Map();
    this._framesToIndices = new Map();
    this._framesToForms = new Map();
  },

  _clearFrames: function() {
    if (this.dbg.memory.trackingAllocationSites) {
      this._framesToCounts.clear();
      this._framesToCounts = null;
      this._framesToIndices.clear();
      this._framesToIndices = null;
      this._framesToForms.clear();
      this._framesToForms = null;
    }
  },

  


  _onWindowReady: function({ isTopLevel }) {
    if (this.state == "attached") {
      if (isTopLevel && this.dbg.memory.trackingAllocationSites) {
        this._clearDebuggees();
        this._initFrames();
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
    this._initFrames();
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
      allocations: []
    };

    for (let stack of allocations) {
      if (stack && Cu.isDeadWrapper(stack)) {
        continue;
      }

      
      let waived = Cu.waiveXrays(stack);

      
      
      
      
      this._assignFrameIndices(waived);
      this._createFrameForms(waived);
      this._countFrame(waived);

      packet.allocations.push(this._framesToIndices.get(waived));
    }

    
    
    
    const size = this._framesToForms.size;
    packet.frames = Array(size).fill(null);
    packet.counts = Array(size).fill(0);

    
    for (let [stack, index] of this._framesToIndices) {
      packet.frames[index] = this._framesToForms.get(stack);
      packet.counts[index] = this._framesToCounts.get(stack) || 0;
    }

    return packet;
  }), {
    request: {},
    response: RetVal("json")
  }),

  






  _assignFrameIndices: function(frame) {
    if (this._framesToIndices.has(frame)) {
      return;
    }

    if (frame) {
      this._assignFrameIndices(frame.parent);
    }

    const index = this._framesToIndices.size;
    this._framesToIndices.set(frame, index);
  },

  





  _createFrameForms: function(frame) {
    if (this._framesToForms.has(frame)) {
      return;
    }

    let form = null;
    if (frame) {
      form = {
        line: frame.line,
        column: frame.column,
        source: frame.source,
        functionDisplayName: frame.functionDisplayName,
        parent: this._framesToIndices.get(frame.parent)
      };
      this._createFrameForms(frame.parent);
    }

    this._framesToForms.set(frame, form);
  },

  





  _countFrame: function(frame) {
    if (!this._framesToCounts.has(frame)) {
      this._framesToCounts.set(frame, 1);
    } else {
      let count = this._framesToCounts.get(frame);
      this._framesToCounts.set(frame, count + 1);
    }
  },

  


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
