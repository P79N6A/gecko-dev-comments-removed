


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

devtools.lazyRequireGetter(this, "TreeWidget",
  "devtools/shared/widgets/TreeWidget", true);
devtools.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/shared/timeline/global", true);
devtools.lazyRequireGetter(this, "L10N",
  "devtools/shared/profiler/global", true);
devtools.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils", true);
devtools.lazyRequireGetter(this, "RecordingModel",
  "devtools/performance/recording-model", true);
devtools.lazyRequireGetter(this, "GraphsController",
  "devtools/performance/graphs", true);
devtools.lazyRequireGetter(this, "Waterfall",
  "devtools/shared/timeline/waterfall", true);
devtools.lazyRequireGetter(this, "MarkerDetails",
  "devtools/shared/timeline/marker-details", true);
devtools.lazyRequireGetter(this, "CallView",
  "devtools/shared/profiler/tree-view", true);
devtools.lazyRequireGetter(this, "ThreadNode",
  "devtools/shared/profiler/tree-model", true);
devtools.lazyRequireGetter(this, "FrameNode",
  "devtools/shared/profiler/tree-model", true);
devtools.lazyRequireGetter(this, "JITOptimizations",
  "devtools/shared/profiler/jit", true);
devtools.lazyRequireGetter(this, "OptionsView",
  "devtools/shared/options-view", true);
devtools.lazyRequireGetter(this, "FlameGraphUtils",
  "devtools/shared/widgets/FlameGraph", true);
devtools.lazyRequireGetter(this, "FlameGraph",
  "devtools/shared/widgets/FlameGraph", true);

devtools.lazyImporter(this, "SideMenuWidget",
  "resource:///modules/devtools/SideMenuWidget.jsm");
devtools.lazyImporter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

const BRANCH_NAME = "devtools.performance.ui.";


const EVENTS = {
  
  PREF_CHANGED: "Performance:PrefChanged",

  
  THEME_CHANGED: "Performance:ThemeChanged",

  
  
  
  CONSOLE_RECORDING_STARTED: "Performance:ConsoleRecordingStarted",
  CONSOLE_RECORDING_WILL_STOP: "Performance:ConsoleRecordingWillStop",
  CONSOLE_RECORDING_STOPPED: "Performance:ConsoleRecordingStopped",

  
  
  
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
    this._onConsoleProfileStart = this._onConsoleProfileStart.bind(this);
    this._onConsoleProfileEnd = this._onConsoleProfileEnd.bind(this);
    this._onConsoleProfileEnding = this._onConsoleProfileEnding.bind(this);

    
    
    
    this._nonBooleanPrefs = new ViewHelpers.Prefs("devtools.performance", {
      "hidden-markers": ["Json", "timeline.hidden-markers"],
      "memory-sample-probability": ["Float", "memory.sample-probability"],
      "memory-max-log-length": ["Int", "memory.max-log-length"],
      "profiler-buffer-size": ["Int", "profiler.buffer-size"],
      "profiler-sample-frequency": ["Int", "profiler.sample-frequency-khz"],
    });

    this._nonBooleanPrefs.registerObserver();
    this._nonBooleanPrefs.on("pref-changed", this._onPrefChanged);

    gFront.on("console-profile-start", this._onConsoleProfileStart);
    gFront.on("console-profile-ending", this._onConsoleProfileEnding);
    gFront.on("console-profile-end", this._onConsoleProfileEnd);
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
    this._nonBooleanPrefs.unregisterObserver();
    this._nonBooleanPrefs.off("pref-changed", this._onPrefChanged);

    gFront.off("console-profile-start", this._onConsoleProfileStart);
    gFront.off("console-profile-ending", this._onConsoleProfileEnding);
    gFront.off("console-profile-end", this._onConsoleProfileEnd);
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
    return this._nonBooleanPrefs[prefName];
  },

  




  setPref: function (prefName, prefValue) {
    this._nonBooleanPrefs[prefName] = prefValue;
  },

  



  startRecording: Task.async(function *() {
    let options = {
      withMarkers: true,
      withMemory: this.getOption("enable-memory"),
      withTicks: this.getOption("enable-framerate"),
      withAllocations: this.getOption("enable-memory"),
      allocationsSampleProbability: this.getPref("memory-sample-probability"),
      allocationsMaxLogLength: this.getPref("memory-max-log-length"),
      bufferSize: this.getPref("profiler-buffer-size"),
      sampleFrequency: this.getPref("profiler-sample-frequency")
    };

    this.emit(EVENTS.RECORDING_WILL_START);

    let recording = yield gFront.startRecording(options);
    this._recordings.push(recording);

    this.emit(EVENTS.RECORDING_STARTED, recording);
  }),

  



  stopRecording: Task.async(function *() {
    let recording = this.getLatestManualRecording();

    this.emit(EVENTS.RECORDING_WILL_STOP, recording);
    yield gFront.stopRecording(recording);
    this.emit(EVENTS.RECORDING_STOPPED, recording);
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
    return RecordingUtils.getFilteredBlueprint({ blueprint, hiddenMarkers });
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

  


  _onConsoleProfileStart: function (_, recording) {
    this._recordings.push(recording);
    this.emit(EVENTS.CONSOLE_RECORDING_STARTED, recording);
  },

  



  _onConsoleProfileEnding: function (_, recording) {
    this.emit(EVENTS.CONSOLE_RECORDING_WILL_STOP, recording);
  },

  



  _onConsoleProfileEnd: function (_, recording) {
    this.emit(EVENTS.CONSOLE_RECORDING_STOPPED, recording);
  },

  


  getRecordings: function () {
    return this._recordings;
  },

  

















  isFeatureSupported: function ({ features, actors, isRecording: shouldBeRecording }, recording) {
    recording = recording || this.getCurrentRecording();
    let recordingConfig = recording ? recording.getConfiguration() : {};
    let currentRecordingState = recording ? recording.isRecording() : void 0;
    let actorsSupported = gFront.getActorSupport();

    if (shouldBeRecording != null && shouldBeRecording !== currentRecordingState) {
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

  toString: () => "[object PerformanceController]"
};




EventEmitter.decorate(PerformanceController);




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
