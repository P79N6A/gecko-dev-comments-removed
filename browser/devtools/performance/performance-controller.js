


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
devtools.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils", true);
devtools.lazyRequireGetter(this, "RecordingModel",
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
devtools.lazyRequireGetter(this, "OptionsView",
  "devtools/shared/options-view", true);

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

const BRANCH_NAME = "devtools.performance.ui.";


const EVENTS = {
  
  PREF_CHANGED: "Preformance:PrefChanged",

  
  
  RECORDING_SELECTED: "Performance:RecordingSelected",

  
  UI_START_RECORDING: "Performance:UI:StartRecording",
  UI_STOP_RECORDING: "Performance:UI:StopRecording",

  
  UI_IMPORT_RECORDING: "Performance:UI:ImportRecording",
  
  UI_EXPORT_RECORDING: "Performance:UI:ExportRecording",

  
  RECORDING_STARTED: "Performance:RecordingStarted",
  RECORDING_STOPPED: "Performance:RecordingStopped",
  RECORDING_WILL_START: "Performance:RecordingWillStart",
  RECORDING_WILL_STOP: "Performance:RecordingWillStop",

  
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

  
  WATERFALL_RENDERED: "Performance:UI:WaterfallRendered",

  
  JS_CALL_TREE_RENDERED: "Performance:UI:JsCallTreeRendered",

  
  JS_FLAMEGRAPH_RENDERED: "Performance:UI:JsFlameGraphRendered",

  
  MEMORY_CALL_TREE_RENDERED: "Performance:UI:MemoryCallTreeRendered",

  
  MEMORY_FLAMEGRAPH_RENDERED: "Performance:UI:MemoryFlameGraphRendered",

  
  SOURCE_SHOWN_IN_JS_DEBUGGER: "Performance:UI:SourceShownInJsDebugger",
  SOURCE_NOT_FOUND_IN_JS_DEBUGGER: "Performance:UI:SourceNotFoundInJsDebugger"
};




let gToolbox, gTarget, gFront;




let startupPerformance = Task.async(function*() {
  yield promise.all([
    PerformanceController.initialize(),
    PerformanceView.initialize()
  ]);
});




let shutdownPerformance = Task.async(function*() {
  yield promise.all([
    PerformanceController.destroy(),
    PerformanceView.destroy()
  ]);
});





let PerformanceController = {
  _recordings: [],
  _currentRecording: null,

  



  initialize: Task.async(function* () {
    this.startRecording = this.startRecording.bind(this);
    this.stopRecording = this.stopRecording.bind(this);
    this.importRecording = this.importRecording.bind(this);
    this.exportRecording = this.exportRecording.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);
    this._onRecordingSelectFromView = this._onRecordingSelectFromView.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);

    ToolbarView.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceView.on(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.on(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.on(EVENTS.UI_IMPORT_RECORDING, this.importRecording);
    RecordingsView.on(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    RecordingsView.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelectFromView);

    gFront.on("markers", this._onTimelineData); 
    gFront.on("frames", this._onTimelineData); 
    gFront.on("memory", this._onTimelineData); 
    gFront.on("ticks", this._onTimelineData); 
    gFront.on("allocations", this._onTimelineData); 
  }),

  


  destroy: function() {
    ToolbarView.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceView.off(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.off(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.off(EVENTS.UI_IMPORT_RECORDING, this.importRecording);
    RecordingsView.off(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    RecordingsView.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelectFromView);

    gFront.off("markers", this._onTimelineData);
    gFront.off("frames", this._onTimelineData);
    gFront.off("memory", this._onTimelineData);
    gFront.off("ticks", this._onTimelineData);
    gFront.off("allocations", this._onTimelineData);
  },

  



  getPref: function (prefName) {
    return ToolbarView.optionsView.getPref(prefName);
  },

  



  startRecording: Task.async(function *() {
    let recording = this._createRecording();

    let withMemory = this.getPref("enable-memory");
    let withTicks = this.getPref("enable-framerate");
    let withAllocations = true;

    this.emit(EVENTS.RECORDING_WILL_START, recording);
    yield recording.startRecording({ withTicks, withMemory, withAllocations });
    this.emit(EVENTS.RECORDING_STARTED, recording, { withTicks, withMemory, withAllocations });

    this.setCurrentRecording(recording);
  }),

  



  stopRecording: Task.async(function *() {
    let recording = this._getLatestRecording();

    this.emit(EVENTS.RECORDING_WILL_STOP, recording);
    yield recording.stopRecording({
      withAllocations: true
    });
    this.emit(EVENTS.RECORDING_STOPPED, recording);
  }),

  








  exportRecording: Task.async(function*(_, recording, file) {
    yield recording.exportRecording(file);
    this.emit(EVENTS.RECORDING_EXPORTED, recording);
  }),

  






  importRecording: Task.async(function*(_, file) {
    let recording = this._createRecording();
    yield recording.importRecording(file);

    this.emit(EVENTS.RECORDING_IMPORTED, recording);
  }),

  






  _createRecording: function () {
    let recording = new RecordingModel({ front: gFront, performance });
    this._recordings.push(recording);

    this.emit(EVENTS.RECORDING_CREATED, recording);
    return recording;
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

  



  _getLatestRecording: function () {
    for (let i = this._recordings.length - 1; i >= 0; i--) {
      return this._recordings[i];
    }
    return null;
  },

  


  _onTimelineData: function (...data) {
    this._recordings.forEach(e => e.addTimelineData.apply(e, data));
    this.emit(EVENTS.TIMELINE_DATA, ...data);
  },

  



  _onRecordingSelectFromView: function (_, recording) {
    this.setCurrentRecording(recording);
  },

  



  _onPrefChanged: function (_, prefName, value) {
    this.emit(EVENTS.PREF_CHANGED, prefName, value);
  }
};




EventEmitter.decorate(PerformanceController);




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
