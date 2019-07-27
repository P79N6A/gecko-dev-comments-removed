


"use strict";

const { Task } = require("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "Poller",
  "devtools/shared/poller", true);

loader.lazyRequireGetter(this, "CompatUtils",
  "devtools/performance/compatibility");
loader.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils");
loader.lazyRequireGetter(this, "TimelineFront",
  "devtools/server/actors/timeline", true);
loader.lazyRequireGetter(this, "MemoryFront",
  "devtools/server/actors/memory", true);


const ALLOCATION_SITE_POLL_TIMER = 200; 


const PROFILER_CHECK_TIMER = 5000; 

const MEMORY_ACTOR_METHODS = [
  "attach", "detach", "getState", "getAllocationsSettings",
  "getAllocations", "startRecordingAllocations", "stopRecordingAllocations"
];

const TIMELINE_ACTOR_METHODS = [
  "start", "stop",
];

const PROFILER_ACTOR_METHODS = [
  "startProfiler", "getStartOptions", "stopProfiler",
  "registerEventNotifications", "unregisterEventNotifications"
];




function ProfilerFrontFacade (target) {
  this._target = target;
  this._onProfilerEvent = this._onProfilerEvent.bind(this);
  this._checkProfilerStatus = this._checkProfilerStatus.bind(this);
  this._PROFILER_CHECK_TIMER = this._target.TEST_MOCK_PROFILER_CHECK_TIMER || PROFILER_CHECK_TIMER;

  EventEmitter.decorate(this);
}

ProfilerFrontFacade.prototype = {
  EVENTS: ["console-api-profiler", "profiler-stopped"],

  
  connect: Task.async(function*() {
    let target = this._target;
    this._actor = yield CompatUtils.getProfiler(target);

    
    
    this.traits = {};
    this.traits.filterable = target.getTrait("profilerDataFilterable");

    
    
    yield this.registerEventNotifications({ events: this.EVENTS });
    
    target.client.addListener("eventNotification", this._onProfilerEvent);
  }),

  


  destroy: Task.async(function *() {
    if (this._poller) {
      yield this._poller.destroy();
    }
    yield this.unregisterEventNotifications({ events: this.EVENTS });
    
    this._target.client.removeListener("eventNotification", this._onProfilerEvent);
  }),

  





  start: Task.async(function *(options={}) {
    
    
    
    if (!this._poller) {
      this._poller = new Poller(this._checkProfilerStatus, this._PROFILER_CHECK_TIMER, false);
    }
    if (!this._poller.isPolling()) {
      this._poller.on();
    }

    
    
    
    
    let { isActive, currentTime, position, generation, totalSize } = yield this.getStatus();

    if (isActive) {
      this.emit("profiler-already-active");
      return { startTime: currentTime, position, generation, totalSize };
    }

    
    
    let profilerOptions = {
      entries: options.bufferSize,
      interval: options.sampleFrequency ? (1000 / (options.sampleFrequency * 1000)) : void 0
    };

    let startInfo = yield this.startProfiler(profilerOptions);
    let startTime = 0;
    if ('currentTime' in startInfo) {
      startTime = startInfo.currentTime;
    }

    this.emit("profiler-activated");
    return { startTime, position, generation, totalSize };
  }),

  




  stop: Task.async(function *() {
    yield this._poller.off();
  }),

  


  getStatus: Task.async(function *() {
    let data = yield (CompatUtils.actorCompatibilityBridge("isActive").call(this));
    
    
    if (!data) {
      return;
    }

    
    
    
    if (this._target.TEST_PROFILER_FILTER_STATUS) {
      data = Object.keys(data).reduce((acc, prop) => {
        if (this._target.TEST_PROFILER_FILTER_STATUS.indexOf(prop) === -1) {
          acc[prop] = data[prop];
        }
        return acc;
      }, {});
    }

    this.emit("profiler-status", data);
    return data;
  }),

  


  getProfile: Task.async(function *(options) {
    let profilerData = yield (CompatUtils.actorCompatibilityBridge("getProfile").call(this, options));
    
    
    if (profilerData.profile.meta.version === 2) {
      RecordingUtils.deflateProfile(profilerData.profile);
    }

    
    
    if (!this.traits.filterable) {
      RecordingUtils.filterSamples(profilerData.profile, options.startTime || 0);
    }

    return profilerData;
  }),

  





  _onProfilerEvent: function (_, { topic, subject, details }) {
    if (topic === "console-api-profiler") {
      if (subject.action === "profile") {
        this.emit("console-profile-start", details);
      } else if (subject.action === "profileEnd") {
        this.emit("console-profile-end", details);
      }
    } else if (topic === "profiler-stopped") {
      this.emit("profiler-stopped");
    }
  },

  _checkProfilerStatus: Task.async(function *() {
    
    yield this.getStatus();
  }),

  toString: () => "[object ProfilerFrontFacade]"
};




function TimelineFrontFacade (target) {
  this._target = target;
  EventEmitter.decorate(this);
}

TimelineFrontFacade.prototype = {
  EVENTS: ["markers", "frames", "memory", "ticks"],

  connect: Task.async(function*() {
    let supported = yield CompatUtils.timelineActorSupported(this._target);
    this._actor = supported ?
                  new TimelineFront(this._target.client, this._target.form) :
                  new CompatUtils.MockTimelineFront();

    this.IS_MOCK = !supported;

    
    
    this.EVENTS.forEach(type => {
      let handler = this[`_on${type}`] = this._onTimelineData.bind(this, type);
      this._actor.on(type, handler);
    });
  }),

  



  destroy: Task.async(function *() {
    this.EVENTS.forEach(type => this._actor.off(type, this[`_on${type}`]));
    yield this._actor.destroy();
  }),

  



  _onTimelineData: function (type, ...data) {
    this.emit("timeline-data", type, ...data);
  },

  toString: () => "[object TimelineFrontFacade]"
};




function MemoryFrontFacade (target) {
  this._target = target;
  this._pullAllocationSites = this._pullAllocationSites.bind(this);

  EventEmitter.decorate(this);
}

MemoryFrontFacade.prototype = {
  connect: Task.async(function*() {
    let supported = yield CompatUtils.memoryActorSupported(this._target);
    this._actor = supported ?
                  new MemoryFront(this._target.client, this._target.form) :
                  new CompatUtils.MockMemoryFront();

    this.IS_MOCK = !supported;
  }),

  


  destroy: Task.async(function *() {
    if (this._poller) {
      yield this._poller.destroy();
    }
    yield this._actor.destroy();
  }),

  


  start: Task.async(function *(options) {
    if (!options.withAllocations) {
      return 0;
    }

    yield this.attach();

    
    
    let allocationOptions = {};
    if (options.allocationsSampleProbability !== void 0) {
      allocationOptions.probability = options.allocationsSampleProbability;
    }
    if (options.allocationsMaxLogLength !== void 0) {
      allocationOptions.maxLogLength = options.allocationsMaxLogLength;
    }

    let startTime = yield this.startRecordingAllocations(allocationOptions);

    if (!this._poller) {
      this._poller = new Poller(this._pullAllocationSites, ALLOCATION_SITE_POLL_TIMER, false);
    }
    if (!this._poller.isPolling()) {
      this._poller.on();
    }

    return startTime;
  }),

  


  stop: Task.async(function *(options) {
    if (!options.withAllocations) {
      return 0;
    }

    
    
    
    
    
    yield this._poller.off();
    yield this._lastPullAllocationSitesFinished;

    let endTime = yield this.stopRecordingAllocations();
    yield this.detach();

    return endTime;
  }),

  





  _pullAllocationSites: Task.async(function *() {
    let deferred = promise.defer();
    this._lastPullAllocationSitesFinished = deferred.promise;

    if ((yield this.getState()) !== "attached") {
      deferred.resolve();
      return;
    }

    let memoryData = yield this.getAllocations();
    
    
    this.emit("timeline-data", "allocations", {
      sites: memoryData.allocations,
      timestamps: memoryData.allocationsTimestamps,
      frames: memoryData.frames,
      counts: memoryData.counts
    });

    deferred.resolve();
  }),

  toString: () => "[object MemoryFrontFacade]"
};


PROFILER_ACTOR_METHODS.forEach(m => ProfilerFrontFacade.prototype[m] = CompatUtils.actorCompatibilityBridge(m));
TIMELINE_ACTOR_METHODS.forEach(m => TimelineFrontFacade.prototype[m] = CompatUtils.actorCompatibilityBridge(m));
MEMORY_ACTOR_METHODS.forEach(m => MemoryFrontFacade.prototype[m] = CompatUtils.actorCompatibilityBridge(m));

exports.ProfilerFront = ProfilerFrontFacade;
exports.TimelineFront = TimelineFrontFacade;
exports.MemoryFront = MemoryFrontFacade;
