


"use strict";





let FlameGraphView = {
  


  initialize: Task.async(function* () {
    this._onRecordingStoppedOrSelected = this._onRecordingStoppedOrSelected.bind(this);
    this._onRangeChange = this._onRangeChange.bind(this);
    this._onRangeChangeInGraph = this._onRangeChangeInGraph.bind(this);
    this._onDetailsViewSelected = this._onDetailsViewSelected.bind(this);

    this.graph = new FlameGraph($("#flamegraph-view"));
    this.graph.timelineTickUnits = L10N.getStr("graphs.ms");
    yield this.graph.ready();

    this.graph.on("selecting", this._onRangeChangeInGraph);

    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
    DetailsView.on(EVENTS.DETAILS_VIEW_SELECTED, this._onDetailsViewSelected);
  }),

  


  destroy: function () {
    this.graph.off("selecting", this._onRangeChangeInGraph);

    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
    DetailsView.off(EVENTS.DETAILS_VIEW_SELECTED, this._onDetailsViewSelected);
  },

  





  render: function (interval={}) {
    let recording = PerformanceController.getCurrentRecording();
    let startTime = interval.startTime || 0;
    let endTime = interval.endTime || recording.getDuration();
    this.graph.setViewRange({ startTime, endTime });
    this.emit(EVENTS.FLAMEGRAPH_RENDERED);
  },

  


  _onRecordingStoppedOrSelected: function (_, recording) {
    if (recording.isRecording()) {
      return;
    }
    let profile = recording.getProfile();
    let samples = profile.threads[0].samples;
    let data = FlameGraphUtils.createFlameGraphDataFromSamples(samples, {
      flattenRecursion: Prefs.flattenTreeRecursion,
      filterFrames: !Prefs.showPlatformData && FrameNode.isContent,
      showIdleBlocks: Prefs.showIdleBlocks && L10N.getStr("table.idle")
    });
    let startTime = 0;
    let endTime = recording.getDuration();
    this.graph.setData({ data, bounds: { startTime, endTime } });
    this.render();
  },

  


  _onRangeChange: function (_, interval) {
    if (DetailsView.isViewSelected(this)) {
      this.render(interval);
    } else {
      this._dirty = true;
      this._interval = interval;
    }
  },

  


  _onRangeChangeInGraph: function () {
    let interval = this.graph.getViewRange();
    OverviewView.setTimeInterval(interval, { stopPropagation: true });
  },

  


  _onDetailsViewSelected: function() {
    if (DetailsView.isViewSelected(this) && this._dirty) {
      this.render(this._interval);
      this._dirty = false;
    }
  }
};




EventEmitter.decorate(FlameGraphView);
