


"use strict";

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");







function MockFront (blueprint) {
  EventEmitter.decorate(this);

  for (let [funcName, retVal] of blueprint) {
    this[funcName] = (x => typeof x === "function" ? x() : x).bind(this, retVal);
  }
}

function MockMemoryFront () {
  MockFront.call(this, [
    ["start", 0], 
    ["stop", 0], 
    ["destroy"],
    ["attach"],
    ["detach"],
    ["getState", "detached"],
    ["startRecordingAllocations", 0],
    ["stopRecordingAllocations", 0],
    ["getAllocations", createMockAllocations],
  ]);
}

function MockTimelineFront () {
  MockFront.call(this, [
    ["destroy"],
    ["start", 0],
    ["stop", 0],
  ]);
}







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








function timelineActorSupported(target) {
  
  
  if (target.TEST_MOCK_TIMELINE_ACTOR) {
    return false;
  }

  return target.hasActor("timeline");
}








function getProfiler (target) {
  let deferred = promise.defer();
  
  
  if (target.form && target.form.profilerActor) {
    deferred.resolve(target.form.profilerActor);
  }
  
  
  else if (target.root && target.root.profilerActor) {
    deferred.resolve(target.root.profilerActor);
  }
  
  else {
    target.client.listTabs(({ profilerActor }) => deferred.resolve(profilerActor));
  }
  return deferred.promise;
}





function legacyRequest (target, actor, method, args) {
  let deferred = promise.defer();
  let data = args[0] || {};
  data.to = actor;
  data.type = method;
  target.client.request(data, deferred.resolve);
  return deferred.promise;
}







function actorCompatibilityBridge (method) {
  return function () {
    
    
    
    
    if (!this._target || !this._target.client) {
      return;
    }
    
    
    
    
    
    
    
    if (this.IS_MOCK || this._actor.request) {
      return this._actor[method].apply(this._actor, arguments);
    }
    else {
      return legacyRequest(this._target, this._actor, method, arguments);
    }
  };
}

exports.MockMemoryFront = MockMemoryFront;
exports.MockTimelineFront = MockTimelineFront;
exports.memoryActorSupported = memoryActorSupported;
exports.timelineActorSupported = timelineActorSupported;
exports.getProfiler = getProfiler;
exports.actorCompatibilityBridge = actorCompatibilityBridge;
