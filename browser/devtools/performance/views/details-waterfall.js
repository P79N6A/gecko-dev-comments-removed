


"use strict";




let WaterfallView = Heritage.extend(DetailsSubview, {

  observedPrefs: [
    "hidden-markers"
  ],

  rerenderPrefs: [
    "hidden-markers"
  ],

  rangeChangeDebounceTime: 75, 

  


  initialize: function () {
    DetailsSubview.initialize.call(this);

    this.waterfall = new Waterfall($("#waterfall-breakdown"), $("#waterfall-view"));
    this.details = new MarkerDetails($("#waterfall-details"), $("#waterfall-view > splitter"));

    this._onMarkerSelected = this._onMarkerSelected.bind(this);
    this._onResize = this._onResize.bind(this);
    this._onViewSource = this._onViewSource.bind(this);

    this.waterfall.on("selected", this._onMarkerSelected);
    this.waterfall.on("unselected", this._onMarkerSelected);
    this.details.on("resize", this._onResize);
    this.details.on("view-source", this._onViewSource);

    let blueprint = PerformanceController.getTimelineBlueprint();
    this.waterfall.setBlueprint(blueprint);
    this.waterfall.recalculateBounds();
  },

  


  destroy: function () {
    DetailsSubview.destroy.call(this);

    this.waterfall.off("selected", this._onMarkerSelected);
    this.waterfall.off("unselected", this._onMarkerSelected);
    this.details.off("resize", this._onResize);
    this.details.off("view-source", this._onViewSource);
  },

  





  render: function(interval={}) {
    let recording = PerformanceController.getCurrentRecording();
    let startTime = interval.startTime || 0;
    let endTime = interval.endTime || recording.getDuration();
    let markers = recording.getMarkers();
    this.waterfall.setData({ markers, interval: { startTime, endTime } });
    this.emit(EVENTS.WATERFALL_RENDERED);
  },

  



  _onMarkerSelected: function (event, marker) {
    let recording = PerformanceController.getCurrentRecording();
    
    
    if (!recording) {
      return;
    }
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
  },

  


  _onObservedPrefChange: function(_, prefName) {
    let blueprint = PerformanceController.getTimelineBlueprint();
    this.waterfall.setBlueprint(blueprint);
  },

  


  _onViewSource: function (_, file, line) {
    gToolbox.viewSourceInDebugger(file, line);
  },

  toString: () => "[object WaterfallView]"
});
