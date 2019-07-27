


"use strict";






const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/Graphs.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

loader.lazyRequireGetter(this, "L10N",
  "devtools/timeline/global", true);

const HTML_NS = "http://www.w3.org/1999/xhtml";

const OVERVIEW_HEIGHT = 30; 

const OVERVIEW_BACKGROUND_COLOR = "#fff";
const OVERVIEW_BACKGROUND_GRADIENT_START = "rgba(0,136,204,0.1)";
const OVERVIEW_BACKGROUND_GRADIENT_END = "rgba(0,136,204,0.0)";
const OVERVIEW_STROKE_WIDTH = 1; 
const OVERVIEW_STROKE_COLOR = "rgba(0,136,204,1)";
const OVERVIEW_CLIPHEAD_LINE_COLOR = "#666";
const OVERVIEW_SELECTION_LINE_COLOR = "#555";
const OVERVIEW_MAXIMUM_LINE_COLOR = "rgba(0,136,204,0.4)";
const OVERVIEW_AVERAGE_LINE_COLOR = "rgba(0,136,204,0.7)";
const OVERVIEW_MINIMUM_LINE_COLOR = "rgba(0,136,204,0.9)";

const OVERVIEW_SELECTION_BACKGROUND_COLOR = "rgba(76,158,217,0.25)";
const OVERVIEW_SELECTION_STRIPES_COLOR = "rgba(255,255,255,0.1)";







function MemoryOverview(parent) {
  LineGraphWidget.call(this, parent, L10N.getStr("graphs.memory"));

  this.once("ready", () => {
    
    this.setData({ interval: { startTime: 0, endTime: 1000 }, memory: [] });
  });
}

MemoryOverview.prototype = Heritage.extend(LineGraphWidget.prototype, {
  dampenValuesFactor: 0.95,
  fixedHeight: OVERVIEW_HEIGHT,
  backgroundColor: OVERVIEW_BACKGROUND_COLOR,
  backgroundGradientStart: OVERVIEW_BACKGROUND_GRADIENT_START,
  backgroundGradientEnd: OVERVIEW_BACKGROUND_GRADIENT_END,
  strokeColor: OVERVIEW_STROKE_COLOR,
  strokeWidth: OVERVIEW_STROKE_WIDTH,
  maximumLineColor: OVERVIEW_MAXIMUM_LINE_COLOR,
  averageLineColor: OVERVIEW_AVERAGE_LINE_COLOR,
  minimumLineColor: OVERVIEW_MINIMUM_LINE_COLOR,
  clipheadLineColor: OVERVIEW_CLIPHEAD_LINE_COLOR,
  selectionLineColor: OVERVIEW_SELECTION_LINE_COLOR,
  selectionBackgroundColor: OVERVIEW_SELECTION_BACKGROUND_COLOR,
  selectionStripesColor: OVERVIEW_SELECTION_STRIPES_COLOR,
  withTooltipArrows: false,
  withFixedTooltipPositions: true,

  


  clearView: function() {
    this.selectionEnabled = false;
    this.dropSelection();
    this.setData({ interval: { startTime: 0, endTime: 0 }, memory: [] });
  },

  


  setData: function({ interval, memory }) {
    this.dataOffsetX = interval.startTime;
    LineGraphWidget.prototype.setData.call(this, memory);
  }
});

exports.MemoryOverview = MemoryOverview;
