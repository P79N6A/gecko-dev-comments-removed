


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const { Task } = require("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "extend",
  "sdk/util/object", true);

loader.lazyRequireGetter(this, "Actors",
  "devtools/performance/actors");
loader.lazyRequireGetter(this, "RecordingModel",
  "devtools/performance/recording-model", true);
loader.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");

loader.lazyImporter(this, "gDevTools",
  "resource:///modules/devtools/gDevTools.jsm");


const CONNECTION_PIPE_EVENTS = [
  "timeline-data", "profiler-already-active", "profiler-activated",
  "recording-starting", "recording-started", "recording-stopping", "recording-stopped",
  "profiler-status"
];





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
  this._pendingConsoleRecordings = [];
  this._sitesPullTimeout = 0;
  this._recordings = [];

  this._pipeToConnection = this._pipeToConnection.bind(this);
  this._onTimelineData = this._onTimelineData.bind(this);
  this._onConsoleProfileStart = this._onConsoleProfileStart.bind(this);
  this._onConsoleProfileEnd = this._onConsoleProfileEnd.bind(this);
  this._onProfilerStatus = this._onProfilerStatus.bind(this);
  this._onProfilerUnexpectedlyStopped = this._onProfilerUnexpectedlyStopped.bind(this);

  Services.obs.notifyObservers(null, "performance-actors-connection-created", null);
}

PerformanceActorsConnection.prototype = {

  
  _memorySupported: true,
  _timelineSupported: true,

  






  open: Task.async(function*() {
    if (this._connecting) {
      return this._connecting.promise;
    }

    
    
    this._connecting = promise.defer();

    
    yield this._target.makeRemote();

    
    
    
    
    yield this._connectActors();
    yield this._registerListeners();

    this._connecting.resolve();
    Services.obs.notifyObservers(null, "performance-actors-connection-opened", null);
  }),

  


  destroy: Task.async(function*() {
    if (this._connecting) {
      yield this._connecting.promise;
    } else {
      return;
    }

    yield this._unregisterListeners();
    yield this._disconnectActors();

    this._connecting = null;
    this._profiler = null;
    this._timeline = null;
    this._memory = null;
    this._target = null;
    this._client = null;
  }),

  



  _connectActors: Task.async(function*() {
    this._profiler = new Actors.ProfilerFront(this._target);
    this._memory = new Actors.MemoryFront(this._target);
    this._timeline = new Actors.TimelineFront(this._target);

    yield promise.all([
      this._profiler.connect(),
      this._memory.connect(),
      this._timeline.connect()
    ]);

    
    
    this._memorySupported = !this._memory.IS_MOCK;
    this._timelineSupported = !this._timeline.IS_MOCK;
  }),

  



  _registerListeners: function () {
    this._timeline.on("timeline-data", this._onTimelineData);
    this._memory.on("timeline-data", this._onTimelineData);
    this._profiler.on("console-profile-start", this._onConsoleProfileStart);
    this._profiler.on("console-profile-end", this._onConsoleProfileEnd);
    this._profiler.on("profiler-stopped", this._onProfilerUnexpectedlyStopped);
    this._profiler.on("profiler-already-active", this._pipeToConnection);
    this._profiler.on("profiler-activated", this._pipeToConnection);
    this._profiler.on("profiler-status", this._onProfilerStatus);
  },

  


  _unregisterListeners: function () {
    this._timeline.off("timeline-data", this._onTimelineData);
    this._memory.off("timeline-data", this._onTimelineData);
    this._profiler.off("console-profile-start", this._onConsoleProfileStart);
    this._profiler.off("console-profile-end", this._onConsoleProfileEnd);
    this._profiler.off("profiler-stopped", this._onProfilerUnexpectedlyStopped);
    this._profiler.off("profiler-already-active", this._pipeToConnection);
    this._profiler.off("profiler-activated", this._pipeToConnection);
    this._profiler.off("profiler-status", this._onProfilerStatus);
  },

  


  _disconnectActors: Task.async(function* () {
    yield promise.all([
      this._profiler.destroy(),
      this._timeline.destroy(),
      this._memory.destroy()
    ]);
  }),

  








  _onConsoleProfileStart: Task.async(function *(_, { profileLabel, currentTime: startTime }) {
    let recordings = this._recordings;

    
    if (recordings.find(e => e.getLabel() === profileLabel)) {
      return;
    }

    
    
    
    
    yield gDevTools.getToolbox(this._target).loadTool("performance");

    let model = yield this.startRecording(extend(getRecordingModelPrefs(), {
      console: true,
      label: profileLabel
    }));
  }),

  








  _onConsoleProfileEnd: Task.async(function *(_, data) {
    
    
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

    yield this.stopRecording(model);
  }),

 


  _onProfilerUnexpectedlyStopped: function () {
    Cu.reportError("Profiler unexpectedly stopped.", arguments);
  },

  









  _onTimelineData: function (_, ...data) {
    this._recordings.forEach(e => e._addTimelineData.apply(e, data));
    this.emit("timeline-data", ...data);
  },

  


  _onProfilerStatus: function (_, data) {
    
    
    if (!data) {
      return;
    }
    
    
    if (data.position !== void 0) {
      this._recordings.forEach(e => e._addBufferStatusData.call(e, data));
    }
    this.emit("profiler-status", data);
  },

  








  startRecording: Task.async(function*(options = {}) {
    let model = new RecordingModel(options);
    this.emit("recording-starting", model);

    
    
    
    
    let { startTime, position, generation, totalSize } = yield this._profiler.start(options);
    let timelineStartTime = yield this._timeline.start(options);
    let memoryStartTime = yield this._memory.start(options);

    let data = {
      profilerStartTime: startTime, timelineStartTime, memoryStartTime,
      generation, position, totalSize
    };

    
    
    model._populate(data);
    this._recordings.push(model);

    this.emit("recording-started", model);
    return model;
  }),

  







  stopRecording: Task.async(function*(model) {
    
    
    if (this._recordings.indexOf(model) === -1) {
      return;
    }

    
    
    
    let endTime = Date.now();
    model._onStoppingRecording(endTime);
    this.emit("recording-stopping", model);

    
    
    
    
    
    
    
    
    this._recordings.splice(this._recordings.indexOf(model), 1);

    let config = model.getConfiguration();
    let startTime = model.getProfilerStartTime();
    let profilerData = yield this._profiler.getProfile({ startTime });
    let memoryEndTime = Date.now();
    let timelineEndTime = Date.now();

    
    
    
    
    if (!this.isRecording()) {
      
      
      yield this._profiler.stop();
      memoryEndTime = yield this._memory.stop(config);
      timelineEndTime = yield this._timeline.stop(config);
    }

    
    model._onStopRecording({
      
      profile: profilerData.profile,

      
      profilerEndTime: profilerData.currentTime,
      timelineEndTime: timelineEndTime,
      memoryEndTime: memoryEndTime
    });

    this.emit("recording-stopped", model);
    return model;
  }),

  





  isRecording: function () {
    return this._recordings.some(recording => recording.isRecording());
  },

  



  _pipeToConnection: function (eventName, ...args) {
    this.emit(eventName, ...args);
  },

  toString: () => "[object PerformanceActorsConnection]"
};








function PerformanceFront(connection) {
  EventEmitter.decorate(this);

  this._connection = connection;

  
  this._memorySupported = connection._memorySupported;
  this._timelineSupported = connection._timelineSupported;

  
  
  CONNECTION_PIPE_EVENTS.forEach(eventName => this._connection.on(eventName, () => this.emit.apply(this, arguments)));
}

PerformanceFront.prototype = {

  










  startRecording: function (options) {
    return this._connection.startRecording(options);
  },

  








  stopRecording: function (model) {
    return this._connection.stopRecording(model);
  },

  




  getActorSupport: function () {
    return {
      memory: this._memorySupported,
      timeline: this._timelineSupported
    };
  },

  




  isRecording: function () {
    return this._connection.isRecording();
  },

  


  _request: function (actorName, method, ...args) {
    if (!gDevTools.testing) {
      throw new Error("PerformanceFront._request may only be used in tests.");
    }
    let actor = this._connection[`_${actorName}`];
    return actor[method].apply(actor, args);
  },

  toString: () => "[object PerformanceFront]"
};




function getRecordingModelPrefs () {
  return {
    withMarkers: true,
    withMemory: Services.prefs.getBoolPref("devtools.performance.ui.enable-memory"),
    withTicks: Services.prefs.getBoolPref("devtools.performance.ui.enable-framerate"),
    withAllocations: Services.prefs.getBoolPref("devtools.performance.ui.enable-allocations"),
    allocationsSampleProbability: +Services.prefs.getCharPref("devtools.performance.memory.sample-probability"),
    allocationsMaxLogLength: Services.prefs.getIntPref("devtools.performance.memory.max-log-length")
  };
}

exports.getPerformanceActorsConnection = t => SharedPerformanceActors.forTarget(t);
exports.PerformanceFront = PerformanceFront;
