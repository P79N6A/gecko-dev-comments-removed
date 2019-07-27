


"use strict";







const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/Graphs.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

loader.lazyRequireGetter(this, "L10N",
  "devtools/timeline/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/timeline/global", true);

const HTML_NS = "http://www.w3.org/1999/xhtml";

const OVERVIEW_HEADER_HEIGHT = 14; 
const OVERVIEW_BODY_HEIGHT = 55; 

const OVERVIEW_BACKGROUND_COLOR = "#fff";
const OVERVIEW_CLIPHEAD_LINE_COLOR = "#666";
const OVERVIEW_SELECTION_LINE_COLOR = "#555";
const OVERVIEW_SELECTION_BACKGROUND_COLOR = "rgba(76,158,217,0.25)";
const OVERVIEW_SELECTION_STRIPES_COLOR = "rgba(255,255,255,0.1)";

const OVERVIEW_HEADER_TICKS_MULTIPLE = 100; 
const OVERVIEW_HEADER_TICKS_SPACING_MIN = 75; 
const OVERVIEW_HEADER_SAFE_BOUNDS = 50; 
const OVERVIEW_HEADER_BACKGROUND = "#fff";
const OVERVIEW_HEADER_TEXT_COLOR = "#18191a";
const OVERVIEW_HEADER_TEXT_FONT_SIZE = 9; 
const OVERVIEW_HEADER_TEXT_FONT_FAMILY = "sans-serif";
const OVERVIEW_HEADER_TEXT_PADDING_LEFT = 6; 
const OVERVIEW_HEADER_TEXT_PADDING_TOP = 1; 
const OVERVIEW_TIMELINE_STROKES = "#ccc";
const OVERVIEW_MARKERS_COLOR_STOPS = [0, 0.1, 0.75, 1];
const OVERVIEW_MARKER_DURATION_MIN = 4; 
const OVERVIEW_GROUP_VERTICAL_PADDING = 5; 
const OVERVIEW_GROUP_ALTERNATING_BACKGROUND = "rgba(0,0,0,0.05)";







function MarkersOverview(parent, ...args) {
  AbstractCanvasGraph.apply(this, [parent, "markers-overview", ...args]);
  this.once("ready", () => {
    
    this.setBlueprint(TIMELINE_BLUEPRINT);

    
    this.setData({ interval: { startTime: 0, endTime: 1000 }, markers: [] });
  });
}

MarkersOverview.prototype = Heritage.extend(AbstractCanvasGraph.prototype, {
  fixedHeight: OVERVIEW_HEADER_HEIGHT + OVERVIEW_BODY_HEIGHT,
  clipheadLineColor: OVERVIEW_CLIPHEAD_LINE_COLOR,
  selectionLineColor: OVERVIEW_SELECTION_LINE_COLOR,
  selectionBackgroundColor: OVERVIEW_SELECTION_BACKGROUND_COLOR,
  selectionStripesColor: OVERVIEW_SELECTION_STRIPES_COLOR,

  



  setBlueprint: function(blueprint) {
    this._paintBatches = new Map();
    this._lastGroup = 0;

    for (let type in blueprint) {
      this._paintBatches.set(type, { style: blueprint[type], batch: [] });
      this._lastGroup = Math.max(this._lastGroup, blueprint[type].group);
    }
  },

  


  clearView: function() {
    this.selectionEnabled = false;
    this.dropSelection();
    this.setData({ interval: { startTime: 0, endTime: 0 }, markers: [] });
  },

  



  buildGraphImage: function() {
    let { interval, markers } = this._data;
    let { startTime, endTime } = interval;

    let { canvas, ctx } = this._getNamedCanvas("overview-data");
    let canvasWidth = this._width;
    let canvasHeight = this._height;
    let safeBounds = OVERVIEW_HEADER_SAFE_BOUNDS * this._pixelRatio;
    let availableWidth = canvasWidth - safeBounds;

    
    

    for (let marker of markers) {
      this._paintBatches.get(marker.name).batch.push(marker);
    }

    

    let totalGroups = this._lastGroup + 1;
    let headerHeight = OVERVIEW_HEADER_HEIGHT * this._pixelRatio;
    let groupHeight = OVERVIEW_BODY_HEIGHT * this._pixelRatio / totalGroups;
    let groupPadding = OVERVIEW_GROUP_VERTICAL_PADDING * this._pixelRatio;

    let totalTime = (endTime - startTime) || 0;
    let dataScale = this.dataScaleX = availableWidth / totalTime;

    

    ctx.fillStyle = OVERVIEW_HEADER_BACKGROUND;
    ctx.fillRect(0, 0, canvasWidth, headerHeight);

    ctx.fillStyle = OVERVIEW_BACKGROUND_COLOR;
    ctx.fillRect(0, headerHeight, canvasWidth, canvasHeight);

    

    ctx.fillStyle = OVERVIEW_GROUP_ALTERNATING_BACKGROUND;
    ctx.beginPath();

    for (let i = 0; i < totalGroups; i += 2) {
      let top = headerHeight + i * groupHeight;
      ctx.rect(0, top, canvasWidth, groupHeight);
    }

    ctx.fill();

    

    let fontSize = OVERVIEW_HEADER_TEXT_FONT_SIZE * this._pixelRatio;
    let fontFamily = OVERVIEW_HEADER_TEXT_FONT_FAMILY;
    let textPaddingLeft = OVERVIEW_HEADER_TEXT_PADDING_LEFT * this._pixelRatio;
    let textPaddingTop = OVERVIEW_HEADER_TEXT_PADDING_TOP * this._pixelRatio;
    let tickInterval = this._findOptimalTickInterval(dataScale);

    ctx.textBaseline = "middle";
    ctx.font = fontSize + "px " + fontFamily;
    ctx.fillStyle = OVERVIEW_HEADER_TEXT_COLOR;
    ctx.strokeStyle = OVERVIEW_TIMELINE_STROKES;
    ctx.beginPath();

    for (let x = 0; x < availableWidth; x += tickInterval) {
      let left = x + textPaddingLeft;
      let time = Math.round(x / dataScale);
      let label = L10N.getFormatStr("timeline.tick", time);
      ctx.fillText(label, left, headerHeight / 2 + textPaddingTop);
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvasHeight);
    }

    ctx.stroke();

    

    for (let [, { style, batch }] of this._paintBatches) {
      let top = headerHeight + style.group * groupHeight + groupPadding / 2;
      let height = groupHeight - groupPadding;

      let gradient = ctx.createLinearGradient(0, top, 0, top + height);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[0], style.stroke);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[1], style.fill);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[2], style.fill);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[3], style.stroke);
      ctx.fillStyle = gradient;
      ctx.beginPath();

      for (let { start, end } of batch) {
        start -= interval.startTime;
        end -= interval.startTime;

        let left = start * dataScale;
        let duration = Math.max(end - start, OVERVIEW_MARKER_DURATION_MIN);
        let width = Math.max(duration * dataScale, this._pixelRatio);
        ctx.rect(left, top, width, height);
      }

      ctx.fill();

      
      
      
      batch.length = 0;
    }

    return canvas;
  },

  


  _findOptimalTickInterval: function(dataScale) {
    let timingStep = OVERVIEW_HEADER_TICKS_MULTIPLE;
    let spacingMin = OVERVIEW_HEADER_TICKS_SPACING_MIN * this._pixelRatio;

    while (true) {
      let scaledStep = dataScale * timingStep;
      if (scaledStep < spacingMin) {
        timingStep <<= 1;
        continue;
      }
      return scaledStep;
    }
  }
});

exports.MarkersOverview = MarkersOverview;
