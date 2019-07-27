


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





function callFrontMethod (method) {
  return function () {
    
    
    
    
    if (!this._target || !this._target.client) {
      return;
    }
    return this._front[method].apply(this._front, arguments);
  };
}

exports.MockMemoryFront = MockMemoryFront;
exports.MockTimelineFront = MockTimelineFront;
exports.memoryActorSupported = memoryActorSupported;
exports.timelineActorSupported = timelineActorSupported;
exports.callFrontMethod = callFrontMethod;
