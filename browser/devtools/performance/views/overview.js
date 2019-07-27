


"use strict";

const OVERVIEW_UPDATE_INTERVAL = 100;
const FRAMERATE_CALC_INTERVAL = 16; 
const FRAMERATE_GRAPH_HEIGHT = 60; 
const GRAPH_SCROLL_EVENTS_DRAIN = 50; 





let OverviewView = {

  


  initialize: function () {
    this._framerateEl = $("#time-framerate");
    this._ticksData = [];

    this._start = this._start.bind(this);
    this._stop = this._stop.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onGraphMouseUp = this._onGraphMouseUp.bind(this);
    this._onGraphScroll = this._onGraphScroll.bind(this);

    this._initializeFramerateGraph();

    this.framerateGraph.on("mouseup", this._onGraphMouseUp);
    this.framerateGraph.on("scroll", this._onGraphScroll);
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);
    PerformanceController.on(EVENTS.TIMELINE_DATA, this._onTimelineData);
  },

  


  destroy: function () {
    this.framerateGraph.off("mouseup", this._onGraphMouseUp);
    this.framerateGraph.off("scroll", this._onGraphScroll);
    clearNamedTimeout("graph-scroll");
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
    PerformanceController.off(EVENTS.TIMELINE_DATA, this._onTimelineData);
  },

  




  _onRecordingTick: Task.async(function *() {
    yield this.framerateGraph.setDataWhenReady(this._ticksData);
    this.emit(EVENTS.OVERVIEW_RENDERED);
    this._draw();
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
    this._onSelectionChange();
  },

  



  _onGraphScroll: function () {
    setNamedTimeout("graph-scroll", GRAPH_SCROLL_EVENTS_DRAIN, () => {
      this._onSelectionChange();
    });
  },

  


  _initializeFramerateGraph: function () {
    let graph = new LineGraphWidget(this._framerateEl, L10N.getStr("graphs.fps"));
    graph.minDistanceBetweenPoints = 1;
    graph.fixedHeight = FRAMERATE_GRAPH_HEIGHT;
    graph.selectionEnabled = false;
    this.framerateGraph = graph;
  },

  


  _draw: function () {
    
    
    if (this._timeoutId) {
      this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    }
  },

  



  _start: function () {
    this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    this.framerateGraph.dropSelection();
  },

  _stop: function () {
    clearTimeout(this._timeoutId);
    this.framerateGraph.selectionEnabled = true;
  },

  




  _onTimelineData: function (_, eventName, ...data) {
    if (eventName === "ticks") {
      let [delta, timestamps] = data;
      
      
      
      this._ticksData = FramerateFront.plotFPS(timestamps, FRAMERATE_CALC_INTERVAL);
    }
  }
};


EventEmitter.decorate(OverviewView);
