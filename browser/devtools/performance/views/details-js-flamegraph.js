


"use strict";





let JsFlameGraphView = Heritage.extend(DetailsSubview, {

  rerenderPrefs: [
    "invert-flame-graph",
    "flatten-tree-recursion",
    "show-platform-data",
    "show-idle-blocks"
  ],

  


  initialize: Task.async(function* () {
    DetailsSubview.initialize.call(this);

    this.graph = new FlameGraph($("#js-flamegraph-view"));
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
    let profile = recording.getProfile();
    let samples = profile.threads[0].samples;

    let data = FlameGraphUtils.createFlameGraphDataFromSamples(samples, {
      invertStack: PerformanceController.getOption("invert-flame-graph"),
      flattenRecursion: PerformanceController.getOption("flatten-tree-recursion"),
      filterFrames: !PerformanceController.getOption("show-platform-data") && FrameNode.isContent,
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

    this.emit(EVENTS.JS_FLAMEGRAPH_RENDERED);
  },

  


  _onRangeChangeInGraph: function () {
    let interval = this.graph.getViewRange();
    OverviewView.setTimeInterval(interval, { stopPropagation: true });
  },

  


  _onRerenderPrefChanged: function() {
    let recording = PerformanceController.getCurrentRecording();
    let profile = recording.getProfile();
    let samples = profile.threads[0].samples;
    FlameGraphUtils.removeFromCache(samples);
  },

  


  _onThemeChanged: function (_, theme) {
    this.graph.setTheme(theme);
    this.graph.refresh({ force: true });
  },

  toString: () => "[object JsFlameGraphView]"
});
