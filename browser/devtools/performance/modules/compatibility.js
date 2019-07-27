


"use strict";

const { Task } = require("resource://gre/modules/Task.jsm");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

const REQUIRED_MEMORY_ACTOR_METHODS = [
  "attach", "detach", "startRecordingAllocations", "stopRecordingAllocations", "getAllocations"
];







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
