


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

let require = devtools.require;
devtools.lazyRequireGetter(this, "Services");
devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
devtools.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");
devtools.lazyRequireGetter(this, "FramerateFront",
  "devtools/server/actors/framerate", true);
devtools.lazyRequireGetter(this, "L10N",
  "devtools/profiler/global", true);
devtools.lazyImporter(this, "LineGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyRequireGetter(this, "CallView",
  "devtools/profiler/tree-view", true);
devtools.lazyRequireGetter(this, "ThreadNode",
  "devtools/profiler/tree-model", true);


const EVENTS = {
  
  RECORDING_STARTED: "Performance:RecordingStarted",
  RECORDING_STOPPED: "Performance:RecordingStopped",
  
  TIMELINE_DATA: "Performance:TimelineData",

  
  UI_START_RECORDING: "Performance:UI:StartRecording",
  UI_STOP_RECORDING: "Performance:UI:StopRecording",

  
  OVERVIEW_RENDERED: "Performance:UI:OverviewRendered",

  
  CALL_TREE_RENDERED: "Performance:UI:CallTreeRendered"
};




let gToolbox, gTarget, gFront;




let startupPerformance = Task.async(function*() {
  yield promise.all([
    PrefObserver.register(),
    PerformanceController.initialize(),
    PerformanceView.initialize()
  ]);
});




let shutdownPerformance = Task.async(function*() {
  yield promise.all([
    PrefObserver.unregister(),
    PerformanceController.destroy(),
    PerformanceView.destroy()
  ]);
});





let PrefObserver = {
  register: function() {
    this.branch = Services.prefs.getBranch("devtools.profiler.");
    this.branch.addObserver("", this, false);
  },
  unregister: function() {
    this.branch.removeObserver("", this);
  },
  observe: function(subject, topic, pref) {
    Prefs.refresh();
  }
};





let PerformanceController = {
  



  initialize: function() {
    this.startRecording = this.startRecording.bind(this);
    this.stopRecording = this.stopRecording.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);

    PerformanceView.on(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.on(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    gFront.on("ticks", this._onTimelineData);
  },

  


  destroy: function() {
    PerformanceView.off(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.off(EVENTS.UI_STOP_RECORDING, this.stopRecording);
  },

  



  startRecording: Task.async(function *() {
    yield gFront.startRecording();
    this.emit(EVENTS.RECORDING_STARTED);
  }),

  



  stopRecording: Task.async(function *() {
    let results = yield gFront.stopRecording();
    this.emit(EVENTS.RECORDING_STOPPED, results);
  }),

  


  _onTimelineData: function (eventName, ...data) {
    this.emit(EVENTS.TIMELINE_DATA, eventName, ...data);
  }
};




EventEmitter.decorate(PerformanceController);




const Prefs = new ViewHelpers.Prefs("devtools.profiler", {
  showPlatformData: ["Bool", "ui.show-platform-data"]
});




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
