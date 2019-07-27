


"use strict";




let WaterfallView = {
  


  initialize: Task.async(function *() {
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);
    this._onMarkerSelected = this._onMarkerSelected.bind(this);
    this._onResize = this._onResize.bind(this);

    this.graph = new Waterfall($("#waterfall-graph"), $("#details-pane"));
    this.markerDetails = new MarkerDetails($("#waterfall-details"), $("#waterfall-view > splitter"));

    this.graph.on("selected", this._onMarkerSelected);
    this.graph.on("unselected", this._onMarkerSelected);
    this.markerDetails.on("resize", this._onResize);

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);

    this.graph.recalculateBounds();
  }),

  


  destroy: function () {
    this.graph.off("selected", this._onMarkerSelected);
    this.graph.off("unselected", this._onMarkerSelected);
    this.markerDetails.off("resize", this._onResize);

    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
  },

  


  render: function() {
    let { startTime, endTime } = PerformanceController.getInterval();
    let markers = PerformanceController.getMarkers();

    this.graph.setData(markers, startTime, startTime, endTime);
    this.emit(EVENTS.WATERFALL_RENDERED);
  },

  


  _onRecordingStarted: function () {
    this.graph.clearView();
  },

  


  _onRecordingStopped: function () {
    this.render();
  },

  



  _onMarkerSelected: function (event, marker) {
    if (event === "selected") {
      this.markerDetails.render({
        toolbox: gToolbox,
        marker: marker,
        frames: PerformanceController.getFrames()
      });
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
