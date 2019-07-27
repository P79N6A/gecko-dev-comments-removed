


"use strict";






const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/Graphs.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const { colorUtils: { setAlpha }} = require("devtools/css-color");
const { getColor } = require("devtools/shared/theme");

loader.lazyRequireGetter(this, "L10N",
  "devtools/shared/timeline/global", true);

const OVERVIEW_DAMPEN_VALUES = 0.95;

const OVERVIEW_HEIGHT = 30; 
const OVERVIEW_STROKE_WIDTH = 1; 
const OVERVIEW_MAXIMUM_LINE_COLOR = "rgba(0,136,204,0.4)";
const OVERVIEW_AVERAGE_LINE_COLOR = "rgba(0,136,204,0.7)";
const OVERVIEW_MINIMUM_LINE_COLOR = "rgba(0,136,204,0.9)";
const OVERVIEW_CLIPHEAD_LINE_COLOR = "#666";
const OVERVIEW_SELECTION_LINE_COLOR = "#555";







function MemoryOverview(parent) {
  LineGraphWidget.call(this, parent, { metric: L10N.getStr("graphs.memory") });
  this.setTheme();
}

MemoryOverview.prototype = Heritage.extend(LineGraphWidget.prototype, {
  dampenValuesFactor: OVERVIEW_DAMPEN_VALUES,
  fixedHeight: OVERVIEW_HEIGHT,
  strokeWidth: OVERVIEW_STROKE_WIDTH,
  maximumLineColor: OVERVIEW_MAXIMUM_LINE_COLOR,
  averageLineColor: OVERVIEW_AVERAGE_LINE_COLOR,
  minimumLineColor: OVERVIEW_MINIMUM_LINE_COLOR,
  clipheadLineColor: OVERVIEW_CLIPHEAD_LINE_COLOR,
  selectionLineColor: OVERVIEW_SELECTION_LINE_COLOR,
  withTooltipArrows: false,
  withFixedTooltipPositions: true,

  


  clearView: function() {
    this.selectionEnabled = false;
    this.dropSelection();
    this.setData([]);
  },

  




  setTheme: function (theme) {
    theme = theme || "light";
    this.backgroundColor = getColor("body-background", theme);
    this.backgroundGradientStart = setAlpha(getColor("highlight-blue", theme), 0.1);
    this.backgroundGradientEnd = setAlpha(getColor("highlight-blue", theme), 0);
    this.strokeColor = getColor("highlight-blue", theme);
    this.selectionBackgroundColor = setAlpha(getColor("selection-background", theme), 0.25);
    this.selectionStripesColor = "rgba(255, 255, 255, 0.1)";
  }
});

exports.MemoryOverview = MemoryOverview;
