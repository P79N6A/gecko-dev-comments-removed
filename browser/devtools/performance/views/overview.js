


"use strict";

const OVERVIEW_UPDATE_INTERVAL = 100;
const FRAMERATE_CALC_INTERVAL = 16; 
const FRAMERATE_GRAPH_HEIGHT = 60; 





let OverviewView = {

  


  initialize: function () {
    this._framerateEl = $("#time-framerate");
    this._ticksData = [];

    this._start = this._start.bind(this);
    this._stop = this._stop.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);
    this._onRecordingTick = this._onRecordingTick.bind(this);

    this._initializeFramerateGraph();

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);
    PerformanceController.on(EVENTS.TIMELINE_DATA, this._onTimelineData);
  },

  


  destroy: function () {
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
  },

  




  _onRecordingTick: Task.async(function *() {
    yield this.framerateGraph.setDataWhenReady(this._ticksData);
    this.emit(EVENTS.OVERVIEW_RENDERED);
    this._draw();
  }),

  


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
