


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

devtools.lazyRequireGetter(this, "Services");
devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
devtools.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");

devtools.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/timeline/global", true);
devtools.lazyRequireGetter(this, "L10N",
  "devtools/profiler/global", true);
devtools.lazyRequireGetter(this, "PerformanceIO",
  "devtools/performance/io", true);
devtools.lazyRequireGetter(this, "RecordingModel",
  "devtools/performance/recording-model", true);
devtools.lazyRequireGetter(this, "RECORDING_IN_PROGRESS",
  "devtools/performance/recording-model", true);
devtools.lazyRequireGetter(this, "RECORDING_UNAVAILABLE",
  "devtools/performance/recording-model", true);

devtools.lazyRequireGetter(this, "MarkersOverview",
  "devtools/timeline/markers-overview", true);
devtools.lazyRequireGetter(this, "MemoryOverview",
  "devtools/timeline/memory-overview", true);
devtools.lazyRequireGetter(this, "Waterfall",
  "devtools/timeline/waterfall", true);
devtools.lazyRequireGetter(this, "MarkerDetails",
  "devtools/timeline/marker-details", true);
devtools.lazyRequireGetter(this, "CallView",
  "devtools/profiler/tree-view", true);
devtools.lazyRequireGetter(this, "ThreadNode",
  "devtools/profiler/tree-model", true);
devtools.lazyRequireGetter(this, "FrameNode",
  "devtools/profiler/tree-model", true);

devtools.lazyImporter(this, "CanvasGraphUtils",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "LineGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "FlameGraphUtils",
  "resource:///modules/devtools/FlameGraph.jsm");
devtools.lazyImporter(this, "FlameGraph",
  "resource:///modules/devtools/FlameGraph.jsm");
devtools.lazyImporter(this, "SideMenuWidget",
  "resource:///modules/devtools/SideMenuWidget.jsm");


const EVENTS = {
  
  
  RECORDING_SELECTED: "Performance:RecordingSelected",

  
  UI_START_RECORDING: "Performance:UI:StartRecording",
  UI_STOP_RECORDING: "Performance:UI:StopRecording",

  
  UI_IMPORT_RECORDING: "Performance:UI:ImportRecording",
  
  UI_EXPORT_RECORDING: "Performance:UI:ExportRecording",

  
  RECORDING_STARTED: "Performance:RecordingStarted",
  RECORDING_STOPPED: "Performance:RecordingStopped",

  
  RECORDING_IMPORTED: "Performance:RecordingImported",
  RECORDING_EXPORTED: "Performance:RecordingExported",

  
  TIMELINE_DATA: "Performance:TimelineData",

  
  OVERVIEW_RENDERED: "Performance:UI:OverviewRendered",
  FRAMERATE_GRAPH_RENDERED: "Performance:UI:OverviewFramerateRendered",
  MARKERS_GRAPH_RENDERED: "Performance:UI:OverviewMarkersRendered",
  MEMORY_GRAPH_RENDERED: "Performance:UI:OverviewMemoryRendered",

  
  OVERVIEW_RANGE_SELECTED: "Performance:UI:OverviewRangeSelected",
  
  OVERVIEW_RANGE_CLEARED: "Performance:UI:OverviewRangeCleared",

  
  DETAILS_VIEW_SELECTED: "Performance:UI:DetailsViewSelected",

  
  CALL_TREE_RENDERED: "Performance:UI:CallTreeRendered",

  
  SOURCE_SHOWN_IN_JS_DEBUGGER: "Performance:UI:SourceShownInJsDebugger",
  SOURCE_NOT_FOUND_IN_JS_DEBUGGER: "Performance:UI:SourceNotFoundInJsDebugger",

  
  WATERFALL_RENDERED: "Performance:UI:WaterfallRendered",

  
  FLAMEGRAPH_RENDERED: "Performance:UI:FlameGraphRendered"
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
  _recordings: [],
  _currentRecording: null,

  



  initialize: function() {
    this.startRecording = this.startRecording.bind(this);
    this.stopRecording = this.stopRecording.bind(this);
    this.importRecording = this.importRecording.bind(this);
    this.exportRecording = this.exportRecording.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);
    this._onRecordingSelectFromView = this._onRecordingSelectFromView.bind(this);

    PerformanceView.on(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.on(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.on(EVENTS.UI_IMPORT_RECORDING, this.importRecording);
    RecordingsView.on(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    RecordingsView.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelectFromView);

    gFront.on("ticks", this._onTimelineData); 
    gFront.on("markers", this._onTimelineData); 
    gFront.on("frames", this._onTimelineData); 
    gFront.on("memory", this._onTimelineData); 
  },

  


  destroy: function() {
    PerformanceView.off(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.off(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.off(EVENTS.UI_IMPORT_RECORDING, this.importRecording);
    RecordingsView.off(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    RecordingsView.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelectFromView);

    gFront.off("ticks", this._onTimelineData);
    gFront.off("markers", this._onTimelineData);
    gFront.off("frames", this._onTimelineData);
    gFront.off("memory", this._onTimelineData);
  },

  



  startRecording: Task.async(function *() {
    let model = this.createNewRecording();
    this.setCurrentRecording(model);
    yield model.startRecording();

    this.emit(EVENTS.RECORDING_STARTED, model);
  }),

  



  stopRecording: Task.async(function *() {
    let recording = this._getLatest();
    yield recording.stopRecording();

    this.emit(EVENTS.RECORDING_STOPPED, recording);
  }),

  







  exportRecording: Task.async(function*(_, recording, file) {
    let recordingData = recording.getAllData();
    yield PerformanceIO.saveRecordingToFile(recordingData, file);

    this.emit(EVENTS.RECORDING_EXPORTED, recordingData);
  }),

  





  importRecording: Task.async(function*(_, file) {
    let model = this.createNewRecording();
    yield model.importRecording(file);

    this.emit(EVENTS.RECORDING_IMPORTED, model.getAllData(), model);
  }),

  



  createNewRecording: function () {
    let model = new RecordingModel({
      front: gFront,
      performance: performance
    });
    this._recordings.push(model);
    this.emit(EVENTS.RECORDING_CREATED, model);
    return model;
  },

  


  setCurrentRecording: function (recording) {
    if (this._currentRecording !== recording) {
      this._currentRecording = recording;
      this.emit(EVENTS.RECORDING_SELECTED, recording);
    }
  },

  


  getCurrentRecording: function () {
    return this._currentRecording;
  },

  


  getLocalElapsedTime: function () {
    return this.getCurrentRecording().getLocalElapsedTime;
  },

  



  getInterval: function() {
    return this.getCurrentRecording().getInterval();
  },

  



  getMarkers: function() {
    return this.getCurrentRecording().getMarkers();
  },

  



  getFrames: function() {
    return this.getCurrentRecording().getFrames();
  },

  



  getMemory: function() {
    return this.getCurrentRecording().getMemory();
  },

  



  getTicks: function() {
    return this.getCurrentRecording().getTicks();
  },

  



  getProfilerData: function() {
    return this.getCurrentRecording().getProfilerData();
  },

  


  getAllData: function() {
    return this.getCurrentRecording().getAllData();
  },

  



  _getLatest: function () {
    for (let i = this._recordings.length - 1; i >= 0; i--) {
      return this._recordings[i];
    }
    return null;
  },

  


  _onTimelineData: function (...data) {
    this._recordings.forEach(profile => profile.addTimelineData.apply(profile, data));
    this.emit(EVENTS.TIMELINE_DATA, ...data);
  },

  



  _onRecordingSelectFromView: function (_, recording) {
    this.setCurrentRecording(recording);
  }
};




EventEmitter.decorate(PerformanceController);




const Prefs = new ViewHelpers.Prefs("devtools.profiler", {
  flattenTreeRecursion: ["Bool", "ui.flatten-tree-recursion"],
  showPlatformData: ["Bool", "ui.show-platform-data"],
});




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
