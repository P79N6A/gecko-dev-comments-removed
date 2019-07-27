


"use strict";





let JsFlameGraphView = Heritage.extend(DetailsSubview, {
  


  initialize: Task.async(function* () {
    DetailsSubview.initialize.call(this);

    this.graph = new FlameGraph($("#js-flamegraph-view"));
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
    let profile = recording.getProfile();
    let samples = profile.threads[0].samples;

    let data = FlameGraphUtils.createFlameGraphDataFromSamples(samples, {
      flattenRecursion: Prefs.flattenTreeRecursion,
      filterFrames: !Prefs.showPlatformData && FrameNode.isContent,
      showIdleBlocks: Prefs.showIdleBlocks && L10N.getStr("table.idle")
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
  }
});
