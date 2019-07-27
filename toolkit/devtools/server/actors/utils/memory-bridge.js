



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const { reportException } = require("devtools/toolkit/DevToolsUtils");
const { Class } = require("sdk/core/heritage");
loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "EventTarget", "sdk/event/target", true);
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











let MemoryBridge = Class({
  extends: EventTarget,

  


  initialize: function (parent, frameCache = new StackFrameCache()) {
    this.parent = parent;
    this._mgr = Cc["@mozilla.org/memory-reporter-manager;1"]
                  .getService(Ci.nsIMemoryReporterManager);
    this.state = "detached";
    this._dbg = null;
    this._frameCache = frameCache;

    this._onGarbageCollection = this._onGarbageCollection.bind(this);
    this._onWindowReady = this._onWindowReady.bind(this);

    events.on(this.parent, "window-ready", this._onWindowReady);
  },

  destroy: function() {
    events.off(this.parent, "window-ready", this._onWindowReady);

    this._mgr = null;
    if (this.state === "attached") {
      this.detach();
    }
  },

  get dbg() {
    if (!this._dbg) {
      this._dbg = this.parent.makeDebugger();
    }
    return this._dbg;
  },


  






  attach: expectState("detached", function() {
    this.dbg.addDebuggees();
    this.dbg.memory.onGarbageCollection = this._onGarbageCollection.bind(this);
    this.state = "attached";
  }, `attaching to the debugger`),

  


  detach: expectState("attached", function() {
    this._clearDebuggees();
    this.dbg.enabled = false;
    this._dbg = null;
    this.state = "detached";
  }, `detaching from the debugger`),

  


  getState: function () {
    return this.state;
  },

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

  


  _onGarbageCollection: function (data) {
    events.emit(this, "garbage-collection", data);
  },

  



  takeCensus: expectState("attached", function() {
    return this.dbg.memory.takeCensus();
  }, `taking census`),

  





  startRecordingAllocations: expectState("attached", function(options = {}) {
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
  }, `starting recording allocations`),

  


  stopRecordingAllocations: expectState("attached", function() {
    this.dbg.memory.trackingAllocationSites = false;
    this._clearFrames();

    return Date.now();
  }, `stopping recording allocations`),

  



  getAllocationsSettings: expectState("attached", function() {
    return {
      maxLogLength: this.dbg.memory.maxAllocationsLogLength,
      probability: this.dbg.memory.allocationSamplingProbability
    };
  }, `getting allocations settings`),

  


























































  getAllocations: expectState("attached", function() {
    if (this.dbg.memory.allocationsLogOverflowed) {
      
      
      
      
      reportException("MemoryBridge.prototype.getAllocations",
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
  }, `getting allocations`),

  


  forceGarbageCollection: function () {
    for (let i = 0; i < 3; i++) {
      Cu.forceGC();
    }
  },

  




  forceCycleCollection: function () {
    Cu.forceCC();
  },

  





  measure: function () {
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
      reportException("MemoryBridge.prototype.measure", e);
    }

    return result;
  },

  residentUnique: function () {
    return this._mgr.residentUnique;
  }
});

exports.MemoryBridge = MemoryBridge;
