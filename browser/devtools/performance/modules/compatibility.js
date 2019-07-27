


"use strict";

const { Task } = require("resource://gre/modules/Task.jsm");
const { Promise } = require("resource://gre/modules/Promise.jsm");
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
exports.MockMemoryFront = MockMemoryFront;

function MockTimelineFront () {
  MockFront.call(this, [
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








function getProfiler (target) {
  let { promise, resolve } = Promise.defer();
  
  
  if (target.form && target.form.profilerActor) {
    resolve(target.form.profilerActor);
  }
  
  
  else if (target.root && target.root.profilerActor) {
    resolve(target.root.profilerActor);
  }
  
  else {
    target.client.listTabs(({ profilerActor }) => resolve(profilerActor));
  }
  return promise;
}
exports.getProfiler = Task.async(getProfiler);





function legacyRequest (target, actor, method, args) {
  let { promise, resolve } = Promise.defer();
  let data = args[0] || {};
  data.to = actor;
  data.type = method;
  target.client.request(data, resolve);
  return promise;
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
exports.actorCompatibilityBridge = actorCompatibilityBridge;
