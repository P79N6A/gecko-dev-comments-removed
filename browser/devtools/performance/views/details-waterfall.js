


"use strict";




let WaterfallView = {
  


  initialize: Task.async(function *() {
    this._start = this._start.bind(this);
    this._stop = this._stop.bind(this);
    this._onMarkerSelected = this._onMarkerSelected.bind(this);
    this._onResize = this._onResize.bind(this);

    this.graph = new Waterfall($("#waterfall-graph"), $("#details-pane"));
    this.markerDetails = new MarkerDetails($("#waterfall-details"), $("#waterfall-view > splitter"));

    this.graph.on("selected", this._onMarkerSelected);
    this.graph.on("unselected", this._onMarkerSelected);
    this.markerDetails.on("resize", this._onResize);

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);

    this.graph.recalculateBounds();
  }),

  


  destroy: function () {
    this.graph.off("selected", this._onMarkerSelected);
    this.graph.off("unselected", this._onMarkerSelected);
    this.markerDetails.off("resize", this._onResize);

    PerformanceController.off(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
  },

  


  render: function() {
    let { startTime, endTime } = PerformanceController.getInterval();
    let markers = PerformanceController.getMarkers();

    this.graph.setData(markers, startTime, startTime, endTime);
    this.emit(EVENTS.WATERFALL_RENDERED);
  },

  


  _start: function (_, { startTime }) {
    this.graph.clearView();
  },

  


  _stop: function (_, { endTime }) {
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
  }
};





EventEmitter.decorate(WaterfallView);
