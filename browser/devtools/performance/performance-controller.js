


"use strict";

const { Task } = require("resource://gre/modules/Task.jsm");
const { Heritage, ViewHelpers, WidgetMethods } = require("resource:///modules/devtools/ViewHelpers.jsm");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");



loader.lazyRequireGetter(this, "L10N",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/performance/markers", true);
loader.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils");
loader.lazyRequireGetter(this, "RecordingModel",
  "devtools/performance/recording-model", true);
loader.lazyRequireGetter(this, "GraphsController",
  "devtools/performance/graphs", true);
loader.lazyRequireGetter(this, "WaterfallHeader",
  "devtools/performance/waterfall-ticks", true);
loader.lazyRequireGetter(this, "MarkerView",
  "devtools/performance/marker-view", true);
loader.lazyRequireGetter(this, "MarkerDetails",
  "devtools/performance/marker-details", true);
loader.lazyRequireGetter(this, "MarkerUtils",
  "devtools/performance/marker-utils");
loader.lazyRequireGetter(this, "WaterfallUtils",
  "devtools/performance/waterfall-utils");
loader.lazyRequireGetter(this, "CallView",
  "devtools/performance/tree-view", true);
loader.lazyRequireGetter(this, "ThreadNode",
  "devtools/performance/tree-model", true);
loader.lazyRequireGetter(this, "FrameNode",
  "devtools/performance/tree-model", true);
loader.lazyRequireGetter(this, "JITOptimizations",
  "devtools/performance/jit", true);



loader.lazyRequireGetter(this, "OptionsView",
  "devtools/shared/options-view", true);
loader.lazyRequireGetter(this, "FlameGraphUtils",
  "devtools/shared/widgets/FlameGraph", true);
loader.lazyRequireGetter(this, "FlameGraph",
  "devtools/shared/widgets/FlameGraph", true);
loader.lazyRequireGetter(this, "TreeWidget",
  "devtools/shared/widgets/TreeWidget", true);

loader.lazyImporter(this, "SideMenuWidget",
  "resource:///modules/devtools/SideMenuWidget.jsm");
loader.lazyImporter(this, "setNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyImporter(this, "clearNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyImporter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

const BRANCH_NAME = "devtools.performance.ui.";


const EVENTS = {
  
  PREF_CHANGED: "Performance:PrefChanged",

  
  THEME_CHANGED: "Performance:ThemeChanged",

  
  
  
  UI_STATE_CHANGED: "Performance:UI:StateChanged",

  
  UI_CLEAR_RECORDINGS: "Performance:UI:ClearRecordings",

  
  UI_START_RECORDING: "Performance:UI:StartRecording",
  UI_STOP_RECORDING: "Performance:UI:StopRecording",

  
  UI_IMPORT_RECORDING: "Performance:UI:ImportRecording",
  
  UI_EXPORT_RECORDING: "Performance:UI:ExportRecording",

  
  RECORDING_STARTED: "Performance:RecordingStarted",
  RECORDING_STOPPED: "Performance:RecordingStopped",
  RECORDING_WILL_START: "Performance:RecordingWillStart",
  RECORDING_WILL_STOP: "Performance:RecordingWillStop",

  
  
  RECORDING_SELECTED: "Performance:RecordingSelected",

  
  RECORDINGS_CLEARED: "Performance:RecordingsCleared",

  
  RECORDING_IMPORTED: "Performance:RecordingImported",
  RECORDING_EXPORTED: "Performance:RecordingExported",

  
  PROFILER_STATUS_UPDATED: "Performance:BufferUpdated",

  
  UI_BUFFER_STATUS_UPDATED: "Performance:UI:BufferUpdated",

  
  
  OPTIMIZATIONS_RESET: "Performance:UI:OptimizationsReset",
  OPTIMIZATIONS_RENDERED: "Performance:UI:OptimizationsRendered",

  
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
    this.clearRecordings = this.clearRecordings.bind(this);
    this._onRecordingSelectFromView = this._onRecordingSelectFromView.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);
    this._onThemeChanged = this._onThemeChanged.bind(this);
    this._onRecordingStateChange = this._onRecordingStateChange.bind(this);
    this._onProfilerStatusUpdated = this._onProfilerStatusUpdated.bind(this);

    
    this._e10s = Services.appinfo.browserTabsRemoteAutostart;
    this._setMultiprocessAttributes();

    this._prefs = require("devtools/performance/global").PREFS;
    this._prefs.on("pref-changed", this._onPrefChanged);

    gFront.on("recording-starting", this._onRecordingStateChange);
    gFront.on("recording-started", this._onRecordingStateChange);
    gFront.on("recording-stopping", this._onRecordingStateChange);
    gFront.on("recording-stopped", this._onRecordingStateChange);
    gFront.on("profiler-status", this._onProfilerStatusUpdated);
    ToolbarView.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceView.on(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.on(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.on(EVENTS.UI_IMPORT_RECORDING, this.importRecording);
    PerformanceView.on(EVENTS.UI_CLEAR_RECORDINGS, this.clearRecordings);
    RecordingsView.on(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    RecordingsView.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelectFromView);

    gDevTools.on("pref-changed", this._onThemeChanged);
  }),

  


  destroy: function() {
    this._prefs.off("pref-changed", this._onPrefChanged);

    gFront.off("recording-starting", this._onRecordingStateChange);
    gFront.off("recording-started", this._onRecordingStateChange);
    gFront.off("recording-stopping", this._onRecordingStateChange);
    gFront.off("recording-stopped", this._onRecordingStateChange);
    gFront.off("profiler-status", this._onProfilerStatusUpdated);
    ToolbarView.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceView.off(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.off(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.off(EVENTS.UI_IMPORT_RECORDING, this.importRecording);
    PerformanceView.off(EVENTS.UI_CLEAR_RECORDINGS, this.clearRecordings);
    RecordingsView.off(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    RecordingsView.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelectFromView);

    gDevTools.off("pref-changed", this._onThemeChanged);
  },

  


  getTheme: function () {
    return Services.prefs.getCharPref("devtools.theme");
  },

  







  getOption: function (prefName) {
    return ToolbarView.optionsView.getPref(prefName);
  },

  






  getPref: function (prefName) {
    return this._prefs[prefName];
  },

  






  setPref: function (prefName, prefValue) {
    this._prefs[prefName] = prefValue;
  },

  



  startRecording: Task.async(function *() {
    let options = {
      withMarkers: true,
      withMemory: this.getOption("enable-memory"),
      withTicks: this.getOption("enable-framerate"),
      withAllocations: this.getOption("enable-allocations"),
      allocationsSampleProbability: this.getPref("memory-sample-probability"),
      allocationsMaxLogLength: this.getPref("memory-max-log-length"),
      bufferSize: this.getPref("profiler-buffer-size"),
      sampleFrequency: this.getPref("profiler-sample-frequency")
    };

    yield gFront.startRecording(options);
  }),

  



  stopRecording: Task.async(function *() {
    let recording = this.getLatestManualRecording();
    yield gFront.stopRecording(recording);
  }),

  








  exportRecording: Task.async(function*(_, recording, file) {
    yield recording.exportRecording(file);
    this.emit(EVENTS.RECORDING_EXPORTED, recording);
  }),

  



  clearRecordings: Task.async(function* () {
    let latest = this.getLatestManualRecording();
    if (latest && latest.isRecording()) {
      yield this.stopRecording();
    }
    
    
    if (latest && !latest.isCompleted()) {
      yield this.once(EVENTS.RECORDING_STOPPED);
    }

    this._recordings.length = 0;
    this.setCurrentRecording(null);
    this.emit(EVENTS.RECORDINGS_CLEARED);
  }),

  






  importRecording: Task.async(function*(_, file) {
    let recording = new RecordingModel();
    this._recordings.push(recording);
    yield recording.importRecording(file);

    this.emit(EVENTS.RECORDING_IMPORTED, recording);
  }),

  





  setCurrentRecording: function (recording) {
    if (this._currentRecording !== recording) {
      this._currentRecording = recording;
      this.emit(EVENTS.RECORDING_SELECTED, recording);
    }
  },

  



  getCurrentRecording: function () {
    return this._currentRecording;
  },

  



  getLatestManualRecording: function () {
    for (let i = this._recordings.length - 1; i >= 0; i--) {
      let model = this._recordings[i];
      if (!model.isConsole() && !model.isImported()) {
        return this._recordings[i];
      }
    }
    return null;
  },

  



  getTimelineBlueprint: function() {
    let blueprint = TIMELINE_BLUEPRINT;
    let hiddenMarkers = this.getPref("hidden-markers");
    return MarkerUtils.getFilteredBlueprint({ blueprint, hiddenMarkers });
  },

  



  _onRecordingSelectFromView: function (_, recording) {
    this.setCurrentRecording(recording);
  },

  



  _onPrefChanged: function (_, prefName, prefValue) {
    this.emit(EVENTS.PREF_CHANGED, prefName, prefValue);
  },

  


  _onThemeChanged: function (_, data) {
    
    
    if (data.pref !== "devtools.theme") {
      return;
    }

    this.emit(EVENTS.THEME_CHANGED, data.newValue);
  },

  


  _onProfilerStatusUpdated: function (_, data) {
    this.emit(EVENTS.PROFILER_STATUS_UPDATED, data);
  },

  





  _onRecordingStateChange: function (state, model) {
    
    
    if (state !== "recording-starting" && this.getRecordings().indexOf(model) === -1) {
      return;
    }
    switch (state) {
      
      case "recording-starting":
        
        this._recordings.push(model);
        this.emit(EVENTS.RECORDING_WILL_START, model);
        break;
      
      case "recording-started":
        this.emit(EVENTS.RECORDING_STARTED, model);
        break;
      
      
      case "recording-stopping":
        this.emit(EVENTS.RECORDING_WILL_STOP, model);
        break;
      
      case "recording-stopped":
        this.emit(EVENTS.RECORDING_STOPPED, model);
        break;
    }
  },

  


  getRecordings: function () {
    return this._recordings;
  },

  

















  isFeatureSupported: function ({ features, actors, mustBeCompleted }, recording) {
    recording = recording || this.getCurrentRecording();
    let recordingConfig = recording ? recording.getConfiguration() : {};
    let currentCompletedState = recording ? recording.isCompleted() : void 0;
    let actorsSupported = gFront.getActorSupport();

    if (mustBeCompleted != null && mustBeCompleted !== currentCompletedState) {
      return false;
    }
    if (actors && !actors.every(a => actorsSupported[a])) {
      return false;
    }
    if (features && !features.every(f => recordingConfig[f])) {
      return false;
    }
    return true;
  },

  






  getMultiprocessStatus: function () {
    
    
    
    
    if (gDevTools.testing) {
      return { supported: true, enabled: true };
    }
    let supported = SYSTEM.MULTIPROCESS_SUPPORTED;
    
    
    let enabled = this._e10s;
    return { supported, enabled };
  },

  




  _setMultiprocessAttributes: function () {
    let { enabled, supported } = this.getMultiprocessStatus();
    if (!enabled && supported) {
      $("#performance-view").setAttribute("e10s", "disabled");
    }
    
    else if (!enabled && !supported) {
      $("#performance-view").setAttribute("e10s", "unsupported");
    }
  },

  toString: () => "[object PerformanceController]"
};




EventEmitter.decorate(PerformanceController);




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
