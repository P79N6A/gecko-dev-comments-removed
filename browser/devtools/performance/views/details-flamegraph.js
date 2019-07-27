


"use strict";





let FlameGraphView = {
  


  initialize: Task.async(function* () {
    this._onRecordingStoppedOrSelected = this._onRecordingStoppedOrSelected.bind(this);
    this._onRangeChange = this._onRangeChange.bind(this);

    this.graph = new FlameGraph($("#flamegraph-view"));
    this.graph.timelineTickUnits = L10N.getStr("graphs.ms");
    yield this.graph.ready();

    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
  }),

  


  destroy: function () {
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
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
      showIdleBlocks: Prefs.showIdleBlocks && L10N.getStr("table.idle")
    });
    this.graph.setData(dataSrc);
    this.emit(EVENTS.FLAMEGRAPH_RENDERED);
  },

  


  _onRecordingStoppedOrSelected: function (_, recording) {
    
    if (!recording.isRecording()) {
      let profilerData = recording.getProfilerData();
      this.render(profilerData);
    }
  },

  


  _onRangeChange: function (_, params) {
    
  }
};




EventEmitter.decorate(FlameGraphView);
