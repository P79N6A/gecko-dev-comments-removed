


"use strict";





const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/Graphs.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const { colorUtils: { setAlpha }} = require("devtools/css-color");
const { getColor } = require("devtools/shared/theme");

loader.lazyRequireGetter(this, "ProfilerGlobal",
  "devtools/shared/profiler/global");
loader.lazyRequireGetter(this, "TimelineGlobal",
  "devtools/shared/timeline/global");

const HEIGHT = 35; 
const STROKE_WIDTH = 1; 
const DAMPEN_VALUES = 0.95;
const CLIPHEAD_LINE_COLOR = "#666";
const SELECTION_LINE_COLOR = "#555";
const SELECTION_BACKGROUND_COLOR_NAME = "highlight-blue";
const FRAMERATE_GRAPH_COLOR_NAME = "highlight-green";
const MEMORY_GRAPH_COLOR_NAME = "highlight-blue";









function PerformanceGraph(parent, metric) {
  LineGraphWidget.call(this, parent, { metric });
  this.setTheme();
}

PerformanceGraph.prototype = Heritage.extend(LineGraphWidget.prototype, {
  strokeWidth: STROKE_WIDTH,
  dampenValuesFactor: DAMPEN_VALUES,
  fixedHeight: HEIGHT,
  clipheadLineColor: CLIPHEAD_LINE_COLOR,
  selectionLineColor: SELECTION_LINE_COLOR,
  withTooltipArrows: false,
  withFixedTooltipPositions: true,

  


  clearView: function() {
    this.selectionEnabled = false;
    this.dropSelection();
    this.setData([]);
  },

  




  setTheme: function (theme) {
    theme = theme || "light";
    let mainColor = getColor(this.mainColor || "highlight-blue", theme);
    this.backgroundColor = getColor("body-background", theme);
    this.strokeColor = mainColor;
    this.backgroundGradientStart = setAlpha(mainColor, 0.2);
    this.backgroundGradientEnd = setAlpha(mainColor, 0.2);
    this.selectionBackgroundColor = setAlpha(getColor(SELECTION_BACKGROUND_COLOR_NAME, theme), 0.25);
    this.selectionStripesColor = "rgba(255, 255, 255, 0.1)";
    this.maximumLineColor = setAlpha(mainColor, 0.4);
    this.averageLineColor = setAlpha(mainColor, 0.7);
    this.minimumLineColor = setAlpha(mainColor, 0.9);
  }
});







function FramerateGraph(parent) {
  PerformanceGraph.call(this, parent, ProfilerGlobal.L10N.getStr("graphs.fps"));
}

FramerateGraph.prototype = Heritage.extend(PerformanceGraph.prototype, {
  mainColor: FRAMERATE_GRAPH_COLOR_NAME
});

exports.FramerateGraph = FramerateGraph;







function MemoryGraph(parent) {
  PerformanceGraph.call(this, parent, TimelineGlobal.L10N.getStr("graphs.memory"));
}

MemoryGraph.prototype = Heritage.extend(PerformanceGraph.prototype, {
  mainColor: MEMORY_GRAPH_COLOR_NAME
});

exports.MemoryGraph = MemoryGraph;
