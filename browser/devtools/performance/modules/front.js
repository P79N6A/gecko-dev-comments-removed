


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const { Task } = require("resource://gre/modules/Task.jsm");
const { extend } = require("sdk/util/object");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "TimelineFront",
  "devtools/server/actors/timeline", true);
loader.lazyRequireGetter(this, "MemoryFront",
  "devtools/server/actors/memory", true);
loader.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");
loader.lazyRequireGetter(this, "compatibility",
  "devtools/performance/compatibility");

loader.lazyImporter(this, "gDevTools",
  "resource:///modules/devtools/gDevTools.jsm");
loader.lazyImporter(this, "setTimeout",
  "resource://gre/modules/Timer.jsm");
loader.lazyImporter(this, "clearTimeout",
  "resource://gre/modules/Timer.jsm");


const DEFAULT_ALLOCATION_SITES_PULL_TIMEOUT = 200; 





let SharedPerformanceActors = new WeakMap();










SharedPerformanceActors.forTarget = function(target) {
  if (this.has(target)) {
    return this.get(target);
  }

  let instance = new PerformanceActorsConnection(target);
  this.set(target, instance);
  return instance;
};











function PerformanceActorsConnection(target) {
  EventEmitter.decorate(this);

  this._target = target;
  this._client = this._target.client;
  this._request = this._request.bind(this);

  Services.obs.notifyObservers(null, "performance-actors-connection-created", null);
}

PerformanceActorsConnection.prototype = {

  
  _usingMockMemory: false,
  _usingMockTimeline: false,

  






  open: Task.async(function*() {
    if (this._connected) {
      return;
    }

    
    yield this._target.makeRemote();

    
    
    
    
    yield this._connectProfilerActor();
    yield this._connectTimelineActor();
    yield this._connectMemoryActor();

    this._connected = true;

    Services.obs.notifyObservers(null, "performance-actors-connection-opened", null);
  }),

  


  destroy: Task.async(function*() {
    yield this._disconnectActors();
    this._connected = false;
  }),

  


  _connectProfilerActor: Task.async(function*() {
    
    
    if (this._target.form && this._target.form.profilerActor) {
      this._profiler = this._target.form.profilerActor;
    }
    
    
    else if (this._target.root && this._target.root.profilerActor) {
      this._profiler = this._target.root.profilerActor;
    }
    
    else {
      this._profiler = (yield listTabs(this._client)).profilerActor;
    }
  }),

  


  _connectTimelineActor: function() {
    let supported = yield compatibility.timelineActorSupported(this._target);
    if (supported) {
      this._timeline = new TimelineFront(this._target.client, this._target.form);
    } else {
      this._usingMockTimeline = true;
      this._timeline = new compatibility.MockTimelineFront();
    }
  },

  


  _connectMemoryActor: Task.async(function* () {
    let supported = yield compatibility.memoryActorSupported(this._target);
    if (supported) {
      this._memory = new MemoryFront(this._target.client, this._target.form);
    } else {
      this._usingMockMemory = true;
      this._memory = new compatibility.MockMemoryFront();
    }
  }),

  


  _disconnectActors: Task.async(function* () {
    yield this._timeline.destroy();
    yield this._memory.destroy();
  }),

  












  _request: function(actor, method, ...args) {
    
    if (actor == "profiler") {
      let deferred = promise.defer();
      let data = args[0] || {};
      data.to = this._profiler;
      data.type = method;
      this._client.request(data, deferred.resolve);
      return deferred.promise;
    }

    
    if (actor == "timeline") {
      return this._timeline[method].apply(this._timeline, args);
    }

    
    if (actor == "memory") {
      return this._memory[method].apply(this._memory, args);
    }
  }
};








function PerformanceFront(connection) {
  EventEmitter.decorate(this);

  this._request = connection._request;

  
  connection._timeline.on("markers", markers => this.emit("markers", markers));
  connection._timeline.on("frames", (delta, frames) => this.emit("frames", delta, frames));
  connection._timeline.on("memory", (delta, measurement) => this.emit("memory", delta, measurement));
  connection._timeline.on("ticks", (delta, timestamps) => this.emit("ticks", delta, timestamps));

  
  this._usingMockMemory = connection._usingMockMemory;
  this._usingMockTimeline = connection._usingMockTimeline;

  this._pullAllocationSites = this._pullAllocationSites.bind(this);
  this._sitesPullTimeout = 0;
}

PerformanceFront.prototype = {

  








  startRecording: Task.async(function*(options = {}) {
    
    
    let profilerStartTime = yield this._startProfiler(options);
    let timelineStartTime = yield this._startTimeline(options);
    let memoryStartTime = yield this._startMemory(options);

    return {
      profilerStartTime,
      timelineStartTime,
      memoryStartTime
    };
  }),

  








  stopRecording: Task.async(function*(options = {}) {
    let memoryEndTime = yield this._stopMemory(options);
    let timelineEndTime = yield this._stopTimeline(options);
    let profilerData = yield this._request("profiler", "getProfile");

    return {
      
      profile: profilerData.profile,

      
      profilerEndTime: profilerData.currentTime,
      timelineEndTime: timelineEndTime,
      memoryEndTime: memoryEndTime
    };
  }),

  


  _startProfiler: Task.async(function *(options={}) {
    
    
    
    
    let profilerStatus = yield this._request("profiler", "isActive");
    if (profilerStatus.isActive) {
      this.emit("profiler-already-active");
      return profilerStatus.currentTime;
    }

    
    
    let profilerOptions = {
      entries: options.bufferSize,
      interval: options.sampleFrequency ? (1000 / (options.sampleFrequency * 1000)) : void 0
    };

    yield this._request("profiler", "startProfiler", profilerOptions);

    this.emit("profiler-activated");
    return 0;
  }),

  


  _startTimeline: Task.async(function *(options) {
    
    
    return (yield this._request("timeline", "start", options));
  }),

  


  _stopTimeline: Task.async(function *(options) {
    return (yield this._request("timeline", "stop"));
  }),

  


  _startMemory: Task.async(function *(options) {
    if (!options.withAllocations) {
      return 0;
    }
    let memoryStartTime = yield this._startRecordingAllocations(options);
    yield this._pullAllocationSites();
    return memoryStartTime;
  }),

  


  _stopMemory: Task.async(function *(options) {
    if (!options.withAllocations) {
      return 0;
    }
    
    
    
    
    
    yield this._lastPullAllocationSitesFinished;
    clearTimeout(this._sitesPullTimeout);

    return yield this._stopRecordingAllocations();
  }),

  


  _startRecordingAllocations: Task.async(function*(options) {
    yield this._request("memory", "attach");
    let memoryStartTime = yield this._request("memory", "startRecordingAllocations", {
      probability: options.allocationsSampleProbability,
      maxLogLength: options.allocationsMaxLogLength
    });
    return memoryStartTime;
  }),

  


  _stopRecordingAllocations: Task.async(function*() {
    let memoryEndTime = yield this._request("memory", "stopRecordingAllocations");
    yield this._request("memory", "detach");
    return memoryEndTime;
  }),

  



  _pullAllocationSites: Task.async(function *() {
    let deferred = promise.defer();
    this._lastPullAllocationSitesFinished = deferred.promise;

    let isDetached = (yield this._request("memory", "getState")) !== "attached";
    if (isDetached) {
      deferred.resolve();
      return;
    }

    let memoryData = yield this._request("memory", "getAllocations");
    this.emit("allocations", {
      sites: memoryData.allocations,
      timestamps: memoryData.allocationsTimestamps,
      frames: memoryData.frames,
      counts: memoryData.counts
    });

    let delay = DEFAULT_ALLOCATION_SITES_PULL_TIMEOUT;
    this._sitesPullTimeout = setTimeout(this._pullAllocationSites, delay);

    deferred.resolve();
  }),

  


  getMocksInUse: function () {
    return {
      memory: this._usingMockMemory,
      timeline: this._usingMockTimeline
    };
  }
};





function listTabs(client) {
  let deferred = promise.defer();
  client.listTabs(deferred.resolve);
  return deferred.promise;
}

exports.getPerformanceActorsConnection = target => SharedPerformanceActors.forTarget(target);
exports.PerformanceFront = PerformanceFront;
