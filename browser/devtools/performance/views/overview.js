


"use strict";




const OVERVIEW_UPDATE_INTERVAL = 200; 

const FRAMERATE_GRAPH_LOW_RES_INTERVAL = 100; 
const FRAMERATE_GRAPH_HIGH_RES_INTERVAL = 16; 

const FRAMERATE_GRAPH_HEIGHT = 40; 
const MARKERS_GRAPH_HEADER_HEIGHT = 14; 
const MARKERS_GRAPH_ROW_HEIGHT = 10; 
const MARKERS_GROUP_VERTICAL_PADDING = 4; 
const MEMORY_GRAPH_HEIGHT = 30; 





let OverviewView = {
  


  initialize: Task.async(function *() {
    this._onRecordingWillStart = this._onRecordingWillStart.bind(this);
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingWillStop = this._onRecordingWillStop.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);
    this._onRecordingSelected = this._onRecordingSelected.bind(this);
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onGraphSelecting = this._onGraphSelecting.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);

    yield this._showMarkersGraph();
    yield this._showMemoryGraph();
    yield this._showFramerateGraph();

    this.markersOverview.on("selecting", this._onGraphSelecting);

    PerformanceController.on(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);

    
    
    $("#memory-overview").hidden = !PerformanceController.getPref("enable-memory");
    $("#time-framerate").hidden = !PerformanceController.getPref("enable-framerate");

    PerformanceController.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);

    
    this.markersOverview.setData({ duration: 1000, markers: [] });
    this.memoryOverview.setData([]);
    this.framerateGraph.setData([]);
  }),

  


  destroy: function () {
    this.markersOverview.off("selecting", this._onGraphSelecting);

    PerformanceController.off(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);
    PerformanceController.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
  },

  





  setTimeInterval: function(interval, options = {}) {
    let recording = PerformanceController.getCurrentRecording();
    if (recording == null) {
      throw new Error("A recording should be available in order to set the selection.");
    }
    let mapStart = () => 0;
    let mapEnd = () => recording.getDuration();
    let selection = { start: interval.startTime, end: interval.endTime };
    this._stopSelectionChangeEventPropagation = options.stopPropagation;
    this.markersOverview.setMappedSelection(selection, { mapStart, mapEnd });
    this._stopSelectionChangeEventPropagation = false;
  },

  





  getTimeInterval: function() {
    let recording = PerformanceController.getCurrentRecording();
    if (recording == null) {
      throw new Error("A recording should be available in order to get the selection.");
    }
    let mapStart = () => 0;
    let mapEnd = () => recording.getDuration();
    let selection = this.markersOverview.getMappedSelection({ mapStart, mapEnd });
    return { startTime: selection.min, endTime: selection.max };
  },

  


  _showFramerateGraph: Task.async(function *() {
    this.framerateGraph = new LineGraphWidget($("#time-framerate"), {
      metric: L10N.getStr("graphs.fps")
    });
    this.framerateGraph.fixedHeight = FRAMERATE_GRAPH_HEIGHT;
    yield this.framerateGraph.ready();
  }),

  


  _showMarkersGraph: Task.async(function *() {
    this.markersOverview = new MarkersOverview($("#markers-overview"), TIMELINE_BLUEPRINT);
    this.markersOverview.headerHeight = MARKERS_GRAPH_HEADER_HEIGHT;
    this.markersOverview.rowHeight = MARKERS_GRAPH_ROW_HEIGHT;
    this.markersOverview.groupPadding = MARKERS_GROUP_VERTICAL_PADDING;
    yield this.markersOverview.ready();
  }),

  


  _showMemoryGraph: Task.async(function *() {
    this.memoryOverview = new MemoryOverview($("#memory-overview"));
    this.memoryOverview.fixedHeight = MEMORY_GRAPH_HEIGHT;
    yield this.memoryOverview.ready();

    CanvasGraphUtils.linkAnimation(this.markersOverview, this.memoryOverview);
    CanvasGraphUtils.linkSelection(this.markersOverview, this.memoryOverview);
  }),

  


  _showFramerateGraph: Task.async(function *() {
    let metric = L10N.getStr("graphs.fps");
    this.framerateGraph = new LineGraphWidget($("#time-framerate"), { metric });
    this.framerateGraph.fixedHeight = FRAMERATE_GRAPH_HEIGHT;
    yield this.framerateGraph.ready();

    CanvasGraphUtils.linkAnimation(this.markersOverview, this.framerateGraph);
    CanvasGraphUtils.linkSelection(this.markersOverview, this.framerateGraph);
  }),

  





  render: Task.async(function *(resolution) {
    let recording = PerformanceController.getCurrentRecording();
    let duration = recording.getDuration();
    let markers = recording.getMarkers();
    let memory = recording.getMemory();
    let timestamps = recording.getTicks();

    
    if (markers) {
      this.markersOverview.setData({ markers, duration });
      this.emit(EVENTS.MARKERS_GRAPH_RENDERED);
    }
    if (memory) {
      this.memoryOverview.dataDuration = duration;
      this.memoryOverview.setData(memory);
      this.emit(EVENTS.MEMORY_GRAPH_RENDERED);
    }
    if (timestamps) {
      this.framerateGraph.dataDuration = duration;
      yield this.framerateGraph.setDataFromTimestamps(timestamps, resolution);
      this.emit(EVENTS.FRAMERATE_GRAPH_RENDERED);
    }

    
    this.emit(EVENTS.OVERVIEW_RENDERED);
  }),

  




  _onRecordingTick: Task.async(function *() {
    yield this.render(FRAMERATE_GRAPH_LOW_RES_INTERVAL);
    this._prepareNextTick();
  }),

  



  _onGraphSelecting: function () {
    let recording = PerformanceController.getCurrentRecording();
    if (recording == null || this._stopSelectionChangeEventPropagation) {
      return;
    }
    
    
    let interval = this.getTimeInterval();
    if (interval.endTime - interval.startTime < 1) {
      this.emit(EVENTS.OVERVIEW_RANGE_CLEARED);
    } else {
      this.emit(EVENTS.OVERVIEW_RANGE_SELECTED, interval);
    }
  },

  


  _prepareNextTick: function () {
    
    
    if (this._timeoutId) {
      this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    }
  },

  


  _onRecordingWillStart: function (_, recording) {
    this._checkSelection(recording);
    this.framerateGraph.dropSelection();
  },

  


  _onRecordingStarted: function (_, recording) {
    this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
  },

  


  _onRecordingWillStop: function(_, recording) {
    clearTimeout(this._timeoutId);
    this._timeoutId = null;
  },

  


  _onRecordingStopped: function (_, recording) {
    this._checkSelection(recording);
    this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
  },

  


  _onRecordingSelected: function (_, recording) {
    if (!recording) {
      return;
    }
    this.markersOverview.dropSelection();
    this._checkSelection(recording);

    
    
    if (!this._timeoutId) {
      this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
    }
  },

  _checkSelection: function (recording) {
    let selectionEnabled = !recording.isRecording();
    this.markersOverview.selectionEnabled = selectionEnabled;
    this.memoryOverview.selectionEnabled = selectionEnabled;
    this.framerateGraph.selectionEnabled = selectionEnabled;
  },

  



  _onPrefChanged: function (_, prefName, value) {
    if (prefName === "enable-memory") {
      $("#memory-overview").hidden = !PerformanceController.getPref("enable-memory");
    }
    if (prefName === "enable-framerate") {
      $("#time-framerate").hidden = !PerformanceController.getPref("enable-framerate");
    }
  }
};


EventEmitter.decorate(OverviewView);
