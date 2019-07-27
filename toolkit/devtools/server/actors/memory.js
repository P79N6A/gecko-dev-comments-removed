



"use strict";

const { Cc, Ci, Cu } = require("chrome");
let protocol = require("devtools/server/protocol");
let { method, RetVal, Arg } = protocol;
const { reportException } = require("devtools/toolkit/DevToolsUtils");
















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
  },

  destroy: function() {
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
    this.dbg.removeAllDebuggees();
    this.dbg.enabled = false;
    this._dbg = null;
    this.state = "detached";
  }), {
    request: {},
    response: {
      type: "detached"
    }
  }),

  





  recordAllocations: method(expectState("attached", function(shouldRecord) {
    this.dbg.memory.trackingAllocationSites = shouldRecord;
  }), {
    request: {
      shouldRecord: Arg(0, "boolean")
    },
    response: {}
  }),

  

































  getAllocations: method(expectState("attached", function() {
    const packet = {
      frames: [],
      allocations: []
    };

    const framesToIndices = new Map();

    for (let stack of this.dbg.memory.flushAllocationsLog()) {
      packet.allocations.push(this._insertStackInAllocationsPacket(stack,
                                                                   packet,
                                                                   framesToIndices));
    }

    return packet;
  }), {
    request: {},
    response: RetVal("json")
  }),

  

















  _insertStackInAllocationsPacket: function(frame, packet, framesToIndices) {
    if (framesToIndices.has(frame)) {
      return framesToIndices.get(frame);
    }

    let frameForm = {
      line: frame.line,
      column: frame.column,
      source: frame.source,
      functionDisplayName: frame.functionDisplayName
    };

    if (frame.parent) {
      frameForm.parent = this._insertStackInAllocationsPacket(frame.parent,
                                                              packet,
                                                              framesToIndices);
    }

    let idx = packet.frames.length;
    packet.frames.push(frameForm);
    return idx;
  },

  





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

exports.register = function(handle) {
  handle.addGlobalActor(MemoryActor, "memoryActor");
  handle.addTabActor(MemoryActor, "memoryActor");
};

exports.unregister = function(handle) {
  handle.removeGlobalActor(MemoryActor, "memoryActor");
  handle.removeTabActor(MemoryActor, "memoryActor");
};
