


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
loader.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");

loader.lazyImporter(this, "gDevTools",
  "resource:///modules/devtools/gDevTools.jsm");





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
  






  open: Task.async(function*() {
    if (this._connected) {
      return;
    }

    
    yield this._target.makeRemote();

    
    yield this._connectProfilerActor();

    
    yield this._connectTimelineActor();

    this._connected = true;

    Services.obs.notifyObservers(null, "performance-actors-connection-opened", null);
  }),

  


  destroy: function () {
    this._disconnectActors();
    this._connected = false;
  },

  


  _connectProfilerActor: Task.async(function*() {
    
    
    if (this._target.chrome) {
      this._profiler = this._target.form.profilerActor;
    }
    
    
    else if (this._target.form && this._target.form.profilerActor) {
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
    
    
    
    
    
    
    
    
    
    
    if (this._target.form && this._target.form.timelineActor) {
      this._timeline = new TimelineFront(this._target.client, this._target.form);
    } else {
      this._timeline = {
        start: () => 0,
        stop: () => 0,
        isRecording: () => false,
        on: () => null,
        off: () => null,
        once: () => promise.reject(),
        destroy: () => null
      };
    }
  },

  


  _disconnectActors: function () {
    this._timeline.destroy();
  },

  












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
  }
};








function PerformanceFront(connection) {
  EventEmitter.decorate(this);

  this._request = connection._request;

  
  connection._timeline.on("markers", markers => this.emit("markers", markers));
  connection._timeline.on("frames", (delta, frames) => this.emit("frames", delta, frames));
  connection._timeline.on("memory", (delta, measurement) => this.emit("memory", delta, measurement));
  connection._timeline.on("ticks", (delta, timestamps) => this.emit("ticks", delta, timestamps));
}

PerformanceFront.prototype = {
  








  startRecording: Task.async(function*(timelineOptions = {}) {
    let profilerStatus = yield this._request("profiler", "isActive");
    let profilerStartTime;

    
    
    
    
    if (!profilerStatus.isActive) {
      
      let profilerOptions = extend({}, this._customProfilerOptions);
      yield this._request("profiler", "startProfiler", profilerOptions);
      profilerStartTime = 0;
      this.emit("profiler-activated");
    } else {
      profilerStartTime = profilerStatus.currentTime;
      this.emit("profiler-already-active");
    }

    
    
    let timelineStartTime = yield this._request("timeline", "start", timelineOptions);

    
    
    return {
      profilerStartTime,
      timelineStartTime
    };
  }),

  






  stopRecording: Task.async(function*() {
    let timelineEndTime = yield this._request("timeline", "stop");
    let profilerData = yield this._request("profiler", "getProfile");

    
    
    return {
      profile: profilerData.profile,
      profilerEndTime: profilerData.currentTime,
      timelineEndTime: timelineEndTime
    };
  }),

  





  _customProfilerOptions: {
    entries: 1000000,
    interval: 1,
    features: ["js"]
  }
};




function listTabs(client) {
  let deferred = promise.defer();
  client.listTabs(deferred.resolve);
  return deferred.promise;
}

exports.getPerformanceActorsConnection = target => SharedPerformanceActors.forTarget(target);
exports.PerformanceFront = PerformanceFront;
