


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const { extend } = require("sdk/util/object");
const { Task } = require("resource://gre/modules/Task.jsm");

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

let showTimelineMemory = () => Services.prefs.getBoolPref("devtools.performance.ui.show-timeline-memory");




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
        start: () => {},
        stop: () => {},
        isRecording: () => false,
        on: () => {},
        off: () => {},
        destroy: () => {}
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
  connection._timeline.on("memory", (delta, measurement) => this.emit("memory", delta, measurement));
  connection._timeline.on("ticks", (delta, timestamps) => this.emit("ticks", delta, timestamps));
}

PerformanceFront.prototype = {
  





  startRecording: Task.async(function*() {
    let { isActive, currentTime } = yield this._request("profiler", "isActive");

    
    
    
    
    if (!isActive) {
      
      
      let options = extend({}, this._customPerformanceOptions);
      yield this._request("profiler", "startProfiler", options);
      this._profilingStartTime = 0;
      this.emit("profiler-activated");
    } else {
      this._profilingStartTime = currentTime;
      this.emit("profiler-already-active");
    }

    
    
    let withMemory = showTimelineMemory();
    yield this._request("timeline", "start", { withTicks: true, withMemory: withMemory });
  }),

  






  stopRecording: Task.async(function*() {
    
    
    let profilerData = yield this._request("profiler", "getProfile");
    filterSamples(profilerData, this._profilingStartTime);
    offsetSampleTimes(profilerData, this._profilingStartTime);

    yield this._request("timeline", "stop");

    
    return {
      recordingDuration: profilerData.currentTime - this._profilingStartTime,
      profilerData: profilerData
    };
  }),

  





  _customPerformanceOptions: {
    entries: 1000000,
    interval: 1,
    features: ["js"]
  }
};










function filterSamples(profilerData, profilingStartTime) {
  let firstThread = profilerData.profile.threads[0];

  firstThread.samples = firstThread.samples.filter(e => {
    return e.time >= profilingStartTime;
  });
}









function offsetSampleTimes(profilerData, timeOffset) {
  let firstThreadSamples = profilerData.profile.threads[0].samples;

  for (let sample of firstThreadSamples) {
    sample.time -= timeOffset;
  }
}




function listTabs(client) {
  let deferred = promise.defer();
  client.listTabs(deferred.resolve);
  return deferred.promise;
}

exports.getPerformanceActorsConnection = target => SharedPerformanceActors.forTarget(target);
exports.PerformanceFront = PerformanceFront;
