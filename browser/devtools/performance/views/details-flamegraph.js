


"use strict";





let FlameGraphView = {
  


  initialize: Task.async(function* () {
    this._onRecordingStopped = this._onRecordingStopped.bind(this);
    this._onRangeChange = this._onRangeChange.bind(this);

    this.graph = new FlameGraph($("#flamegraph-view"));
    this.graph.timelineTickUnits = L10N.getStr("graphs.ms");
    yield this.graph.ready();

    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
  }),

  


  destroy: function () {
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
  },

  


  render: function (profilerData) {
    
    if (profilerData.profile == null) {
      return;
    }
    let samples = profilerData.profile.threads[0].samples;
    let dataSrc = FlameGraphUtils.createFlameGraphDataFromSamples(samples, {
      flattenRecursion: Prefs.flattenTreeRecursion,
      filterFrames: !Prefs.showPlatformData && FrameNode.isContent,
    });
    this.graph.setData(dataSrc);
    this.emit(EVENTS.FLAMEGRAPH_RENDERED);
  },

  


  _onRecordingStopped: function () {
    let profilerData = PerformanceController.getProfilerData();
    this.render(profilerData);
  },

  


  _onRangeChange: function (_, params) {
    
  }
};




EventEmitter.decorate(FlameGraphView);
