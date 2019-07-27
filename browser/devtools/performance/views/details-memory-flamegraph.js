


"use strict";





let MemoryFlameGraphView = Heritage.extend(DetailsSubview, {

  rerenderPrefs: ["flatten-tree-recursion", "show-idle-blocks"],

  


  initialize: Task.async(function* () {
    DetailsSubview.initialize.call(this);

    this.graph = new FlameGraph($("#memory-flamegraph-view"));
    this.graph.timelineTickUnits = L10N.getStr("graphs.ms");
    yield this.graph.ready();

    this._onRangeChangeInGraph = this._onRangeChangeInGraph.bind(this);

    this.graph.on("selecting", this._onRangeChangeInGraph);
  }),

  


  destroy: function () {
    DetailsSubview.destroy.call(this);

    this.graph.off("selecting", this._onRangeChangeInGraph);
  },

  





  render: function (interval={}) {
    let recording = PerformanceController.getCurrentRecording();
    let duration = recording.getDuration();
    let allocations = recording.getAllocations();

    let samples = RecordingUtils.getSamplesFromAllocations(allocations);
    let data = FlameGraphUtils.createFlameGraphDataFromSamples(samples, {
      flattenRecursion: PerformanceController.getPref("flatten-tree-recursion"),
      showIdleBlocks: PerformanceController.getPref("show-idle-blocks") && L10N.getStr("table.idle")
    });

    this.graph.setData({ data,
      bounds: {
        startTime: 0,
        endTime: duration
      },
      visible: {
        startTime: interval.startTime || 0,
        endTime: interval.endTime || duration
      }
    });

    this.emit(EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  },

  


  _onRangeChangeInGraph: function () {
    let interval = this.graph.getViewRange();
    OverviewView.setTimeInterval(interval, { stopPropagation: true });
  }
});
