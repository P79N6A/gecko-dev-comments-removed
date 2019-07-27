


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");

devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

devtools.lazyRequireGetter(this, "Overview",
  "devtools/timeline/overview", true);
devtools.lazyRequireGetter(this, "Waterfall",
  "devtools/timeline/waterfall", true);

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
  


  _markers: [],

  


  initialize: function() {
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onMarkers = this._onMarkers.bind(this);
    gFront.on("markers", this._onMarkers);
  },

  


  destroy: function() {
    gFront.off("markers", this._onMarkers);
  },

  



  getMarkers: function() {
    return this._markers;
  },

  


  toggleRecording: Task.async(function*() {
    let isRecording = yield gFront.isRecording();
    if (isRecording == false) {
      yield this._startRecording();
    } else {
      yield this._stopRecording();
    }
  }),

  


  _startRecording: function*() {
    this._markers = [];
    this._markers.startTime = performance.now();
    this._markers.endTime = performance.now();
    this._updateId = setInterval(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);

    TimelineView.handleRecordingStarted();
    yield gFront.start();
  },

  


  _stopRecording: function*() {
    clearInterval(this._updateId);

    TimelineView.handleMarkersUpdate(this._markers);
    TimelineView.handleRecordingEnded();
    yield gFront.stop();
  },

  



  _stopRecordingAndDiscardData: function*() {
    this._markers.length = 0;
    yield this._stopRecording();
  },

  






  _onMarkers: function(markers) {
    Array.prototype.push.apply(this._markers, markers);
  },

  



  _onRecordingTick: function() {
    this._markers.endTime = performance.now();
    TimelineView.handleMarkersUpdate(this._markers);
  }
};




let TimelineView = {
  


  initialize: Task.async(function*() {
    this.overview = new Overview($("#timeline-overview"));
    this.waterfall = new Waterfall($("#timeline-waterfall"));

    this._onSelecting = this._onSelecting.bind(this);
    this._onRefresh = this._onRefresh.bind(this);
    this.overview.on("selecting", this._onSelecting);
    this.overview.on("refresh", this._onRefresh);

    yield this.overview.ready();
    yield this.waterfall.recalculateBounds();
  }),

  


  destroy: function() {
    this.overview.off("selecting", this._onSelecting);
    this.overview.off("refresh", this._onRefresh);
    this.overview.destroy();
  },

  



  handleRecordingStarted: function() {
    $("#record-button").setAttribute("checked", "true");
    $("#timeline-pane").selectedPanel = $("#recording-notice");

    this.overview.selectionEnabled = false;
    this.overview.dropSelection();
    this.overview.setData([]);
    this.waterfall.clearView();

    window.emit(EVENTS.RECORDING_STARTED);
  },

  



  handleRecordingEnded: function() {
    $("#record-button").removeAttribute("checked");
    $("#timeline-pane").selectedPanel = $("#timeline-waterfall");

    this.overview.selectionEnabled = true;

    let markers = TimelineController.getMarkers();
    if (markers.length) {
      let start = markers[0].start * this.overview.dataScaleX;
      let end = start + this.overview.width * OVERVIEW_INITIAL_SELECTION_RATIO;
      this.overview.setSelection({ start, end });
    } else {
      let duration = markers.endTime - markers.startTime;
      this.waterfall.setData(markers, 0, duration);
    }

    window.emit(EVENTS.RECORDING_ENDED);
  },

  






  handleMarkersUpdate: function(markers) {
    this.overview.setData(markers);
    window.emit(EVENTS.OVERVIEW_UPDATED);
  },

  


  _onSelecting: function() {
    if (!this.overview.hasSelection() &&
        !this.overview.hasSelectionInProgress()) {
      this.waterfall.clearView();
      return;
    }
    let selection = this.overview.getSelection();
    let start = selection.start / this.overview.dataScaleX;
    let end = selection.end / this.overview.dataScaleX;

    let markers = TimelineController.getMarkers();
    let timeStart = Math.min(start, end);
    let timeEnd = Math.max(start, end);
    this.waterfall.setData(markers, timeStart, timeEnd);
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
