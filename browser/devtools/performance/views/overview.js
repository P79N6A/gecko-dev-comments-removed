


"use strict";




const OVERVIEW_UPDATE_INTERVAL = 200; 

const FRAMERATE_GRAPH_LOW_RES_INTERVAL = 100; 
const FRAMERATE_GRAPH_HIGH_RES_INTERVAL = 16; 

const FRAMERATE_GRAPH_HEIGHT = 40; 
const MARKERS_GRAPH_HEADER_HEIGHT = 14; 
const MARKERS_GRAPH_ROW_HEIGHT = 10; 
const MARKERS_GROUP_VERTICAL_PADDING = 4; 
const MEMORY_GRAPH_HEIGHT = 30; 

const GRAPH_SCROLL_EVENTS_DRAIN = 50; 





let OverviewView = {
  


  initialize: Task.async(function *() {
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);
    this._onRecordingSelected = this._onRecordingSelected.bind(this);
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onGraphMouseUp = this._onGraphMouseUp.bind(this);
    this._onGraphScroll = this._onGraphScroll.bind(this);

    yield this._showFramerateGraph();
    yield this._showMarkersGraph();
    yield this._showMemoryGraph();

    this.framerateGraph.on("mouseup", this._onGraphMouseUp);
    this.framerateGraph.on("scroll", this._onGraphScroll);
    this.markersOverview.on("mouseup", this._onGraphMouseUp);
    this.markersOverview.on("scroll", this._onGraphScroll);
    this.memoryOverview.on("mouseup", this._onGraphMouseUp);
    this.memoryOverview.on("scroll", this._onGraphScroll);

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
  }),

  


  destroy: function () {
    this.framerateGraph.off("mouseup", this._onGraphMouseUp);
    this.framerateGraph.off("scroll", this._onGraphScroll);
    this.markersOverview.off("mouseup", this._onGraphMouseUp);
    this.markersOverview.off("scroll", this._onGraphScroll);
    this.memoryOverview.off("mouseup", this._onGraphMouseUp);
    this.memoryOverview.off("scroll", this._onGraphScroll);

    clearNamedTimeout("graph-scroll");
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
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

    CanvasGraphUtils.linkAnimation(this.framerateGraph, this.markersOverview);
    CanvasGraphUtils.linkSelection(this.framerateGraph, this.markersOverview);
  }),

  


  _showMemoryGraph: Task.async(function *() {
    this.memoryOverview = new MemoryOverview($("#memory-overview"));
    this.memoryOverview.fixedHeight = MEMORY_GRAPH_HEIGHT;
    yield this.memoryOverview.ready();

    CanvasGraphUtils.linkAnimation(this.framerateGraph, this.memoryOverview);
    CanvasGraphUtils.linkSelection(this.framerateGraph, this.memoryOverview);
  }),

  





  render: Task.async(function *(resolution) {
    let interval = PerformanceController.getInterval();
    let markers = PerformanceController.getMarkers();
    let memory = PerformanceController.getMemory();
    let timestamps = PerformanceController.getTicks();

    this.markersOverview.setData({ interval, markers });
    this.emit(EVENTS.MARKERS_GRAPH_RENDERED);

    this.memoryOverview.setData({ interval, memory });
    this.emit(EVENTS.MEMORY_GRAPH_RENDERED);

    yield this.framerateGraph.setDataFromTimestamps(timestamps, resolution);
    this.emit(EVENTS.FRAMERATE_GRAPH_RENDERED);

    
    this.emit(EVENTS.OVERVIEW_RENDERED);
  }),

  




  _onRecordingTick: Task.async(function *() {
    yield this.render(FRAMERATE_GRAPH_LOW_RES_INTERVAL);
    this._prepareNextTick();
  }),

  



  _onSelectionChange: function () {
    if (this.framerateGraph.hasSelection()) {
      let { min: beginAt, max: endAt } = this.framerateGraph.getMappedSelection();
      this.emit(EVENTS.OVERVIEW_RANGE_SELECTED, { beginAt, endAt });
    } else {
      this.emit(EVENTS.OVERVIEW_RANGE_CLEARED);
    }
  },

  



  _onGraphMouseUp: function () {
    
    if (this.framerateGraph.selectionEnabled) {
      this._onSelectionChange();
    }
  },

  



  _onGraphScroll: function () {
    setNamedTimeout("graph-scroll", GRAPH_SCROLL_EVENTS_DRAIN, () => {
      this._onSelectionChange();
    });
  },

  


  _prepareNextTick: function () {
    
    
    if (this._timeoutId) {
      this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    }
  },

  


  _onRecordingStarted: function (_, recording) {
    this._checkSelection(recording);
    this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    this.framerateGraph.dropSelection();
  },

  


  _onRecordingStopped: function (_, recording) {
    this._checkSelection(recording);
    clearTimeout(this._timeoutId);
    this._timeoutId = null;

    this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
  },

  


  _onRecordingSelected: function (_, recording) {
    this.framerateGraph.dropSelection();
    this._checkSelection(recording);

    
    
    if (!this._timeoutId) {
      this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
    }
  },

  _checkSelection: function (recording) {
    let selectionEnabled = !recording.isRecording();
    this.framerateGraph.selectionEnabled = selectionEnabled;
    this.markersOverview.selectionEnabled = selectionEnabled;
    this.memoryOverview.selectionEnabled = selectionEnabled;
  }
};


EventEmitter.decorate(OverviewView);
