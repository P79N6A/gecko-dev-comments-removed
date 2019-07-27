


"use strict";

const { Task } = require("resource://gre/modules/Task.jsm");
const { Promise } = require("resource://gre/modules/Promise.jsm");
const {
  actorCompatibilityBridge, getProfiler,
  MockMemoryFront, MockTimelineFront,
  memoryActorSupported, timelineActorSupported
} = require("devtools/performance/compatibility");

loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils", true);
loader.lazyRequireGetter(this, "TimelineFront",
  "devtools/server/actors/timeline", true);
loader.lazyRequireGetter(this, "MemoryFront",
  "devtools/server/actors/memory", true);
loader.lazyRequireGetter(this, "timers",
  "resource://gre/modules/Timer.jsm");


const ALLOCATION_SITE_POLL_TIMER = 200; 

const MEMORY_ACTOR_METHODS = [
  "destroy", "attach", "detach", "getState", "getAllocationsSettings",
  "getAllocations", "startRecordingAllocations", "stopRecordingAllocations"
];

const TIMELINE_ACTOR_METHODS = [
  "start", "stop",
];

const PROFILER_ACTOR_METHODS = [
  "isActive", "startProfiler", "getStartOptions", "stopProfiler",
  "registerEventNotifications", "unregisterEventNotifications"
];




function ProfilerFrontFacade (target) {
  this._target = target;
  this._onProfilerEvent = this._onProfilerEvent.bind(this);
  EventEmitter.decorate(this);
}

ProfilerFrontFacade.prototype = {
  EVENTS: ["console-api-profiler", "profiler-stopped"],

  
  connect: Task.async(function*() {
    let target = this._target;
    this._actor = yield getProfiler(target);

    
    
    this.traits = {};
    this.traits.filterable = target.getTrait("profilerDataFilterable");

    
    
    yield this.registerEventNotifications({ events: this.EVENTS });
    
    target.client.addListener("eventNotification", this._onProfilerEvent);
  }),

  


  destroy: Task.async(function *() {
    yield this.unregisterEventNotifications({ events: this.EVENTS });
    
    this._target.client.removeListener("eventNotification", this._onProfilerEvent);
  }),

  


  start: Task.async(function *(options={}) {
    
    
    
    
    let profilerStatus = yield this.isActive();
    if (profilerStatus.isActive) {
      this.emit("profiler-already-active");
      return profilerStatus.currentTime;
    }

    
    
    let profilerOptions = {
      entries: options.bufferSize,
      interval: options.sampleFrequency ? (1000 / (options.sampleFrequency * 1000)) : void 0
    };

    yield this.startProfiler(profilerOptions);

    this.emit("profiler-activated");
    return 0;
  }),

  


  getProfile: Task.async(function *(options) {
    let profilerData = yield (actorCompatibilityBridge("getProfile").call(this, options));
    
    
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

  toString: () => "[object ProfilerFrontFacade]"
};


PROFILER_ACTOR_METHODS.forEach(method => ProfilerFrontFacade.prototype[method] = actorCompatibilityBridge(method));
exports.ProfilerFront = ProfilerFrontFacade;




function TimelineFrontFacade (target) {
  this._target = target;
  EventEmitter.decorate(this);
}

TimelineFrontFacade.prototype = {
  EVENTS: ["markers", "frames", "memory", "ticks"],

  connect: Task.async(function*() {
    let supported = yield timelineActorSupported(this._target);
    this._actor = supported ?
                  new TimelineFront(this._target.client, this._target.form) :
                  new MockTimelineFront();

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


TIMELINE_ACTOR_METHODS.forEach(method => TimelineFrontFacade.prototype[method] = actorCompatibilityBridge(method));
exports.TimelineFront = TimelineFrontFacade;




function MemoryFrontFacade (target) {
  this._target = target;
  this._pullAllocationSites = this._pullAllocationSites.bind(this);
  EventEmitter.decorate(this);
}

MemoryFrontFacade.prototype = {
  connect: Task.async(function*() {
    let supported = yield memoryActorSupported(this._target);
    this._actor = supported ?
                  new MemoryFront(this._target.client, this._target.form) :
                  new MockMemoryFront();

    this.IS_MOCK = !supported;
  }),

  


  start: Task.async(function *(options) {
    if (!options.withAllocations) {
      return 0;
    }

    yield this.attach();

    let startTime = yield this.startRecordingAllocations({
      probability: options.allocationsSampleProbability,
      maxLogLength: options.allocationsMaxLogLength
    });

    yield this._pullAllocationSites();

    return startTime;
  }),

  


  stop: Task.async(function *(options) {
    if (!options.withAllocations) {
      return 0;
    }

    
    
    
    
    
    yield this._lastPullAllocationSitesFinished;
    timers.clearTimeout(this._sitesPullTimeout);

    let endTime = yield this.stopRecordingAllocations();
    yield this.detach();

    return endTime;
  }),

  





  _pullAllocationSites: Task.async(function *() {
    let { promise, resolve } = Promise.defer();
    this._lastPullAllocationSitesFinished = promise;

    if ((yield this.getState()) !== "attached") {
      resolve();
      return;
    }

    let memoryData = yield this.getAllocations();
    
    
    this.emit("timeline-data", "allocations", {
      sites: memoryData.allocations,
      timestamps: memoryData.allocationsTimestamps,
      frames: memoryData.frames,
      counts: memoryData.counts
    });

    this._sitesPullTimeout = timers.setTimeout(this._pullAllocationSites, ALLOCATION_SITE_POLL_TIMER);

    resolve();
  }),

  toString: () => "[object MemoryFrontFacade]"
};


MEMORY_ACTOR_METHODS.forEach(method => MemoryFrontFacade.prototype[method] = actorCompatibilityBridge(method));
exports.MemoryFront = MemoryFrontFacade;
