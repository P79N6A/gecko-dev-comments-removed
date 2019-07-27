


"use strict";




let WaterfallView = {
  _startTime: 0,
  _endTime: 0,
  _markers: [],

  


  initialize: function () {
    this.el = $("#waterfall-view");
    this._stop = this._stop.bind(this);
    this._start = this._start.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);
    this._onMarkerSelected = this._onMarkerSelected.bind(this);
    this._onResize = this._onResize.bind(this);

    this.graph = new Waterfall($("#waterfall-graph"), $("#details-pane"));
    this.markerDetails = new MarkerDetails($("#waterfall-details"), $("#waterfall-view > splitter"));

    this.graph.on("selected", this._onMarkerSelected);
    this.graph.on("unselected", this._onMarkerSelected);
    this.markerDetails.on("resize", this._onResize);

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);
    PerformanceController.on(EVENTS.TIMELINE_DATA, this._onTimelineData);

    this.graph.recalculateBounds();
  },

  


  destroy: function () {
    this.graph.off("selected", this._onMarkerSelected);
    this.graph.off("unselected", this._onMarkerSelected);
    this.markerDetails.off("resize", this._onResize);

    PerformanceController.off(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
    PerformanceController.off(EVENTS.TIMELINE_DATA, this._onTimelineData);
  },

  render: function () {
    this.graph.setData(this._markers, this._startTime, this._startTime, this._endTime);
    this.emit(EVENTS.WATERFALL_RENDERED);
  },

  



  


  _start: function (_, { startTime }) {
    this._startTime = startTime;
    this._endTime = startTime;
    this.graph.clearView();
  },

  


  _stop: function (_, { endTime }) {
    this._endTime = endTime;
    this._markers = this._markers.sort((a,b) => (a.start > b.start));
    this.render();
  },

  



  _onMarkerSelected: function (event, marker) {
    if (event === "selected") {
      this.markerDetails.render(marker);
    }
    if (event === "unselected") {
      this.markerDetails.empty();
    }
  },

  


  _onResize: function () {
    this.graph.recalculateBounds();
    this.render();
  },

  




  _onTimelineData: function (_, eventName, ...data) {
    if (eventName === "markers") {
      let [markers, endTime] = data;
      Array.prototype.push.apply(this._markers, markers);
    }
  }
};





EventEmitter.decorate(WaterfallView);
