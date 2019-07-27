


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

devtools.lazyRequireGetter(this, "L10N",
  "devtools/profiler/global", true);
devtools.lazyRequireGetter(this, "PerformanceIO",
  "devtools/performance/io", true);
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

devtools.lazyImporter(this, "CanvasGraphUtils",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "LineGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");


const EVENTS = {
  
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

  
  WATERFALL_RENDERED: "Performance:UI:WaterfallRendered"
};



const RECORDING_IN_PROGRESS = -1;
const RECORDING_UNAVAILABLE = null;




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
  



  _localStartTime: RECORDING_UNAVAILABLE,
  _startTime: RECORDING_UNAVAILABLE,
  _endTime: RECORDING_UNAVAILABLE,
  _markers: [],
  _frames: [],
  _memory: [],
  _ticks: [],
  _profilerData: {},

  



  initialize: function() {
    this.startRecording = this.startRecording.bind(this);
    this.stopRecording = this.stopRecording.bind(this);
    this.importRecording = this.importRecording.bind(this);
    this.exportRecording = this.exportRecording.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);

    PerformanceView.on(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.on(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.on(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    PerformanceView.on(EVENTS.UI_IMPORT_RECORDING, this.importRecording);

    gFront.on("ticks", this._onTimelineData); 
    gFront.on("markers", this._onTimelineData); 
    gFront.on("frames", this._onTimelineData); 
    gFront.on("memory", this._onTimelineData); 
  },

  


  destroy: function() {
    PerformanceView.off(EVENTS.UI_START_RECORDING, this.startRecording);
    PerformanceView.off(EVENTS.UI_STOP_RECORDING, this.stopRecording);
    PerformanceView.off(EVENTS.UI_EXPORT_RECORDING, this.exportRecording);
    PerformanceView.off(EVENTS.UI_IMPORT_RECORDING, this.importRecording);

    gFront.off("ticks", this._onTimelineData);
    gFront.off("markers", this._onTimelineData);
    gFront.off("frames", this._onTimelineData);
    gFront.off("memory", this._onTimelineData);
  },

  



  startRecording: Task.async(function *() {
    
    
    
    
    this._localStartTime = performance.now();

    let { startTime } = yield gFront.startRecording({
      withTicks: true,
      withMemory: true
    });

    this._startTime = startTime;
    this._endTime = RECORDING_IN_PROGRESS;
    this._markers = [];
    this._frames = [];
    this._memory = [];
    this._ticks = [];

    this.emit(EVENTS.RECORDING_STARTED);
  }),

  



  stopRecording: Task.async(function *() {
    let results = yield gFront.stopRecording();

    
    if (!results.endTime) {
      results.endTime = this._startTime + this.getLocalElapsedTime();
    }

    this._endTime = results.endTime;
    this._profilerData = results.profilerData;
    this._markers = this._markers.sort((a,b) => (a.start > b.start));

    this.emit(EVENTS.RECORDING_STOPPED);
  }),

  





  exportRecording: Task.async(function*(_, file) {
    let recordingData = this.getAllData();
    yield PerformanceIO.saveRecordingToFile(recordingData, file);

    this.emit(EVENTS.RECORDING_EXPORTED, recordingData);
  }),

  






  importRecording: Task.async(function*(_, file) {
    let recordingData = yield PerformanceIO.loadRecordingFromFile(file);

    this._startTime = recordingData.interval.startTime;
    this._endTime = recordingData.interval.endTime;
    this._markers = recordingData.markers;
    this._frames = recordingData.frames;
    this._memory = recordingData.memory;
    this._ticks = recordingData.ticks;
    this._profilerData = recordingData.profilerData;

    this.emit(EVENTS.RECORDING_IMPORTED, recordingData);

    
    this.emit(EVENTS.RECORDING_STARTED);
    this.emit(EVENTS.RECORDING_STOPPED);
  }),

  


  getLocalElapsedTime: function() {
    return performance.now() - this._localStartTime;
  },

  



  getInterval: function() {
    let startTime = this._startTime;
    let endTime = this._endTime;

    
    
    
    if (endTime == RECORDING_IN_PROGRESS) {
      endTime = startTime + this.getLocalElapsedTime();
    }

    return { startTime, endTime };
  },

  



  getMarkers: function() {
    return this._markers;
  },

  



  getFrames: function() {
    return this._frames;
  },

  



  getMemory: function() {
    return this._memory;
  },

  



  getTicks: function() {
    return this._ticks;
  },

  



  getProfilerData: function() {
    return this._profilerData;
  },

  


  getAllData: function() {
    let interval = this.getInterval();
    let markers = this.getMarkers();
    let frames = this.getFrames();
    let memory = this.getMemory();
    let ticks = this.getTicks();
    let profilerData = this.getProfilerData();
    return { interval, markers, frames, memory, ticks, profilerData };
  },

  


  _onTimelineData: function (eventName, ...data) {
    
    if (eventName == "markers") {
      let [markers] = data;
      Array.prototype.push.apply(this._markers, markers);
    }
    
    else if (eventName == "frames") {
      let [delta, frames] = data;
      Array.prototype.push.apply(this._frames, frames);
    }
    
    else if (eventName == "memory") {
      let [delta, measurement] = data;
      this._memory.push({ delta, value: measurement.total / 1024 / 1024 });
    }
    
    else if (eventName == "ticks") {
      let [delta, timestamps] = data;
      this._ticks = timestamps;
    }

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
