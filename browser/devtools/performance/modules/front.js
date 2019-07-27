


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const { Task } = require("resource://gre/modules/Task.jsm");
const { extend } = require("sdk/util/object");
const { RecordingModel } = require("devtools/performance/recording-model");

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
loader.lazyImporter(this, "Promise",
  "resource://gre/modules/Promise.jsm");



const DEFAULT_ALLOCATION_SITES_PULL_TIMEOUT = 200; 


const CONNECTION_PIPE_EVENTS = [
  "console-profile-start", "console-profile-ending", "console-profile-end",
  "timeline-data", "profiler-already-active", "profiler-activated"
];


const PROFILER_EVENTS = ["console-api-profiler", "profiler-stopped"];





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
  this._pendingConsoleRecordings = [];
  this._sitesPullTimeout = 0;
  this._recordings = [];

  this._onTimelineMarkers = this._onTimelineMarkers.bind(this);
  this._onTimelineFrames = this._onTimelineFrames.bind(this);
  this._onTimelineMemory = this._onTimelineMemory.bind(this);
  this._onTimelineTicks = this._onTimelineTicks.bind(this);
  this._onProfilerEvent = this._onProfilerEvent.bind(this);
  this._pullAllocationSites = this._pullAllocationSites.bind(this);

  Services.obs.notifyObservers(null, "performance-actors-connection-created", null);
}

PerformanceActorsConnection.prototype = {

  
  _usingMockMemory: false,
  _usingMockTimeline: false,

  






  open: Task.async(function*() {
    if (this._connecting) {
      return this._connecting.promise;
    }

    
    
    this._connecting = Promise.defer();

    
    yield this._target.makeRemote();

    
    
    
    
    yield this._connectProfilerActor();
    yield this._connectTimelineActor();
    yield this._connectMemoryActor();

    yield this._registerListeners();

    this._connected = true;

    this._connecting.resolve();
    Services.obs.notifyObservers(null, "performance-actors-connection-opened", null);
  }),

  


  destroy: Task.async(function*() {
    if (this._connecting && !this._connected) {
      yield this._connecting.promise;
    } else if (!this._connected) {
      return;
    }

    yield this._unregisterListeners();
    yield this._disconnectActors();

    this._memory = this._timeline = this._profiler = this._target = this._client = null;
    this._connected = false;
    this._connecting = null;
  }),

  



  _connectProfilerActor: Task.async(function*() {
    this._profiler = new compatibility.ProfilerFront(this._target);
    yield this._profiler.connect();
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

  



  _registerListeners: Task.async(function*() {
    
    this._timeline.on("markers", this._onTimelineMarkers);
    this._timeline.on("frames", this._onTimelineFrames);
    this._timeline.on("memory", this._onTimelineMemory);
    this._timeline.on("ticks", this._onTimelineTicks);

    
    yield this._request("profiler", "registerEventNotifications", { events: PROFILER_EVENTS });
    this._client.addListener("eventNotification", this._onProfilerEvent);
  }),

  


  _unregisterListeners: Task.async(function*() {
    this._timeline.off("markers", this._onTimelineMarkers);
    this._timeline.off("frames", this._onTimelineFrames);
    this._timeline.off("memory", this._onTimelineMemory);
    this._timeline.off("ticks", this._onTimelineTicks);

    yield this._request("profiler", "unregisterEventNotifications", { events: PROFILER_EVENTS });
    this._client.removeListener("eventNotification", this._onProfilerEvent);
  }),

  


  _disconnectActors: Task.async(function* () {
    yield this._timeline.destroy();
    yield this._memory.destroy();
  }),

  












  _request: function(actor, method, ...args) {
    
    if (actor == "profiler") {
      return this._profiler._request(method, ...args);
    }

    
    if (actor == "timeline") {
      return this._timeline[method].apply(this._timeline, args);
    }

    
    if (actor == "memory") {
      return this._memory[method].apply(this._memory, args);
    }
  },

  





  _onProfilerEvent: function (_, { topic, subject, details }) {
    if (topic === "console-api-profiler") {
      if (subject.action === "profile") {
        this._onConsoleProfileStart(details);
      } else if (subject.action === "profileEnd") {
        this._onConsoleProfileEnd(details);
      }
    } else if (topic === "profiler-stopped") {
      this._onProfilerUnexpectedlyStopped();
    }
  },

  


  _onProfilerUnexpectedlyStopped: function () {

  },

  








  _onConsoleProfileStart: Task.async(function *({ profileLabel, currentTime: startTime }) {
    let recordings = this._recordings;

    
    if (recordings.find(e => e.getLabel() === profileLabel)) {
      return;
    }

    
    
    
    
    yield gDevTools.getToolbox(this._target).loadTool("performance");

    let model = yield this.startRecording(extend(getRecordingModelPrefs(), {
      console: true,
      label: profileLabel
    }));

    this.emit("console-profile-start", model);
  }),

  








  _onConsoleProfileEnd: Task.async(function *(data) {
    
    
    if (!data) {
      return;
    }
    let { profileLabel, currentTime: endTime } = data;

    let pending = this._recordings.filter(r => r.isConsole() && r.isRecording());
    if (pending.length === 0) {
      return;
    }

    let model;
    
    
    if (profileLabel) {
      model = pending.find(e => e.getLabel() === profileLabel);
    }
    
    else {
      model = pending[pending.length - 1];
    }

    
    
    if (!model) {
      Cu.reportError("console.profileEnd() called with label that does not match a recording.");
      return;
    }

    this.emit("console-profile-ending", model);
    yield this.stopRecording(model);
    this.emit("console-profile-end", model);
  }),

  



  _onTimelineMarkers: function (markers) { this._onTimelineData("markers", markers); },
  _onTimelineFrames: function (delta, frames) { this._onTimelineData("frames", delta, frames); },
  _onTimelineMemory: function (delta, measurement) { this._onTimelineData("memory", delta, measurement); },
  _onTimelineTicks: function (delta, timestamps) { this._onTimelineData("ticks", delta, timestamps); },

  










  _onTimelineData: function (...data) {
    this._recordings.forEach(e => e.addTimelineData.apply(e, data));
    this.emit("timeline-data", ...data);
  },

  








  startRecording: Task.async(function*(options = {}) {
    let model = new RecordingModel(options);
    
    
    let profilerStartTime = yield this._startProfiler();
    let timelineStartTime = yield this._startTimeline(options);
    let memoryStartTime = yield this._startMemory(options);

    let data = {
      profilerStartTime,
      timelineStartTime,
      memoryStartTime
    };

    
    
    model.populate(data);
    this._recordings.push(model);

    return model;
  }),

  







  stopRecording: Task.async(function*(model) {
    
    
    if (this._recordings.indexOf(model) === -1) {
      return;
    }

    
    
    
    
    
    
    
    
    this._recordings.splice(this._recordings.indexOf(model), 1);

    let config = model.getConfiguration();
    let startTime = model.getProfilerStartTime();
    let profilerData = yield this._request("profiler", "getProfile", { startTime });
    let memoryEndTime = Date.now();
    let timelineEndTime = Date.now();

    
    
    
    
    if (!this.isRecording()) {
      memoryEndTime = yield this._stopMemory(config);
      timelineEndTime = yield this._stopTimeline(config);
    }

    
    model._onStopRecording({
      
      profile: profilerData.profile,

      
      profilerEndTime: profilerData.currentTime,
      timelineEndTime: timelineEndTime,
      memoryEndTime: memoryEndTime
    });

    return model;
  }),

  





  isRecording: function () {
    return this._recordings.some(recording => recording.isRecording());
  },

  


  _startProfiler: Task.async(function *() {
    
    
    
    
    let profilerStatus = yield this._request("profiler", "isActive");
    if (profilerStatus.isActive) {
      this.emit("profiler-already-active");
      return profilerStatus.currentTime;
    }

    
    
    
    let profilerOptions = this._customProfilerOptions || {};
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

    this._onTimelineData("allocations", {
      sites: memoryData.allocations,
      timestamps: memoryData.allocationsTimestamps,
      frames: memoryData.frames,
      counts: memoryData.counts
    });

    let delay = DEFAULT_ALLOCATION_SITES_PULL_TIMEOUT;
    this._sitesPullTimeout = setTimeout(this._pullAllocationSites, delay);

    deferred.resolve();
  }),

  toString: () => "[object PerformanceActorsConnection]"
};








function PerformanceFront(connection) {
  EventEmitter.decorate(this);

  this._connection = connection;
  this._request = connection._request;

  
  this._usingMockMemory = connection._usingMockMemory;
  this._usingMockTimeline = connection._usingMockTimeline;

  
  
  CONNECTION_PIPE_EVENTS.forEach(eventName => this._connection.on(eventName, () => this.emit.apply(this, arguments)));
}

PerformanceFront.prototype = {

  









  startRecording: function (options) {
    return this._connection.startRecording(options);
  },

  








  stopRecording: function (model) {
    return this._connection.stopRecording(model);
  },

  


  getMocksInUse: function () {
    return {
      memory: this._usingMockMemory,
      timeline: this._usingMockTimeline
    };
  }
};




function getRecordingModelPrefs () {
  return {
    withMemory: Services.prefs.getBoolPref("devtools.performance.ui.enable-memory"),
    withTicks: Services.prefs.getBoolPref("devtools.performance.ui.enable-framerate"),
    withAllocations: Services.prefs.getBoolPref("devtools.performance.ui.enable-memory"),
    allocationsSampleProbability: +Services.prefs.getCharPref("devtools.performance.memory.sample-probability"),
    allocationsMaxLogLength: Services.prefs.getIntPref("devtools.performance.memory.max-log-length")
  };
}

exports.getPerformanceActorsConnection = target => SharedPerformanceActors.forTarget(target);
exports.PerformanceFront = PerformanceFront;
