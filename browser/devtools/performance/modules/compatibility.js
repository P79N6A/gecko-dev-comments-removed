


"use strict";

const { Task } = require("resource://gre/modules/Task.jsm");
loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils", true);

const REQUIRED_MEMORY_ACTOR_METHODS = [
  "attach", "detach", "startRecordingAllocations", "stopRecordingAllocations", "getAllocations"
];




function ProfilerFront (target) {
  this._target = target;
}

ProfilerFront.prototype = {
  
  connect: Task.async(function*() {
    let target = this._target;
    
    
    if (target.form && target.form.profilerActor) {
      this._profiler = target.form.profilerActor;
    }
    
    
    else if (target.root && target.root.profilerActor) {
      this._profiler = target.root.profilerActor;
    }
    
    else {
      this._profiler = (yield listTabs(target.client)).profilerActor;
    }

    
    
    this.traits = {};
    this.traits.filterable = target.getTrait("profilerDataFilterable");
  }),

  




  _request: function (method, ...args) {
    let deferred = promise.defer();
    let data = args[0] || {};
    data.to = this._profiler;
    data.type = method;
    this._target.client.request(data, res => {
      
      
      if (method === "getProfile" && !this.traits.filterable) {
        RecordingUtils.filterSamples(res.profile, data.startTime || 0);
      }

      deferred.resolve(res);
    });
    return deferred.promise;
  }
};

exports.ProfilerFront = ProfilerFront;







function MockFront (blueprint) {
  EventEmitter.decorate(this);

  for (let [funcName, retVal] of blueprint) {
    this[funcName] = (x => typeof x === "function" ? x() : x).bind(this, retVal);
  }
}

function MockMemoryFront () {
  MockFront.call(this, [
    ["initialize"],
    ["destroy"],
    ["attach"],
    ["detach"],
    ["getState", "detached"],
    ["startRecordingAllocations", 0],
    ["stopRecordingAllocations", 0],
    ["getAllocations", createMockAllocations],
  ]);
}
exports.MockMemoryFront = MockMemoryFront;

function MockTimelineFront () {
  MockFront.call(this, [
    ["initialize"],
    ["destroy"],
    ["start", 0],
    ["stop", 0],
  ]);
}
exports.MockTimelineFront = MockTimelineFront;







function createMockAllocations () {
  return {
    allocations: [],
    allocationsTimestamps: [],
    frames: [],
    counts: []
  };
}












function memoryActorSupported (target) {
  
  
  if (target.TEST_MOCK_MEMORY_ACTOR) {
    return false;
  }

  
  
  
  
  return !!target.getTrait("memoryActorAllocations") && target.hasActor("memory");
}
exports.memoryActorSupported = Task.async(memoryActorSupported);








function timelineActorSupported(target) {
  
  
  if (target.TEST_MOCK_TIMELINE_ACTOR) {
    return false;
  }

  return target.hasActor("timeline");
}
exports.timelineActorSupported = Task.async(timelineActorSupported);





function listTabs(client) {
  let deferred = promise.defer();
  client.listTabs(deferred.resolve);
  return deferred.promise;
}

