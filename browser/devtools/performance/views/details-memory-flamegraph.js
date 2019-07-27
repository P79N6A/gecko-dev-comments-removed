


"use strict";





let MemoryFlameGraphView = Heritage.extend(DetailsSubview, {

  shouldUpdateWhileMouseIsActive: true,

  rerenderPrefs: [
    "invert-flame-graph",
    "flatten-tree-recursion",
    "show-idle-blocks"
  ],

  


  initialize: Task.async(function* () {
    DetailsSubview.initialize.call(this);

    this.graph = new FlameGraph($("#memory-flamegraph-view"));
    this.graph.timelineTickUnits = L10N.getStr("graphs.ms");
    this.graph.setTheme(PerformanceController.getTheme());
    yield this.graph.ready();

    this._onRangeChangeInGraph = this._onRangeChangeInGraph.bind(this);
    this._onThemeChanged = this._onThemeChanged.bind(this);

    PerformanceController.on(EVENTS.THEME_CHANGED, this._onThemeChanged);
    this.graph.on("selecting", this._onRangeChangeInGraph);
  }),

  


  destroy: Task.async(function* () {
    DetailsSubview.destroy.call(this);

    PerformanceController.off(EVENTS.THEME_CHANGED, this._onThemeChanged);
    this.graph.off("selecting", this._onRangeChangeInGraph);

    yield this.graph.destroy();
  }),

  





  render: function (interval={}) {
    let recording = PerformanceController.getCurrentRecording();
    let duration = recording.getDuration();
    let allocations = recording.getAllocations();

    let samples = RecordingUtils.getSamplesFromAllocations(allocations);
    let data = FlameGraphUtils.createFlameGraphDataFromSamples(samples, {
      invertStack: PerformanceController.getOption("invert-flame-graph"),
      flattenRecursion: PerformanceController.getOption("flatten-tree-recursion"),
      showIdleBlocks: PerformanceController.getOption("show-idle-blocks") && L10N.getStr("table.idle")
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
  },

  


  _onRerenderPrefChanged: function() {
    let recording = PerformanceController.getCurrentRecording();
    let allocations = recording.getAllocations();
    let samples = RecordingUtils.getSamplesFromAllocations(allocations);
    FlameGraphUtils.removeFromCache(samples);
  },

  


  _onThemeChanged: function (_, theme) {
    this.graph.setTheme(theme);
    this.graph.refresh({ force: true });
  },

  toString: () => "[object MemoryFlameGraphView]"
});
