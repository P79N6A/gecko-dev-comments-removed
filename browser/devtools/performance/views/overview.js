


"use strict";




const OVERVIEW_UPDATE_INTERVAL = 200; 

const FRAMERATE_GRAPH_LOW_RES_INTERVAL = 100; 
const FRAMERATE_GRAPH_HIGH_RES_INTERVAL = 16; 

const FRAMERATE_GRAPH_HEIGHT = 45; 
const MARKERS_GRAPH_HEADER_HEIGHT = 12; 
const MARKERS_GRAPH_BODY_HEIGHT = 45; 
const MARKERS_GROUP_VERTICAL_PADDING = 3.5; 
const MEMORY_GRAPH_HEIGHT = 30; 

const GRAPH_SCROLL_EVENTS_DRAIN = 50; 





let OverviewView = {
  


  initialize: Task.async(function *() {
    this._start = this._start.bind(this);
    this._stop = this._stop.bind(this);
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

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);
  }),

  


  destroy: function () {
    this.framerateGraph.off("mouseup", this._onGraphMouseUp);
    this.framerateGraph.off("scroll", this._onGraphScroll);
    this.markersOverview.off("mouseup", this._onGraphMouseUp);
    this.markersOverview.off("scroll", this._onGraphScroll);
    this.memoryOverview.off("mouseup", this._onGraphMouseUp);
    this.memoryOverview.off("scroll", this._onGraphScroll);

    clearNamedTimeout("graph-scroll");
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
  },

  


  _showFramerateGraph: Task.async(function *() {
    this.framerateGraph = new LineGraphWidget($("#time-framerate"), L10N.getStr("graphs.fps"));
    this.framerateGraph.fixedHeight = FRAMERATE_GRAPH_HEIGHT;
    yield this.framerateGraph.ready();
  }),

  


  _showMarkersGraph: Task.async(function *() {
    this.markersOverview = new MarkersOverview($("#markers-overview"));
    this.markersOverview.headerHeight = MARKERS_GRAPH_HEADER_HEIGHT;
    this.markersOverview.bodyHeight = MARKERS_GRAPH_BODY_HEIGHT;
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

    
    
    
    let fakeTime = interval.startTime + interval.localElapsedTime;
    interval.endTime = fakeTime;

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

  


  _start: function () {
    this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);

    this.framerateGraph.dropSelection();
    this.framerateGraph.selectionEnabled = false;
    this.markersOverview.selectionEnabled = false;
    this.memoryOverview.selectionEnabled = false;
  },

  


  _stop: function () {
    clearTimeout(this._timeoutId);
    this._timeoutId = null;

    this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);

    this.framerateGraph.selectionEnabled = true;
    this.markersOverview.selectionEnabled = true;
    this.memoryOverview.selectionEnabled = true;
  }
};


EventEmitter.decorate(OverviewView);
