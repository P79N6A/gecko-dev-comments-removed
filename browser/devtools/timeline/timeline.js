


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");

devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

devtools.lazyRequireGetter(this, "MarkersOverview",
  "devtools/timeline/markers-overview", true);
devtools.lazyRequireGetter(this, "MemoryOverview",
  "devtools/timeline/memory-overview", true);
devtools.lazyRequireGetter(this, "Waterfall",
  "devtools/timeline/waterfall", true);

devtools.lazyImporter(this, "CanvasGraphUtils",
  "resource:///modules/devtools/Graphs.jsm");

devtools.lazyImporter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

const OVERVIEW_UPDATE_INTERVAL = 200;
const OVERVIEW_INITIAL_SELECTION_RATIO = 0.15;


const EVENTS = {
  
  RECORDING_STARTED: "Timeline:RecordingStarted",
  RECORDING_ENDED: "Timeline:RecordingEnded",

  
  OVERVIEW_UPDATED: "Timeline:OverviewUpdated",

  
  WATERFALL_UPDATED: "Timeline:WaterfallUpdated"
};




let gToolbox, gTarget, gFront;




let startupTimeline = Task.async(function*() {
  yield TimelineView.initialize();
  yield TimelineController.initialize();
});




let shutdownTimeline = Task.async(function*() {
  yield TimelineView.destroy();
  yield TimelineController.destroy();
  yield gFront.stop();
});




let TimelineController = {
  



  _starTime: 0,
  _endTime: 0,
  _markers: [],
  _memory: [],

  


  initialize: function() {
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onMarkers = this._onMarkers.bind(this);
    this._onMemory = this._onMemory.bind(this);
    gFront.on("markers", this._onMarkers);
    gFront.on("memory", this._onMemory);
  },

  


  destroy: function() {
    gFront.off("markers", this._onMarkers);
    gFront.off("memory", this._onMemory);
  },

  



  getInterval: function() {
    return { startTime: this._startTime, endTime: this._endTime };
  },

  



  getMarkers: function() {
    return this._markers;
  },

  



  getMemory: function() {
    return this._memory;
  },

  


  updateMemoryRecording: Task.async(function*() {
    if ($("#memory-checkbox").checked) {
      yield TimelineView.showMemoryOverview();
    } else {
      yield TimelineView.hideMemoryOverview();
    }
  }),

  


  toggleRecording: Task.async(function*() {
    let isRecording = yield gFront.isRecording();
    if (isRecording == false) {
      yield this._startRecording();
    } else {
      yield this._stopRecording();
    }
  }),

  


  _startRecording: function*() {
    TimelineView.handleRecordingStarted();

    let withMemory = $("#memory-checkbox").checked;
    let startTime = yield gFront.start({ withMemory });

    
    
    
    
    
    this._localStartTime = performance.now();
    this._startTime = startTime;
    this._endTime = startTime;
    this._markers = [];
    this._memory = [];
    this._updateId = setInterval(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
  },

  


  _stopRecording: function*() {
    clearInterval(this._updateId);

    
    this._markers = this._markers.sort((a,b) => (a.start > b.start));

    TimelineView.handleRecordingUpdate();
    TimelineView.handleRecordingEnded();
    yield gFront.stop();
  },

  



  _stopRecordingAndDiscardData: function*() {
    yield this._stopRecording();
    this._markers.length = 0;
    this._memory.length = 0;
  },

  








  _onMarkers: function(markers, endTime) {
    Array.prototype.push.apply(this._markers, markers);
    this._endTime = endTime;
  },

  







  _onMemory: function(delta, measurement) {
    this._memory.push({ delta, value: measurement.total / 1024 / 1024 });
  },

  



  _onRecordingTick: function() {
    
    
    
    let fakeTime = this._startTime + (performance.now() - this._localStartTime);
    if (fakeTime > this._endTime) {
      this._endTime = fakeTime;
    }
    TimelineView.handleRecordingUpdate();
  }
};




let TimelineView = {
  


  initialize: Task.async(function*() {
    this.markersOverview = new MarkersOverview($("#markers-overview"));
    this.waterfall = new Waterfall($("#timeline-waterfall"));

    this._onSelecting = this._onSelecting.bind(this);
    this._onRefresh = this._onRefresh.bind(this);
    this.markersOverview.on("selecting", this._onSelecting);
    this.markersOverview.on("refresh", this._onRefresh);

    yield this.markersOverview.ready();
    yield this.waterfall.recalculateBounds();
  }),

  


  destroy: function() {
    this.markersOverview.off("selecting", this._onSelecting);
    this.markersOverview.off("refresh", this._onRefresh);
    this.markersOverview.destroy();

    
    if (this.memoryOverview) {
      this.memoryOverview.destroy();
    }
  },

  


  showMemoryOverview: Task.async(function*() {
    this.memoryOverview = new MemoryOverview($("#memory-overview"));
    yield this.memoryOverview.ready();

    let interval = TimelineController.getInterval();
    let memory = TimelineController.getMemory();
    this.memoryOverview.setData({ interval, memory });

    CanvasGraphUtils.linkAnimation(this.markersOverview, this.memoryOverview);
    CanvasGraphUtils.linkSelection(this.markersOverview, this.memoryOverview);
  }),

  


  hideMemoryOverview: function() {
    if (!this.memoryOverview) {
      return;
    }
    this.memoryOverview.destroy();
    this.memoryOverview = null;
  },

  



  handleRecordingStarted: function() {
    $("#record-button").setAttribute("checked", "true");
    $("#memory-checkbox").setAttribute("disabled", "true");
    $("#timeline-pane").selectedPanel = $("#recording-notice");

    this.markersOverview.clearView();

    
    if (this.memoryOverview) {
      this.memoryOverview.clearView();
    }

    this.waterfall.clearView();

    window.emit(EVENTS.RECORDING_STARTED);
  },

  



  handleRecordingEnded: function() {
    $("#record-button").removeAttribute("checked");
    $("#memory-checkbox").removeAttribute("disabled");
    $("#timeline-pane").selectedPanel = $("#timeline-waterfall");

    this.markersOverview.selectionEnabled = true;

    
    if (this.memoryOverview) {
      this.memoryOverview.selectionEnabled = true;
    }

    let interval = TimelineController.getInterval();
    let markers = TimelineController.getMarkers();
    let memory = TimelineController.getMemory();

    if (markers.length) {
      let start = (markers[0].start - interval.startTime) * this.markersOverview.dataScaleX;
      let end = start + this.markersOverview.width * OVERVIEW_INITIAL_SELECTION_RATIO;
      this.markersOverview.setSelection({ start, end });
    } else {
      let timeStart = interval.startTime;
      let timeEnd = interval.endTime;
      this.waterfall.setData(markers, timeStart, timeStart, timeEnd);
    }

    window.emit(EVENTS.RECORDING_ENDED);
  },

  



  handleRecordingUpdate: function() {
    let interval = TimelineController.getInterval();
    let markers = TimelineController.getMarkers();
    let memory = TimelineController.getMemory();

    this.markersOverview.setData({ interval, markers });

    
    if (this.memoryOverview) {
      this.memoryOverview.setData({ interval, memory });
    }

    window.emit(EVENTS.OVERVIEW_UPDATED);
  },

  


  _onSelecting: function() {
    if (!this.markersOverview.hasSelection() &&
        !this.markersOverview.hasSelectionInProgress()) {
      this.waterfall.clearView();
      return;
    }
    let selection = this.markersOverview.getSelection();
    let start = selection.start / this.markersOverview.dataScaleX;
    let end = selection.end / this.markersOverview.dataScaleX;

    let markers = TimelineController.getMarkers();
    let interval = TimelineController.getInterval();

    let timeStart = interval.startTime + Math.min(start, end);
    let timeEnd = interval.startTime + Math.max(start, end);
    this.waterfall.setData(markers, interval.startTime, timeStart, timeEnd);
  },

  


  _onRefresh: function() {
    this.waterfall.recalculateBounds();
    this._onSelecting();
  }
};




EventEmitter.decorate(this);




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
