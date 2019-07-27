


"use strict";




let WaterfallView = {
  


  initialize: Task.async(function *() {
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingStoppedOrSelected = this._onRecordingStoppedOrSelected.bind(this);
    this._onMarkerSelected = this._onMarkerSelected.bind(this);
    this._onResize = this._onResize.bind(this);

    this.waterfall = new Waterfall($("#waterfall-breakdown"), $("#details-pane"), TIMELINE_BLUEPRINT);
    this.details = new MarkerDetails($("#waterfall-details"), $("#waterfall-view > splitter"));

    this.waterfall.on("selected", this._onMarkerSelected);
    this.waterfall.on("unselected", this._onMarkerSelected);
    this.details.on("resize", this._onResize);

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);

    this.waterfall.recalculateBounds();
  }),

  


  destroy: function () {
    this.waterfall.off("selected", this._onMarkerSelected);
    this.waterfall.off("unselected", this._onMarkerSelected);
    this.details.off("resize", this._onResize);

    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
  },

  


  render: function() {
    let recording = PerformanceController.getCurrentRecording();
    let { startTime, endTime } = recording.getInterval();
    let markers = recording.getMarkers();

    this.waterfall.setData(markers, startTime, startTime, endTime);

    this.emit(EVENTS.WATERFALL_RENDERED);
  },

  


  _onRecordingStarted: function () {
    this.waterfall.clearView();
  },

  


  _onRecordingStoppedOrSelected: function (_, recording) {
    if (!recording.isRecording()) {
      this.render();
    }
  },

  



  _onMarkerSelected: function (event, marker) {
    let recording = PerformanceController.getCurrentRecording();
    let frames = recording.getFrames();

    if (event === "selected") {
      this.details.render({ toolbox: gToolbox, marker, frames });
    }
    if (event === "unselected") {
      this.details.empty();
    }
  },

  


  _onResize: function () {
    this.waterfall.recalculateBounds();
    this.render();
  }
};




EventEmitter.decorate(WaterfallView);
